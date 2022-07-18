#include "gft/vk.hpp"
#include "gft/hal/renderer.hpp"
#include "gft/glslang.hpp"
#include "glm/glm.hpp"
#include "glm/ext.hpp"


namespace liong {
namespace vk {
namespace scoped {

scoped::DepthImage create_zbuf(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) {
  scoped::DepthImage zbuf_img = ctxt.build_depth_img()
    .fmt(fmt::L_DEPTH_FORMAT_D16_UNORM)
    .attachment()
    .width(width)
    .height(height)
    .build();
  return zbuf_img;
}

scoped::RenderPass create_pass(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) {
  scoped::RenderPass pass = ctxt.build_pass()
    .clear_store_attm(fmt::L_FORMAT_B8G8R8A8_UNORM_PACK32)
    .clear_store_attm(fmt::L_DEPTH_FORMAT_D16_UNORM)
    .width(width)
    .height(height)
    .build();
  return pass;
}
scoped::Task create_unlit_task(const scoped::RenderPass& pass, Topology topo) {
  const char* vert_src = R"(
    #version 460 core

    layout(location=0) in vec3 pos;
    layout(location=0) out vec4 v_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
    };
    layout(binding=1, std430) readonly buffer Colors {
      vec4 colors[];
    };

    void main() {
      v_color = colors[gl_VertexIndex];
      gl_Position = world2view * model2world * vec4(pos, 1.0);
    }
  )";
  const char* frag_src = R"(
    #version 460 core
    precision mediump float;

    layout(location=0) in highp vec4 v_color;
    layout(location=0) out vec4 scene_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
    };

    layout(binding=3) uniform sampler2D main_tex;

    void main() {
      scene_color = v_color;
    }
  )";

  auto art = glslang::compile_graph(vert_src, "main", frag_src, "main");

  auto wireframe_task = pass.build_graph_task()
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .topo(topo)
    .build();
  return wireframe_task;
}
scoped::Task create_lit_task(const scoped::RenderPass& pass) {
  const char* vert_src = R"(
    #version 460 core

    layout(location=0) in vec3 pos;

    layout(location=0) out vec4 v_world_pos;
    layout(location=1) out vec2 v_uv;
    layout(location=2) out vec4 v_norm;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
      vec4 camera_pos;
      vec4 light_dir;
      vec4 ambient;
      vec4 albedo;
    };

    layout(binding=1, std430) readonly buffer Uvs {
      vec2 uvs[];
    };
    layout(binding=2, std430) readonly buffer Norms {
      vec4 norms[];
    };

    void main() {
      v_world_pos = model2world * vec4(pos, 1.0);
      v_uv = uvs[gl_VertexIndex];
      v_norm = model2world * norms[gl_VertexIndex];

      vec4 ndc_pos = world2view * v_world_pos;
      gl_Position = ndc_pos;
    }
  )";
  const char* frag_src = R"(
    #version 460 core
    precision mediump float;

    layout(location=0) in highp vec4 v_world_pos;
    layout(location=1) in highp vec2 v_uv;
    layout(location=2) in highp vec4 v_norm;

    layout(location=0) out vec4 scene_color;

    layout(binding=0, std140) uniform Uniform {
      mat4 model2world;
      mat4 world2view;
      vec4 camera_pos;
      vec4 light_dir;
      vec4 ambient;
      vec4 albedo;
    };

    layout(binding=3) uniform sampler2D main_tex;

    void main() {
      vec3 N = normalize(v_norm.xyz);
      vec3 V = normalize(camera_pos.xyz - v_world_pos.xyz);
      vec3 L = normalize(light_dir.xyz);
      vec3 H = normalize(V + L);
      float NoH = dot(N, H);

      vec3 diffuse = clamp(NoH, 0.0f, 1.0f) * texture(main_tex, v_uv).xyz;

      scene_color = vec4(albedo.xyz * diffuse.xyz + ambient.xyz, 1.0);
    }
  )";

  auto art = glslang::compile_graph(vert_src, "main", frag_src, "main");

  auto lit_task = pass.build_graph_task()
    .vert(art.vert_spv)
    .frag(art.frag_spv)
    .rsc(L_RESOURCE_TYPE_UNIFORM_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_STORAGE_BUFFER)
    .rsc(L_RESOURCE_TYPE_SAMPLED_IMAGE)
    .topo(L_TOPOLOGY_TRIANGLE)
    .build();
  return lit_task;
}

