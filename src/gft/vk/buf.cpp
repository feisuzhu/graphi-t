#include "gft/vk.hpp"
#include "gft/log.hpp"

namespace liong {
namespace vk {

constexpr VmaMemoryUsage _host_access2vma_usage(MemoryAccess host_access) {
  if (host_access == 0) {
    return VMA_MEMORY_USAGE_GPU_ONLY;
  } else if (host_access == L_MEMORY_ACCESS_READ_BIT) {
    return VMA_MEMORY_USAGE_GPU_TO_CPU;
  } else if (host_access == L_MEMORY_ACCESS_WRITE_BIT) {
    return VMA_MEMORY_USAGE_CPU_TO_GPU;
  } else {
    return VMA_MEMORY_USAGE_CPU_ONLY;
  }
}
bool Buffer::create(
  const Context& ctxt,
  const BufferConfig& buf_cfg,
  Buffer& out
) {
  VkBufferCreateInfo bci {};
  bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  if (buf_cfg.usage & L_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_TRANSFER_DST_BIT) {
    bci.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_UNIFORM_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_STORAGE_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_VERTEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  if (buf_cfg.usage & L_BUFFER_USAGE_INDEX_BIT) {
    bci.usage |=
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
      VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }
  bci.size = buf_cfg.size;

  VmaAllocationCreateInfo aci {};
  aci.usage = _host_access2vma_usage(buf_cfg.host_access);

  sys::BufferRef buf = sys::Buffer::create(*ctxt.allocator, &bci, &aci);

  BufferDynamicDetail dyn_detail {};
  dyn_detail.access = 0;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  out.ctxt = &ctxt;
  out.buf = std::move(buf);
  out.buf_cfg = buf_cfg;
  out.dyn_detail = std::move(dyn_detail);
  L_DEBUG("created buffer '", buf_cfg.label, "'");
  return true;
}
Buffer::~Buffer() {
  if (buf) {
    L_DEBUG("destroyed buffer '", buf_cfg.label, "'");
  }
}

const BufferConfig& Buffer::cfg() const {
  return buf_cfg;
}

void* Buffer::map(MemoryAccess map_access) {
  L_ASSERT(map_access != 0, "memory map access must be read, write or both");

  void* mapped = nullptr;
  VK_ASSERT << vmaMapMemory(*ctxt->allocator, buf->alloc, &mapped);

  dyn_detail.access = map_access == L_MEMORY_ACCESS_READ_BIT ?
    VK_ACCESS_HOST_READ_BIT : VK_ACCESS_HOST_WRITE_BIT;
  dyn_detail.stage = VK_PIPELINE_STAGE_HOST_BIT;

  L_DEBUG("mapped buffer '", buf_cfg.label, "'");
  return (uint8_t*)mapped;
}
void Buffer::unmap(void* mapped) {
  vmaUnmapMemory(*ctxt->allocator, buf->alloc);
  L_DEBUG("unmapped buffer '", buf_cfg.label, "'");
}

BufferView Buffer::view(size_t offset, size_t size) const {
  BufferView out {};
  out.buf = this;
  out.offset = offset;
  out.size = size;
  return out;
}

} // namespace vk
} // namespace liong
