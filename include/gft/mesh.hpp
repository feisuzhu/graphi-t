// # 3D Mesh utilities
// @PENGUINLIONG
#pragma once
#include <array>
#include <vector>
#include <string>
#include <set>
#include "glm/glm.hpp"
#include "gft/geom.hpp"

namespace liong {
namespace mesh {

struct Mesh {
  std::vector<glm::vec3> poses;
  std::vector<glm::vec2> uvs;
  std::vector<glm::vec3> norms;

  static Mesh from_tris(const geom::Triangle* tris, size_t ntri);
  inline static Mesh from_tris(const std::vector<geom::Triangle>& tris) {
    return from_tris(tris.data(), tris.size());
  }
  std::vector<geom::Triangle> to_tris() const;

  geom::Aabb aabb() const;
};

extern bool try_parse_obj(const std::string& obj, Mesh& mesh);
extern Mesh load_obj(const char* path);



struct IndexedMesh {
  Mesh mesh;
  std::vector<glm::uvec3> idxs;

  static IndexedMesh from_mesh(const Mesh& mesh);

  inline geom::Aabb aabb() const { return mesh.aabb(); }
};



struct PointCloud {
  std::vector<glm::vec3> poses;

  geom::Aabb aabb() const;
};


struct Grid {
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
};

extern Grid build_grid(const geom::Aabb& aabb, const glm::uvec3& grid_res);
extern Grid build_grid(const geom::Aabb& aabb, const glm::vec3& grid_interval);

struct Bin {
  geom::Aabb aabb;
  std::vector<uint32_t> iprims;
};
struct BinGrid {
  Grid grid;
  std::vector<Bin> bins;

  inline std::vector<geom::Aabb> to_aabbs() const {
    std::vector<geom::Aabb> out;
    for (const auto& bin : bins) {
      out.emplace_back(bin.aabb);
    }
    return out;
  }
};

extern BinGrid bin_point_cloud(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const PointCloud& point_cloud
);
extern BinGrid bin_point_cloud(
  const glm::vec3& grid_interval,
  const PointCloud& point_cloud
);

extern BinGrid bin_mesh(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const Mesh& mesh
);
extern BinGrid bin_mesh(
  const glm::vec3& grid_interval,
  const Mesh& mesh
);

extern BinGrid bin_idxmesh(
  const geom::Aabb& aabb,
  const glm::uvec3& grid_res,
  const IndexedMesh& idxmesh
);
extern BinGrid bin_idxmesh(
  const glm::vec3& grid_interval,
  const IndexedMesh& idxmesh
);



struct TetrahedralVertex {
  glm::vec3 pos;
  // Indices to adjacent cells.
  std::set<uint32_t> ineighbor_cells;
  // Indices to adjacent vertices.
  std::set<uint32_t> ineighbor_verts;
};
struct TetrahedralCell {
  glm::uvec4 itetra_verts;
};
struct TetrahedralInterpolant {
  // Indices to tettrahedron vertices.
  uint32_t itetra_cell;
  // Barycentric weights of tetrahedron vertices.
  glm::vec4 tetra_weights;
};
struct TetrahedralMesh {
  // Per tetrahedral mesh vertex.
  std::vector<TetrahedralVertex> tetra_verts;
  // Per tetrahedral mesh cell.
  std::vector<TetrahedralCell> tetra_cells;
  // Per triangle mesh vertex.
  std::vector<TetrahedralInterpolant> interps;

  static TetrahedralMesh from_points(const glm::vec3& grid_interval, const std::vector<glm::vec3>& points);
  std::vector<glm::vec3> to_points() const;

  void apply_trans(const glm::mat4& trans);

  std::vector<geom::Tetrahedron> to_tetras() const;
  Mesh to_mesh() const;
};


} // namespace mesh
} // namespace liong
