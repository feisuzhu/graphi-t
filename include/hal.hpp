﻿// # Hardware abstraction layer
// @PENGUINLIONG
//
// This file defines all APIs to be implemented by each platform and provides
// interfacing structures to the most common extent.
//
// Before implementing a HAL, please include `common.hpp` at the beginning of
// your header.

// Don't use `#pragma once` here.
#pragma once
#include <cmath>
#include <array>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include "assert.hpp"

#ifndef HAL_IMPL_NAMESPACE
static_assert(false, "please specify the implementation namespace (e.g. `vk`)");
#endif

// Denote a function to be defined by HAL implementation.
#define L_IMPL_FN extern
// Denote a structure to be defined by HAL implementation.
#define L_IMPL_STRUCT

namespace liong {

namespace HAL_IMPL_NAMESPACE {

// Initialize the implementation of the HAL.
L_IMPL_FN void initialize();

// Generate Human-readable string to describe the properties and capabilities
// of the device at index `idx`. If there is no device at `idx`, an empty string
// is returned.
L_IMPL_FN std::string desc_dev(uint32_t idx);



struct ContextConfig {
  // Human-readable label of the context.
  std::string label;
  // Index of the device.
  uint32_t dev_idx;
};
L_IMPL_STRUCT struct Context;
L_IMPL_FN Context create_ctxt(const ContextConfig& cfg);
L_IMPL_FN void destroy_ctxt(Context& ctxt);
L_IMPL_FN const ContextConfig& get_ctxt_cfg(const Context& ctxt);



enum MemoryAccessBits {
  L_MEMORY_ACCESS_READ_BIT = 0b01,
  L_MEMORY_ACCESS_WRITE_BIT = 0b10,

  L_MEMORY_ACCESS_NONE = 0,
  L_MEMORY_ACCESS_READ_ONLY = L_MEMORY_ACCESS_READ_BIT,
  L_MEMORY_ACCESS_WRITE_ONLY = L_MEMORY_ACCESS_WRITE_BIT,
  L_MEMORY_ACCESS_READ_WRITE =
    L_MEMORY_ACCESS_READ_BIT | L_MEMORY_ACCESS_WRITE_BIT,
};
typedef uint32_t MemoryAccess;

// Calculate a minimal size of allocation that guarantees that we can sub-
// allocate an address-aligned memory of `size`.
constexpr size_t align_size(size_t size, size_t align) {
  return (size + (align - 1));
}
// Align pointer address to the next aligned address. This funciton assumes that
// `align` is a power-of-2.
constexpr size_t align_addr(size_t size, size_t align) {
  return (size + (align - 1)) & (~(align - 1));
}

// Encoded pixel format that can be easily decoded by shift-and ops.
//
//   0bccshibbb
//       \____/
//  `CUarray_format`
//
// - `b`: Number of bits in exponent of 2. Only assigned if its a integral
//   number.
// - `i`: Signedness of integral number.
// - `h`: Is a half-precision floating-point number.
// - `s`: Is a single-precision floating-point number.
// - `c`: Number of texel components (channels) minus 1. Currently only support
//   upto 4 components.
struct PixelFormat {
  union {
    struct {
      uint8_t int_exp2 : 3;
      uint8_t is_signed : 1;
      uint8_t is_half : 1;
      uint8_t is_single : 1;
      uint8_t ncomp : 2;
    };
    uint8_t _raw;
  };
  constexpr PixelFormat(uint8_t raw) : _raw(raw) {}
  constexpr PixelFormat() : _raw() {}
  PixelFormat(const PixelFormat&) = default;
  PixelFormat(PixelFormat&&) = default;
  PixelFormat& operator=(const PixelFormat&) = default;
  PixelFormat& operator=(PixelFormat&&) = default;