scoped::Image create_default_tex_img(
  const scoped::Context& ctxt
) {
  scoped::Image white_img = ctxt.build_img()
    .sampled()
    .width(4)
    .height(4)
    .fmt(fmt::L_FORMAT_R8G8B8A8_UNORM_PACK32)
    .build();

  std::vector<uint32_t> white_img_data(16, ~(uint32_t)0);
  scoped::Buffer staging_buf = ctxt.build_buf()
    .streaming_with(white_img_data)
    .build();

  ctxt.build_trans_invoke()
    .src(staging_buf.view())
    .dst(white_img.view())
    .build()
    .submit()
    .wait();

  return white_img;
}

Renderer::Renderer(
  const scoped::Context& ctxt,
  uint32_t width,
  uint32_t height
) :
  ctxt(scoped::Context::borrow(ctxt)),
  pass(create_pass(ctxt, width, height)),
  zbuf_img(create_zbuf(ctxt, width, height)),
  lit_task(create_lit_task(pass)),
  wireframe_task(create_unlit_task(pass, L_TOPOLOGY_TRIANGLE_WIREFRAME)),
  point_cloud_task(create_unlit_task(pass, L_TOPOLOGY_POINT)),
  default_tex_img(create_default_tex_img(ctxt)),
  width(width),
  height(height),
  camera_pos(0.0f, 0.0f, -10.0f),
  model_pos(0.0f, 0.0f, 0.0f),
  light_dir(0.5f, -1.0f, 1.0f),
  ambient(0.1f, 0.1f, 0.1f),
  albedo(1.0f, 0.1f, 1.0f),
  rpib(nullptr) {}

glm::mat4 Renderer::get_model2world() const {
  glm::mat4 model2world = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.0f, -1.0f, -1.0f));
  return model2world;
}
glm::mat4 Renderer::get_world2view() const {
  glm::mat4 camera2view = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1e-2f, 65534.0f);
  glm::mat4 world2camera = glm::lookAt(-camera_pos, model_pos, glm::vec3(0.0f, 1.0f, 0.0f));
  return camera2view * world2camera;
}

void Renderer::set_camera_pos(const glm::vec3& x) {
  camera_pos = x;
}
void Renderer::set_model_pos(const glm::vec3& x) {
  model_pos = x;
}

Renderer& Renderer::begin_frame(const scoped::Image& render_target_img) {
  push_gc_frame("renderer");
  rpib = std::make_unique<RenderPassInvocationBuilder>(pass.build_pass_invoke());
  rpib->attm(render_target_img.view()).attm(zbuf_img.view());
  return *this;
}
void Renderer::end_frame() {
  rpib->build()
    .submit()
    .wait();
  pop_gc_frame("renderer");
  pass.build_pass_invoke();
}

