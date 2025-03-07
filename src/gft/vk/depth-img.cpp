#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

bool DepthImage::create(
  const Context& ctxt,
  const DepthImageConfig& depth_img_cfg,
  DepthImage& out
) {
  VkFormat fmt = depth_fmt2vk(depth_img_cfg.fmt);
  VkImageUsageFlags usage = 0;
  SubmitType init_submit_ty = L_SUBMIT_TYPE_ANY;

  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SAMPLED_BIT) {
    usage |=
      VK_IMAGE_USAGE_SAMPLED_BIT |
      VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    init_submit_ty = L_SUBMIT_TYPE_ANY;
  }
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_ATTACHMENT_BIT) {
    usage |=
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }
  // KEEP THIS AFTER ANY SUBMIT TYPES.
  if (depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_SUBPASS_DATA_BIT) {
    usage |=
      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
      VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    init_submit_ty = L_SUBMIT_TYPE_GRAPHICS;
  }

  // Check whether the device support our use case.
  VkImageFormatProperties ifp;
  VK_ASSERT << vkGetPhysicalDeviceImageFormatProperties(ctxt.physdev(), fmt,
    VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, usage, 0, &ifp);

  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImageCreateInfo ici {};
  ici.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  ici.imageType = VK_IMAGE_TYPE_2D;
  ici.format = fmt;
  ici.extent.width = (uint32_t)depth_img_cfg.width;
  ici.extent.height = (uint32_t)depth_img_cfg.height;
  ici.extent.depth = 1;
  ici.mipLevels = 1;
  ici.arrayLayers = 1;
  ici.samples = VK_SAMPLE_COUNT_1_BIT;
  ici.tiling = VK_IMAGE_TILING_OPTIMAL;
  ici.usage = usage;
  ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  ici.initialLayout = layout;

  bool is_tile_mem = depth_img_cfg.usage & L_DEPTH_IMAGE_USAGE_TILE_MEMORY_BIT;
  sys::ImageRef img;
  VmaAllocationCreateInfo aci {};
  VkResult res = VK_ERROR_OUT_OF_DEVICE_MEMORY;
  if (is_tile_mem) {
    aci.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
    img = sys::Image::create(*ctxt.allocator, &ici, &aci);
  }
  if (res != VK_SUCCESS) {
    if (is_tile_mem) {
      L_WARN("tile-memory is unsupported, fall back to regular memory");
    }
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    img = sys::Image::create(*ctxt.allocator, &ici, &aci);
  }

  VkImageViewCreateInfo ivci {};
  ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  ivci.image = img->img;
  ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
  ivci.format = fmt;
  ivci.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  ivci.subresourceRange.aspectMask =
    (fmt::get_fmt_depth_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
    (fmt::get_fmt_stencil_nbit(depth_img_cfg.fmt) > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
  ivci.subresourceRange.baseArrayLayer = 0;
  ivci.subresourceRange.layerCount = 1;
  ivci.subresourceRange.baseMipLevel = 0;
  ivci.subresourceRange.levelCount = 1;

  sys::ImageViewRef img_view = sys::ImageView::create(ctxt.dev->dev, &ivci);

  DepthImageDynamicDetail dyn_detail {};
  dyn_detail.layout = layout;
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  out.ctxt = &ctxt;
  out.img = std::move(img);
  out.img_view = std::move(img_view);
  out.depth_img_cfg = depth_img_cfg;
  out.dyn_detail = std::move(dyn_detail);
  L_DEBUG("created depth image '", depth_img_cfg.label, "'");
  return true;
}
DepthImage::~DepthImage() {
  if (img) {
    L_DEBUG("destroyed depth image '", depth_img_cfg.label, "'");
  }
}
const DepthImageConfig& DepthImage::cfg() const {
  return depth_img_cfg;
}
DepthImageView DepthImage::view(
  uint32_t x_offset,
  uint32_t y_offset,
  uint32_t width,
  uint32_t height,
  DepthImageSampler sampler
) const {
  const DepthImageConfig& cfg2 = cfg();
  DepthImageView out {};
  out.depth_img = this;
  out.x_offset = x_offset;
  out.y_offset = y_offset;
  out.width = width;
  out.height = height;
  out.sampler = sampler;
  return out;
}

} // namespace vk
} // namespace liong