  constexpr uint32_t get_ncomp() const { return ncomp + 1; }
  constexpr uint32_t get_fmt_size() const {
    uint32_t comp_size = 0;
    if (is_single) {
      comp_size = sizeof(float);
    } else if (is_half) {
      comp_size = sizeof(uint16_t);
    } else {
      comp_size = 4 << (int_exp2) >> 3;
    }
    return get_ncomp() * comp_size;
  }
  // Extract a real number from the buffer.
  inline float extract(const void* buf, size_t i, uint32_t comp) const {
    if (comp > ncomp) { return 0.f; }
    if (is_single) {
      return ((const float*)buf)[i * get_ncomp() + comp];
    } else if (is_half) {
      throw std::logic_error("not implemented yet");
    } else if (is_signed) {
      switch (int_exp2) {
      case 1:
        return ((const int8_t*)buf)[i * get_ncomp() + comp] / 128.f;
      case 2:
        return ((const int16_t*)buf)[i * get_ncomp() + comp] / 32768.f;
      case 3:
        return ((const int32_t*)buf)[i * get_ncomp() + comp] / 2147483648.f;
      }
    } else {
      switch (int_exp2) {
      case 1:
        return ((const uint8_t*)buf)[i * get_ncomp() + comp] / 255.f;
      case 2:
        return ((const uint16_t*)buf)[i * get_ncomp() + comp] / 65535.f;
      case 3:
        return ((const uint32_t*)buf)[i * get_ncomp() + comp] / 4294967296.f;
      }
    }
    panic("unsupported framebuffer format");
    return 0.0f;
  }
  // Extract a 32-bit word from the buffer as an integer. If the format is
  // shorter than 32 bits zeros are padded from MSB.
  inline uint32_t extract_word(const void* buf, size_t i, uint32_t comp) const {
    assert(!is_single & !is_half,
      "real number type cannot be extracted as bitfield");
    switch (int_exp2) {
    case 1:
      return ((const uint8_t*)buf)[i * get_ncomp() + comp];
    case 2:
      return ((const uint16_t*)buf)[i * get_ncomp() + comp];
    case 3:
      return ((const uint32_t*)buf)[i * get_ncomp() + comp];
    }
    panic("unsupported framebuffer format");
    return 0;
  }
  constexpr bool operator==(const PixelFormat& b) const {
    return _raw == b._raw;
  }
};
#define L_MAKE_VEC(ncomp, fmt)                                                 \
  ((uint8_t)((ncomp - 1) << 6) | (uint8_t)fmt)
#define L_DEF_FMT(name, ncomp, fmt)                                            \
  constexpr PixelFormat L_FORMAT_##name = PixelFormat(L_MAKE_VEC(ncomp, fmt))
L_DEF_FMT(UNDEFINED, 1, 0x00);

L_DEF_FMT(R8_UNORM, 1, 0x01);
L_DEF_FMT(R8G8_UNORM, 2, 0x01);
L_DEF_FMT(R8G8B8_UNORM, 3, 0x01);
L_DEF_FMT(R8G8B8A8_UNORM, 4, 0x01);

L_DEF_FMT(R8_UINT, 1, 0x01);
L_DEF_FMT(R8G8_UINT, 2, 0x01);
L_DEF_FMT(R8G8B8_UINT, 3, 0x01);
L_DEF_FMT(R8G8B8A8_UINT, 4, 0x01);

L_DEF_FMT(R16_UINT, 1, 0x02);
L_DEF_FMT(R16G16_UINT, 2, 0x02);
L_DEF_FMT(R16G16B16_UINT, 3, 0x02);
L_DEF_FMT(R16G16B16A16_UINT, 4, 0x02);

L_DEF_FMT(R32_UINT, 1, 0x03);
L_DEF_FMT(R32G32_UINT, 2, 0x03);
L_DEF_FMT(R32G32B32_UINT, 3, 0x03);
L_DEF_FMT(R32G32B32A32_UINT, 4, 0x03);

L_DEF_FMT(R32_SFLOAT, 1, 0x20);
L_DEF_FMT(R32G32_SFLOAT, 2, 0x20);
L_DEF_FMT(R32G32B32_SFLOAT, 3, 0x20);
L_DEF_FMT(R32G32B32A32_SFLOAT, 4, 0x20);
#undef L_DEF_FMT
#undef L_MAKE_VEC



enum BufferUsageBits {
  L_BUFFER_USAGE_STAGING_BIT = (1 << 0),
  L_BUFFER_USAGE_UNIFORM_BIT = (1 << 1),
  L_BUFFER_USAGE_STORAGE_BIT = (1 << 2),
  L_BUFFER_USAGE_VERTEX_BIT = (1 << 3),
  L_BUFFER_USAGE_INDEX_BIT = (1 << 4),
};
typedef uint32_t BufferUsage;
// Describes a buffer.
struct BufferConfig {
  // Human-readable label of the buffer.
  std::string label;
  MemoryAccess host_access;
  MemoryAccess dev_access;
  // Size of buffer allocation, or minimal size of buffer allocation if the
  // buffer has variable size. MUST NOT be zero.
  size_t size;
  // Buffer base address alignment requirement. Zero is treated as one in this
  // field.
  size_t align;
  // Usage of the buffer.
  BufferUsage usage;
};
L_IMPL_STRUCT struct Buffer;
L_IMPL_FN Buffer create_buf(const Context& ctxt, const BufferConfig& buf_cfg);
L_IMPL_FN void destroy_buf(Buffer& buf);
L_IMPL_FN const BufferConfig& get_buf_cfg(const Buffer& buf);

struct BufferView {
  const Buffer* buf; // Lifetime bound.
  size_t offset;
  size_t size;
};

L_IMPL_FN void map_buf_mem(
  const BufferView& dst,
  MemoryAccess map_access,
  void*& mapped
);
L_IMPL_FN void unmap_buf_mem(
  const BufferView& buf,
  void* mapped
);



enum ImageUsageBits {
  L_IMAGE_USAGE_NONE = 0,
  L_IMAGE_USAGE_STAGING_BIT = (1 << 0),
  L_IMAGE_USAGE_SAMPLED_BIT = (1 << 1),
  L_IMAGE_USAGE_STORAGE_BIT = (1 << 2),
  L_IMAGE_USAGE_ATTACHMENT_BIT = (1 << 3),
  L_IMAGE_USAGE_PRESENT_BIT = (1 << 4),
};
typedef uint32_t ImageUsage;
// Describe a row-major 2D image.
struct ImageConfig {
  // Human-readable label of the image.
  std::string label;
  MemoryAccess host_access;
  MemoryAccess dev_access;
  // Number of rows in the image.
  size_t nrow;
  // Number of columns in the image.
  size_t ncol;
  // Pixel format of the image.
  PixelFormat fmt;
  // Usage of the image.
  ImageUsage usage;
};
L_IMPL_STRUCT struct Image;
L_IMPL_FN Image create_img(const Context& ctxt, const ImageConfig& img_cfg);
L_IMPL_FN void destroy_img(Image& img);
L_IMPL_FN const ImageConfig& get_img_cfg(const Image& img);

struct ImageView {
  const Image* img; // Lifetime bound.
  uint32_t row_offset;
  uint32_t col_offset;
  uint32_t nrow;
  uint32_t ncol;
};

L_IMPL_FN void map_img_mem(
  const ImageView& img,
  MemoryAccess map_access,
  void*& mapped,
  size_t& row_pitch
);
L_IMPL_FN void unmap_img_mem(
  const ImageView& img,
  void* mapped
);



L_IMPL_STRUCT struct RenderPass;
L_IMPL_FN RenderPass create_pass(
  const Context& ctxt,
  const Image& img
);
L_IMPL_FN void destroy_pass(RenderPass& pass);



struct DispatchSize {
  uint32_t x, y, z;
};
enum ResourceType {
  L_RESOURCE_TYPE_UNIFORM_BUFFER,
  L_RESOURCE_TYPE_STORAGE_BUFFER,
  L_RESOURCE_TYPE_SAMPLED_IMAGE,
  L_RESOURCE_TYPE_STORAGE_IMAGE,
};
// A device program to be feeded in a `Transaction`.
struct ComputeTaskConfig {
  // Human-readable label of the task.
  std::string label;
  // Name of the entry point. Ignored if the platform does not require an entry
  // point name.
  std::string entry_name;
  // Code of the task program; will not be copied to the created `Task`.
  // Accepting SPIR-V for Vulkan.
  const void* code;
  // Size of code of the task program in bytes.
  size_t code_size;
  // The resources to be allocated.
  const ResourceType* rsc_tys;
  // Number of resources to be allocated.
  size_t nrsc_ty;
  // Local group size; number of threads in a workgroup.
  DispatchSize workgrp_size;
};
enum Topology {
  L_TOPOLOGY_POINT = 1,
  L_TOPOLOGY_LINE = 2,
  L_TOPOLOGY_TRIANGLE = 3,
};
enum VertexInputRate {
  L_VERTEX_INPUT_RATE_VERTEX,
  L_VERTEX_INPUT_RATE_INSTANCE,
};
struct VertexInput {
  PixelFormat fmt;
  VertexInputRate rate;
};
struct GraphicsTaskConfig {
  // Human-readable label of the task.
  std::string label;
  // Name of the vertex stage entry point. Ignored if the platform does not
  // require an entry point name.
  std::string vert_entry_name;
  // Code of the vertex stage of the task program; will not be copied to the
  // created `Task`. Accepting SPIR-V for Vulkan.
  const void* vert_code;
  // Size of code of the vertex stage of the task program in bytes.
  size_t vert_code_size;
  // Name of the fragment stage entry point. Ignored if the platform does not
  // require an entry point name.
  std::string frag_entry_name;
  // Code of the fragment stage of the task program; will not be copied to the
  // created `Task`. Accepting SPIR-V for Vulkan.
  const void* frag_code;
  // Size of code of the fragment stage of the task program in bytes.
  size_t frag_code_size;
  // Topology of vertex inputs to be assembled.
  Topology topo;
  // Vertex vertex input specifications.
  const VertexInput* vert_inputs;
  // Number of vertex inputs.
  size_t nvert_input;
  // Resources to be allocated.
  const ResourceType* rsc_tys;
  // Number of resources allocated.
  size_t nrsc_ty;
};
L_IMPL_STRUCT struct Task;
L_IMPL_FN Task create_comp_task(
  const Context& ctxt,
  const ComputeTaskConfig& cfg
);
L_IMPL_FN Task create_graph_task(
  const RenderPass& pass,
  const GraphicsTaskConfig& cfg
);
L_IMPL_FN void destroy_task(Task& task);



L_IMPL_STRUCT struct ResourcePool;
L_IMPL_FN ResourcePool create_rsc_pool(
  const Context& ctxt,
  const Task& task
);
L_IMPL_FN void destroy_rsc_pool(ResourcePool& rsc_pool);
L_IMPL_FN void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const BufferView& buf_view
);
L_IMPL_FN void bind_pool_rsc(
  ResourcePool& rsc_pool,
  uint32_t idx,
  const ImageView& img_view
);



struct Command;
// A wrapping of reusable commands. The resources used in commands of a
// transaction MUST be kept alive during transaction's entire lifetime. A
// transaction MUST NOT inline another transaction in its commands.
L_IMPL_STRUCT struct Transaction;
L_IMPL_FN Transaction create_transact(
  const std::string& label,
  const Context& ctxt,
  const Command* cmds,
  size_t ncmd
);
L_IMPL_FN void destroy_transact(Transaction& transact);



// A timestamp object written by the GPU, usually for precise timing.
L_IMPL_STRUCT struct Timestamp;
L_IMPL_FN Timestamp create_timestamp(const Context& ctxt);
L_IMPL_FN void destroy_timestamp(Timestamp& timestamp);
L_IMPL_FN double get_timestamp_result_us(const Timestamp& timestamp);


enum CommandType {
  L_COMMAND_TYPE_SET_SUBMIT_TYPE,
  L_COMMAND_TYPE_INLINE_TRANSACTION,
  L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE,
  L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER,
  L_COMMAND_TYPE_COPY_BUFFER,
  L_COMMAND_TYPE_COPY_IMAGE,
  L_COMMAND_TYPE_DISPATCH,
  L_COMMAND_TYPE_DRAW,
  L_COMMAND_TYPE_DRAW_INDEXED,
  L_COMMAND_TYPE_WRITE_TIMESTAMP,
  L_COMMAND_TYPE_BUFFER_BARRIER,
  L_COMMAND_TYPE_IMAGE_BARRIER,
  L_COMMAND_TYPE_BEGIN_RENDER_PASS,
  L_COMMAND_TYPE_END_RENDER_PASS,
};
enum SubmitType {
  L_SUBMIT_TYPE_COMPUTE,
  L_SUBMIT_TYPE_GRAPHICS,
  L_SUBMIT_TYPE_ANY = ~((uint32_t)0),
};
struct Command {
  CommandType cmd_ty;
  union {
    struct {
      SubmitType submit_ty;
    } cmd_set_submit_ty;
    struct {
      const Transaction* transact;
    } cmd_inline_transact;
    struct {
      BufferView src;
      ImageView dst;
    } cmd_copy_buf2img;
    struct {
      ImageView src;
      BufferView dst;
    } cmd_copy_img2buf;
    struct {
      BufferView src;
      BufferView dst;
    } cmd_copy_buf;
    struct {
      ImageView src;
      ImageView dst;
    } cmd_copy_img;
    struct {
      const Task* task;
      const ResourcePool* rsc_pool;
      DispatchSize nworkgrp;
    } cmd_dispatch;
    struct {
      const Task* task;
      const ResourcePool* rsc_pool;
      BufferView verts;
      uint32_t nvert;
      uint32_t ninst;
    } cmd_draw;
    struct {
      const Task* task;
      const ResourcePool* rsc_pool;
      BufferView verts;
      BufferView idxs;
      uint32_t nidx;
      uint32_t ninst;
    } cmd_draw_indexed;
    struct {
      const Timestamp* timestamp;
    } cmd_write_timestamp;
    struct {
      const Buffer* buf;
      MemoryAccess src_dev_access;
      MemoryAccess dst_dev_access;
      ImageUsage src_usage;
      ImageUsage dst_usage;
    } cmd_buf_barrier;
    struct {
      const Image* img;
      MemoryAccess src_dev_access;
      MemoryAccess dst_dev_access;
      ImageUsage src_usage;
      ImageUsage dst_usage;
    } cmd_img_barrier;
    struct {
      const RenderPass* pass;
      bool draw_inline;
    } cmd_begin_pass;
    struct {
      const RenderPass* pass;
    } cmd_end_pass;
  };
};

inline Command cmd_inline_transact(const Transaction& transact) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_INLINE_TRANSACTION;
  cmd.cmd_inline_transact.transact = &transact;
  return cmd;
}