Renderer& Renderer::draw_mesh(const mesh::Mesh& mesh) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
    glm::vec4 camera_pos;
    glm::vec4 light_dir;
    glm::vec4 ambient;
    glm::vec4 albedo;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();
  u.camera_pos = glm::vec4(camera_pos, 1.0f);
  u.light_dir = glm::vec4(light_dir, 0.0f);
  u.ambient = glm::vec4(ambient, 1.0f);
  u.albedo = glm::vec4(albedo, 1.0f);

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer uv_buf = ctxt.build_buf()
    .storage()
    .streaming_with(mesh.uvs)
    .build();

  scoped::Buffer norm_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(mesh.norms, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = lit_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(mesh.poses.size())
    .rsc(uniform_buf.view())
    .rsc(uv_buf.view())
    .rsc(norm_buf.view())
    .rsc(default_tex_img.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_idxmesh(const mesh::IndexedMesh& idxmesh) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
    glm::vec4 camera_pos;
    glm::vec4 light_dir;
    glm::vec4 ambient;
    glm::vec4 albedo;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();
  u.camera_pos = glm::vec4(camera_pos, 1.0f);
  u.light_dir = glm::vec4(light_dir, 0.0f);
  u.ambient = glm::vec4(ambient, 1.0f);
  u.albedo = glm::vec4(albedo, 1.0f);

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(idxmesh.mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer uv_buf = ctxt.build_buf()
    .storage()
    .streaming_with(idxmesh.mesh.uvs)
    .build();

  scoped::Buffer norm_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(idxmesh.mesh.norms, sizeof(glm::vec4))
    .build();

  scoped::Buffer idxs_buf = ctxt.build_buf()
    .index()
    .streaming_with(idxmesh.idxs)
    .build();

  scoped::Invocation lit_invoke = lit_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .idx_buf(idxs_buf.view())
    .idx_ty(L_INDEX_TYPE_UINT32)
    .nidx(idxmesh.idxs.size() * 3)
    .rsc(uniform_buf.view())
    .rsc(uv_buf.view())
    .rsc(norm_buf.view())
    .rsc(default_tex_img.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}

Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = wireframe_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(mesh.poses.size())
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh, const glm::vec3& color) {
  std::vector<glm::vec3> colors(mesh.poses.size(), color);
  return draw_mesh_wireframe(mesh, colors);
}
Renderer& Renderer::draw_mesh_wireframe(const mesh::Mesh& mesh) {
  return draw_mesh_wireframe(mesh, glm::vec3(1.0f, 1.0f, 0.0f));
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(idxmesh.mesh.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer idxs_buf = ctxt.build_buf()
    .index()
    .streaming_with(idxmesh.idxs)
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation lit_invoke = wireframe_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .idx_buf(idxs_buf.view())
    .idx_ty(L_INDEX_TYPE_UINT32)
    .nidx(idxmesh.idxs.size() * 3)
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(lit_invoke);
  return *this;
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh, const glm::vec3& color) {
  std::vector<glm::vec3> colors(idxmesh.mesh.poses.size(), color);
  return draw_idxmesh_wireframe(idxmesh, colors);
}
Renderer& Renderer::draw_idxmesh_wireframe(const mesh::IndexedMesh& idxmesh) {
  return draw_idxmesh_wireframe(idxmesh, glm::vec3(1.0f, 1.0f, 0.0f));
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud, const std::vector<glm::vec3>& colors) {
  struct Uniform {
    glm::mat4 model2world;
    glm::mat4 world2view;
  };
  Uniform u;
  u.model2world = get_model2world();
  u.world2view = get_world2view();

  scoped::Buffer uniform_buf = ctxt.build_buf()
    .uniform()
    .streaming_with(u)
    .build();

  scoped::Buffer poses_buf = ctxt.build_buf()
    .vertex()
    .streaming_with_aligned(point_cloud.poses, sizeof(glm::vec4))
    .build();

  scoped::Buffer colors_buf = ctxt.build_buf()
    .storage()
    .streaming_with_aligned(colors, sizeof(glm::vec4))
    .build();

  scoped::Invocation invoke = point_cloud_task.build_graph_invoke()
    .vert_buf(poses_buf.view())
    .nvert(point_cloud.poses.size())
    .rsc(uniform_buf.view())
    .rsc(colors_buf.view())
    .build();

  rpib->invoke(invoke);
  return *this;
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud, const glm::vec3& color) {
  std::vector<glm::vec3> colors(point_cloud.poses.size(), color);
  return draw_point_cloud(point_cloud, colors);
}
Renderer& Renderer::draw_point_cloud(const mesh::PointCloud& point_cloud) {
  return draw_point_cloud(point_cloud, glm::vec3(1.0f, 1.0f, 0.0f));
}

} // namespace scoped
} // namespace vk
} // namespace liong