// Copy data from a buffer to an image.
inline Command cmd_copy_buf2img(const BufferView& src, const ImageView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER_TO_IMAGE;
  cmd.cmd_copy_buf2img.src = src;
  cmd.cmd_copy_buf2img.dst = dst;
  return cmd;
}
// Copy data from an image to a buffer.
inline Command cmd_copy_img2buf(const ImageView& src, const BufferView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE_TO_BUFFER;
  cmd.cmd_copy_img2buf.src = src;
  cmd.cmd_copy_img2buf.dst = dst;
  return cmd;
}
// Copy data from a buffer to another buffer.
inline Command cmd_copy_buf(const BufferView& src, const BufferView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_BUFFER;
  cmd.cmd_copy_buf.src = src;
  cmd.cmd_copy_buf.dst = dst;
  return cmd;
}
// Copy data from an image to another image.
inline Command cmd_copy_img(const ImageView& src, const ImageView& dst) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_COPY_IMAGE;
  cmd.cmd_copy_img.src = src;
  cmd.cmd_copy_img.dst = dst;
  return cmd;
}

// Dispatch a task to the transaction.
inline Command cmd_dispatch(
  const Task& task,
  const ResourcePool& rsc_pool,
  DispatchSize nworkgrp
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_DISPATCH;
  cmd.cmd_dispatch.task = &task;
  cmd.cmd_dispatch.rsc_pool = &rsc_pool;
  cmd.cmd_dispatch.nworkgrp = nworkgrp;
  return cmd;
}

// Draw triangle lists, vertex by vertex.
inline Command cmd_draw(
  const Task& task,
  const ResourcePool& rsc_pool,
  const BufferView& verts,
  uint32_t nvert,
  uint32_t ninst
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_DRAW;
  cmd.cmd_draw.task = &task;
  cmd.cmd_draw.rsc_pool = &rsc_pool;
  cmd.cmd_draw.verts = verts;
  cmd.cmd_draw.nvert = nvert;
  cmd.cmd_draw.ninst = ninst;
  return cmd;
}
// Draw triangle lists, index by index, where each index points to a vertex. 
inline Command cmd_draw_indexed(
  const Task& task,
  const ResourcePool& rsc_pool,
  const BufferView& idxs,
  const BufferView& verts,
  uint32_t nidx,
  uint32_t ninst
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_DRAW_INDEXED;
  cmd.cmd_draw_indexed.task = &task;
  cmd.cmd_draw_indexed.rsc_pool = &rsc_pool;
  cmd.cmd_draw_indexed.verts = verts;
  cmd.cmd_draw_indexed.idxs = idxs;
  cmd.cmd_draw_indexed.nidx = nidx;
  cmd.cmd_draw_indexed.ninst = ninst;
  return cmd;
}

inline Command cmd_write_timestamp(const Timestamp& timestamp) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_WRITE_TIMESTAMP;
  cmd.cmd_write_timestamp.timestamp = &timestamp;
  return cmd;
}

inline Command cmd_set_submit_ty(SubmitType submit_ty) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_SET_SUBMIT_TYPE;
  cmd.cmd_set_submit_ty.submit_ty = submit_ty;
  return cmd;
}

inline Command cmd_buf_barrier(
  const Buffer& buf,
  BufferUsage src_usage,
  BufferUsage dst_usage,
  MemoryAccess src_dev_access,
  MemoryAccess dst_dev_access
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_BUFFER_BARRIER;
  cmd.cmd_buf_barrier.buf = &buf;
  cmd.cmd_buf_barrier.src_dev_access = src_dev_access;
  cmd.cmd_buf_barrier.dst_dev_access = dst_dev_access;
  cmd.cmd_buf_barrier.src_usage = src_usage;
  cmd.cmd_buf_barrier.dst_usage = dst_usage;
  return cmd;
}
inline Command cmd_img_barrier(
  const Image& img,
  ImageUsage src_usage,
  ImageUsage dst_usage,
  MemoryAccess src_dev_access,
  MemoryAccess dst_dev_access
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_IMAGE_BARRIER;
  cmd.cmd_img_barrier.img = &img;
  cmd.cmd_img_barrier.src_dev_access = src_dev_access;
  cmd.cmd_img_barrier.dst_dev_access = dst_dev_access;
  cmd.cmd_img_barrier.src_usage = src_usage;
  cmd.cmd_img_barrier.dst_usage = dst_usage;
  return cmd;
}
inline Command cmd_begin_pass(
  const RenderPass& pass,
  bool draw_inline
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_BEGIN_RENDER_PASS;
  cmd.cmd_begin_pass.pass = &pass;
  cmd.cmd_begin_pass.draw_inline = draw_inline;
  return cmd;
}
inline Command cmd_end_pass(
  const RenderPass& pass
) {
  Command cmd {};
  cmd.cmd_ty = L_COMMAND_TYPE_END_RENDER_PASS;
  cmd.cmd_end_pass.pass = &pass;
  return cmd;
}



L_IMPL_STRUCT struct CommandDrain;
L_IMPL_FN CommandDrain create_cmd_drain(const Context& ctxt);
L_IMPL_FN void destroy_cmd_drain(CommandDrain& cmd_drain);
// Submit commands and inlined transactions to the device for execution.
L_IMPL_FN void submit_cmds(
  CommandDrain& cmd_drain,
  const Command* cmds,
  size_t ncmd
);
// Wait until the command drain consumed all the commands and finished
// execution.
L_IMPL_FN void wait_cmd_drain(CommandDrain& cmd_drain);

namespace ext {

std::vector<uint8_t> load_code(const std::string& prefix);

} // namespace ext

} // namespace HAL_IMPL_NAMESPACE

} // namespace liong

#undef L_IMPL_FN
#undef L_IMPL_STRUCT
