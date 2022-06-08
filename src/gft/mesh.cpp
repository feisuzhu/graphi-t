// # 3D Mesh utilities
// @PENGUINLIONG
#include <cstdint>
#include <set>
#include <initializer_list>
#include "glm/glm.hpp"
#include "gft/mesh.hpp"
#include "gft/assert.hpp"
#include "gft/log.hpp"

namespace liong {
namespace mesh {

using namespace geom;

enum ObjTokenType {
  L_OBJ_TOKEN_TYPE_NEWLINE,
  L_OBJ_TOKEN_TYPE_VERB,
  L_OBJ_TOKEN_TYPE_TEXT,
  L_OBJ_TOKEN_TYPE_INTEGER,
  L_OBJ_TOKEN_TYPE_NUMBER,
  L_OBJ_TOKEN_TYPE_SLASH,
  L_OBJ_TOKEN_TYPE_END,
};
struct ObjToken {
  ObjTokenType token_ty;
  std::string token_word;
  uint32_t token_integer;
  float token_number;
};

struct ObjTokenizer {
  const char* pos;
  const char* end;
  bool success;
  ObjToken token;

  ObjTokenizer(const char* beg, const char* end) :
    pos(beg), end(end), success(true), token()
  {
    next();
  }

  bool try_tokenize(ObjToken& token) {
    bool is_in_token = false;
    bool is_last_token_newline = token.token_ty == L_OBJ_TOKEN_TYPE_NEWLINE;
    ObjTokenType token_ty;
    std::string buf {};

    for (;;) {
      if (is_in_token) {
        // It's already been parsing an multi-char token, can be a verb,
        // interger or a real number.
        bool should_break_token = pos == end;

        if (!should_break_token) {
          const char c = *pos;
          should_break_token |= c == ' ' || c == '\t' || c == '\r' ||
            c == '\n' || c == '/' || c == '#';
        }

        if (should_break_token) {
          // Punctuation breaks tokens, but don't step forward `pos` here, the
          // newlines need to be tokenized later.
          token.token_ty = token_ty;
          switch (token_ty) {
            case L_OBJ_TOKEN_TYPE_VERB:
            case L_OBJ_TOKEN_TYPE_TEXT:
              token.token_word = std::move(buf);
              break;
            case L_OBJ_TOKEN_TYPE_INTEGER:
              token.token_integer = std::atoi(buf.c_str());
              break;
            case L_OBJ_TOKEN_TYPE_NUMBER:
              token.token_number = std::atof(buf.c_str());
              break;
            default: unreachable();
          }
          is_in_token = false;
          return true;

        }

        const char c = *pos;

        switch (token_ty) {
        case L_OBJ_TOKEN_TYPE_INTEGER:
          if (c == '.') {
            // Found a fraction point so promote the integer into a floating-
            // point number.
            token_ty = L_OBJ_TOKEN_TYPE_NUMBER;
            break;
          } else if (c >= '0' && c <= '9') {
            break;
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
        case L_OBJ_TOKEN_TYPE_NUMBER:
          if (c >= '0' && c <= '9') {
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
          break;
        case L_OBJ_TOKEN_TYPE_VERB:
          if (c >= 'a' && c <= 'z') {
            break;
          } else if (c != '\0' && c < 128) {
            token_ty = L_OBJ_TOKEN_TYPE_TEXT;
            break;
          } else {
            return false;
          }
        case L_OBJ_TOKEN_TYPE_TEXT:
          if (c != '\0' && c < 128) {
            break;
          } else {
            return false;
          }
        default: unreachable();
        }
        buf.push_back(c);

      } else {
        // It's not parsing a token.
        if (pos == end) {
          // End of input.
          token.token_ty = L_OBJ_TOKEN_TYPE_END;
          return true;
        }

        char c = *pos;

        if (c == '\r' || c == '\n') {
          // Newlines.
          ++pos;
          token.token_ty = L_OBJ_TOKEN_TYPE_NEWLINE;
          return true;
        }
        if (c == '#') {
          // Skip comments.
          while (pos != end && *(++pos) != '\n');
          token.token_ty = L_OBJ_TOKEN_TYPE_NEWLINE;
          return true;
        }
        if (c == '/') {
          ++pos;
          token.token_ty = L_OBJ_TOKEN_TYPE_SLASH;
          return true;
        }

        // Ignore white spaces.
        while (c == ' ' || c == '\t') {
          ++pos; c = *pos;
        }

        if (c == '-' || (c >= '0' && c <= '9')) {
          token_ty = L_OBJ_TOKEN_TYPE_INTEGER;
          buf.push_back(c);
          is_in_token = true;
        } else if (is_last_token_newline && (c >= 'a' && c <= 'z')) {
          token_ty = L_OBJ_TOKEN_TYPE_VERB;
          buf.push_back(c);
          is_in_token = true;
        } else if (c != '\0' && c < 128) {
          // Any other visible characters.
          token_ty = L_OBJ_TOKEN_TYPE_TEXT;
          buf.push_back(c);
          is_in_token = true;
        } else {
          unreachable();
        }
      }
      ++pos;

    }
  }

  bool try_next() {
    return try_tokenize(token);
  }
  void next() {
    success &= try_next();
  }

  bool try_verb(std::string& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_VERB) {
      out = token.token_word;
      next();
      return true;
    }
    return false;
  }
  void verb(std::string& out) {
    success &= try_verb(out);
  }
  bool try_integer(uint32_t& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_INTEGER) {
      out = token.token_integer;
      next();
      return true;
    }
    return false;
  }
  void integer(uint32_t& out) {
    success &= try_integer(out);
  }
  bool try_number(float& out) {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_INTEGER) {
      out = token.token_integer;
      next(); 
      return true;
    } else if (token.token_ty == L_OBJ_TOKEN_TYPE_NUMBER) {
      out = token.token_number;
      next();
      return true;
    }
    return false;
  }
  void number(float& out) {
    success &= try_number(out);
  }
  bool try_slash() {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_SLASH) {
      next();
      return true;
    }
    return false;
  }
  bool try_newline() {
    if (token.token_ty == L_OBJ_TOKEN_TYPE_NEWLINE) {
      next();
      return true;
    }
    return false;
  }
  bool try_end() {
    if (pos == end) {
      return true;
    }
    if (token.token_ty == L_OBJ_TOKEN_TYPE_END) {
      next();
      return true;
    }
    return false;
  }
};

struct ObjParser {
  ObjTokenizer tokenizer;
  std::string verb;
  ObjParser(const char* beg, const char* end) : tokenizer(beg, end) {}

  bool try_parse(Mesh& out) {
    Mesh mesh {};
    std::set<std::string> unknown_verbs;

    std::vector<glm::vec3> poses;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> norms;
    uint32_t counter = 0;

    while (tokenizer.success) {
      if (tokenizer.try_verb(verb)) {
        if (tokenizer.token.token_word == "v") {
          glm::vec3 pos{};
          float w;
          tokenizer.number(pos.x);
          tokenizer.number(pos.y);
          tokenizer.number(pos.z);
          tokenizer.try_number(w);
          tokenizer.try_newline();
          poses.emplace_back(std::move(pos));
          continue;
        } else if (tokenizer.token.token_word == "vt") {
          glm::vec2 uv{};
          float w;
          tokenizer.number(uv.x);
          tokenizer.try_number(uv.y);
          tokenizer.try_number(w);
          tokenizer.try_newline();
          uvs.emplace_back(std::move(uv));
          continue;
        } else if (tokenizer.token.token_word == "vn") {
          glm::vec3 norm{};
          tokenizer.number(norm.x);
          tokenizer.number(norm.y);
          tokenizer.number(norm.z);
          tokenizer.try_newline();
          norms.emplace_back(std::move(norm));
          continue;
        } else if (tokenizer.token.token_word == "f") {
          uint32_t ipos;
          uint32_t iuv;
          uint32_t inorm;
          for (uint32_t i = 0; i < 3; ++i) {
            tokenizer.integer(ipos);
            mesh.poses.emplace_back(poses.at(ipos - 1));
            if (tokenizer.try_slash()) {
              if (tokenizer.try_integer(iuv)) {
                mesh.uvs.emplace_back(uvs.at(iuv - 1));
              }
            }
            if (tokenizer.try_slash()) {
              if (tokenizer.try_integer(inorm)) {
                mesh.norms.emplace_back(norms.at(inorm - 1));
              }
            }
          }
          tokenizer.try_newline();
          continue;
        }

        // We don't know this verb, report the case and skip this line.
        {
          if (unknown_verbs.find(verb) == unknown_verbs.end()) {
            unknown_verbs.emplace(verb);
            log::warn("unknown obj verb '", verb, "' is ignored");
          }
          while (tokenizer.success) {
            if (tokenizer.try_newline() || tokenizer.try_end()) {
              break;
            } else {
              tokenizer.next();
            }
          }
        }
      }

      if (tokenizer.try_newline()) {
        // Ignore empty newlines.
        continue;
      } else if (tokenizer.try_end()) {
        if (mesh.uvs.size() == 0) {
          log::warn("uv data is not available, filled with zeroes instead");
          mesh.uvs.resize(mesh.poses.size());
        } else if (mesh.uvs.size() != mesh.poses.size()) {
          log::warn("uv count mismatches position count; treated as error");
          return false;
        }
        if (mesh.norms.size() == 0) {
          log::warn("normal data is not available, filled with zeroes instead");
          mesh.norms.resize(mesh.poses.size());
        } else if (mesh.norms.size() != mesh.poses.size()) {
          log::warn("normal count mismatches position count; treated as error");
          return false;
        }
        out = std::move(mesh);
        return true;
      }
    }
    return false;
  }
};



bool try_parse_obj(const std::string& obj, Mesh& mesh) {
  ObjParser parser(obj.data(), obj.data() + obj.size());
  return parser.try_parse(mesh);
}
Mesh load_obj(const char* path) {
  auto txt = util::load_text(path);
  Mesh mesh{};
  L_ASSERT(try_parse_obj(txt, mesh));
  return mesh;
}



Mesh Mesh::from_tris(const Triangle* tris, size_t ntri) {
  Mesh out {};
  out.poses.reserve(ntri * 3);
  out.uvs.resize(ntri * 3);
  out.norms.resize(ntri * 3);

  for (size_t i = 0; i < ntri; ++i) {
    const Triangle& tri = tris[i];
    out.poses.emplace_back(tri.a);
    out.poses.emplace_back(tri.b);
    out.poses.emplace_back(tri.c);
  }
  return out;
}
std::vector<Triangle> Mesh::to_tris() const {
  std::vector<Triangle> out {};
  size_t ntri = poses.size() / 3;
  L_ASSERT(ntri * 3 == poses.size());
  out.reserve(ntri);
  for (size_t i = 0; i < poses.size(); i += 3) {
    Triangle tri {
      poses.at(i),
      poses.at(i + 1),
      poses.at(i + 2),
    };
    out.emplace_back(std::move(tri));
  }
  return out;
}


Aabb Mesh::aabb() const {
  return Aabb::from_points(poses.data(), poses.size());
}



struct UniqueVertex {
  glm::vec3 pos;
  glm::vec2 uv;
  glm::vec3 norm;

  friend bool operator<(const UniqueVertex& a, const UniqueVertex& b) {
    return std::memcmp(&a, &b, sizeof(UniqueVertex)) < 0;
  }
};
IndexedMesh IndexedMesh::from_mesh(const Mesh& mesh) {
  IndexedMesh out{};
  std::map<UniqueVertex, uint32_t> vert2idx;

  uint32_t ntri = mesh.poses.size() / 3;
  if (ntri * 3 != mesh.poses.size()) {
    log::warn("mesh vertex number is not aligned to 3; trailing vertices are "
      "ignored because they don't form an actual triangle");
  }

  for (uint32_t i = 0; i < ntri; ++i) {
    uint32_t idxs[3];
    for (uint32_t j = 0; j < 3; ++j) {
      uint32_t ivert = i * 3 + j;
      UniqueVertex vert;
      vert.pos = mesh.poses.at(ivert);
      vert.uv = mesh.uvs.at(ivert);
      vert.norm = mesh.norms.at(ivert);

      uint32_t idx;
      auto it = vert2idx.find(vert);
      if (it == vert2idx.end()) {
        idx = vert2idx.size();
        out.mesh.poses.emplace_back(vert.pos);
        out.mesh.uvs.emplace_back(vert.uv);
        out.mesh.norms.emplace_back(vert.norm);
        vert2idx.insert(it, std::make_pair(std::move(vert), idx));
      } else {
        idx = it->second;
      }
      idxs[j] = idx;
    }
    out.idxs.emplace_back(glm::uvec3 { idxs[0], idxs[1], idxs[2] });
  }

  return out;
}



Aabb PointCloud::aabb() const {
  return geom::Aabb::from_points(poses);
}



struct Binner {
  Aabb aabb;
  glm::uvec3 grid_res;
  // `aabb` range divided by `grid_res` except for the exactly `min` vlaues.
  std::vector<float> grid_lines_x;
  std::vector<float> grid_lines_y;
  std::vector<float> grid_lines_z;
  std::vector<Bin> bins;
  uint32_t counter;

  static std::vector<float> make_grid_lines(float min, float max, uint32_t n) {
    float range = max - min;
    std::vector<float> out;
    for (uint32_t i = 1; i < n; ++i) {
      out.emplace_back((i / float(n)) * range + min);
    }
    out.emplace_back(max);
    return out;
  }

  Binner(const Aabb& aabb, const glm::uvec3& grid_res) :
    aabb(aabb),
    grid_res(grid_res),
    grid_lines_x(make_grid_lines(aabb.min.x, aabb.max.x, grid_res.x)),
    grid_lines_y(make_grid_lines(aabb.min.y, aabb.max.y, grid_res.y)),
    grid_lines_z(make_grid_lines(aabb.min.z, aabb.max.z, grid_res.z)),
    bins(),
    counter()
  {
    bins.reserve(grid_res.x * grid_res.y * grid_res.z);
    for (uint32_t z = 0; z < grid_res.z; ++z) {
      float z_min = z == 0 ? aabb.min.z : grid_lines_z.at(z - 1);
      float z_max = grid_lines_z.at(z);
      for (uint32_t y = 0; y < grid_res.y; ++y) {
        float y_min = y == 0 ? aabb.min.y : grid_lines_y.at(y - 1);
        float y_max = grid_lines_y.at(y);
        for (uint32_t x = 0; x < grid_res.x; ++x) {
          float x_min = x == 0 ? aabb.min.x : grid_lines_x.at(x - 1);
          float x_max = grid_lines_x.at(x);

          glm::vec3 min(x_min, y_min, z_min);
          glm::vec3 max(x_max, y_max, z_max);
          Aabb aabb2 { min, max };

          Bin bin { aabb2, {} };
          bins.emplace_back(bin);
        }
      }
    }
  }

  size_t get_ibin(const std::vector<float>& grid_lines, float x) {
    size_t i = 0;
    // The loop breaks when `x` is less than `grid_lines` so `i` ended at any
    // index of the left-close and right-open interval that contains the point.
    // But also note that if the loop runs out of range, i.e., `x >= aabb.max`
    // the index of the farthest bin is naturally assigned to `i`. Given that
    // non-intersecting triangles are filetered in `bin` any point enclosed
    // by `aabb` can be uniquely assigned to a bin at boundaries.
    for (; i < grid_lines.size() - 1; ++i) {
      if (x < grid_lines.at(i)) { break; }
    }
    return i;
  }

  bool bin(const Aabb& aabb, size_t& iprim) {
    if (!intersect_aabb(this->aabb, aabb)) {
      // The triangle's AABB is not intersecting with the current binner space.
      // So simply ignore this triangle.
      return false;
    }

    size_t imin_x = get_ibin(grid_lines_x, aabb.min.x);
    size_t imax_x = get_ibin(grid_lines_x, aabb.max.x);
    size_t imin_y = get_ibin(grid_lines_y, aabb.min.y);
    size_t imax_y = get_ibin(grid_lines_y, aabb.max.y);
    size_t imin_z = get_ibin(grid_lines_z, aabb.min.z);
    size_t imax_z = get_ibin(grid_lines_z, aabb.max.z);
    // TODO: (penguinliong) Consider the case of a very narrow triangle placed
    // on the diagonal of the bins looped here. There is a significant room of
    // optimization for large triangles.
    for (size_t z = imin_z; z <= imax_z; ++z) {
      for (size_t y = imin_y; y <= imax_y; ++y) {
        for (size_t x = imin_x; x <= imax_x; ++x) {
          size_t i = ((z * grid_res.y + y) * grid_res.x) + x;
          bins.at(i).iprims.emplace_back(counter);
        }
      }
    }
    ++counter;
    return true;
  }

  bool bin(const glm::vec3& point, size_t& iprim) {
    if (!contains_point_aabb(aabb, point)) {
      // The point is not intersecting with the current binner space. So simply
      // ignore this point.
      return false;
    }

    size_t x = get_ibin(grid_lines_x, point.x);
    size_t y = get_ibin(grid_lines_y, point.y);
    size_t z = get_ibin(grid_lines_z, point.z);
    size_t i = ((z * grid_res.y + y) * grid_res.x) + x;
    bins.at(i).iprims.emplace_back(counter);
    ++counter;
    return true;
  }

  BinGrid into_grid() {
    BinGrid grid {
      std::move(grid_lines_x),
      std::move(grid_lines_y),
      std::move(grid_lines_z),
      std::move(bins),
    };
    return grid;
  }
};

BinGrid bin_point_cloud(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const PointCloud& point_cloud
) {
  Binner binner(aabb, grid_res);
  for (const auto& point : point_cloud.poses) {
    size_t _;
    binner.bin(point, _);
  }
  return binner.into_grid();
}
BinGrid bin_point_cloud(
  const glm::vec3& grid_interval,
  const PointCloud& point_cloud
) {
  Aabb aabb = point_cloud.aabb();
  glm::uvec3 grid_res = glm::ceil(aabb.size() / grid_interval);
  aabb = Aabb::from_center_size(
    aabb.center(),
    glm::vec3(grid_res) * grid_interval);
  return bin_point_cloud(aabb, grid_res, point_cloud);
}

BinGrid bin_mesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const Mesh& mesh
) {
  Binner binner(aabb, grid_res);
  for (size_t i = 0; i < mesh.poses.size(); i += 3) {
    glm::vec3 points[3] {
      mesh.poses.at(i),
      mesh.poses.at(i + 2),
      mesh.poses.at(i + 1),
    };
    Aabb aabb2 = Aabb::from_points(points, 3);
    size_t _;
    binner.bin(aabb2, _);
  }
  return binner.into_grid();
}
BinGrid bin_mesh(
  const glm::vec3& grid_interval,
  const Mesh& mesh
) {
  Aabb aabb = mesh.aabb();
  glm::uvec3 grid_res = glm::ceil(aabb.size() / grid_interval);
  aabb = Aabb::from_center_size(
    aabb.center(),
    glm::vec3(grid_res) * grid_interval);
  return bin_mesh(aabb, grid_res, mesh);
}

BinGrid bin_idxmesh(
  const Aabb& aabb,
  const glm::uvec3& grid_res,
  const IndexedMesh& idxmesh
) {
  Binner binner(aabb, grid_res);
  for (const auto& idx : idxmesh.idxs) {
    glm::vec3 points[3] {
      idxmesh.mesh.poses.at(idx.x),
      idxmesh.mesh.poses.at(idx.y),
      idxmesh.mesh.poses.at(idx.z),
    };
    Aabb aabb2 = Aabb::from_points(points, 3);
    size_t _;
    binner.bin(aabb2, _);
  }
  return binner.into_grid();
}
BinGrid bin_idxmesh(
  const glm::vec3& grid_interval,
  const IndexedMesh& idxmesh
) {
  Aabb aabb = idxmesh.aabb();
  glm::uvec3 grid_res = glm::ceil(aabb.size() / grid_interval);
  aabb = Aabb::from_center_size(
    aabb.center(),
    glm::vec3(grid_res) * grid_interval);
  return bin_idxmesh(aabb, grid_res, idxmesh);
}


template<typename TKey, typename TValue = TKey>
struct Dedup {
  struct Key {
    TKey key;
    friend bool operator<(const Key& a, const Key& b) {
      return std::memcmp(&a, &b, sizeof(Key)) < 0;
    }
  };
  std::map<Key, size_t> key2idx;
  std::map<size_t, TValue> idx2val;

  size_t get_idx(const TKey& key) {
    Key k { key };
    auto it = key2idx.find(k);
    if (it == key2idx.end()) {
      size_t idx = idx2val.size();
      idx2val.emplace(std::make_pair(idx, TValue { key }));
      key2idx.emplace_hint(it, std::make_pair(std::move(k), idx));
      return idx;
    } else {
      return it->second;
    }
  }
  TValue& get_value(size_t idx) {
    return idx2val.at(idx);
  }

  std::vector<TValue> take_data() {
    std::vector<TValue> out {};
    out.reserve(idx2val.size());
    for (auto& pair : idx2val) {
      out.emplace_back(std::move(pair.second));
    }
    return out;
  }
};

TetrahedralMesh TetrahedralMesh::from_points(float density, const std::vector<glm::vec3>& points) {
  // Bin vertices into a voxel grid.
  PointCloud point_cloud { points };
  glm::vec3 size = geom::Aabb::from_points(points).size() * density;
  float max_edge = std::max(size.x, std::max(size.y, size.z));
  BinGrid grid = bin_point_cloud(glm::vec3(max_edge / (density * 10.0f)), point_cloud);
  Dedup<glm::vec3, TetrahedralVertex> dedup_tetra_vert {};
  Dedup<glm::uvec4, TetrahedralCell> dedup_tetra_cell {};

  // Split voxel bins into tetrahedrons.
  std::vector<TetrahedralInterpolant> interps;
  interps.resize(points.size());
  std::vector<geom::Tetrahedron> tets;
  tets.reserve(5);
  for (const auto& bin : grid.bins) {
    geom::split_aabb2tetras(bin.aabb, tets);

    std::vector<uint32_t> iprims(bin.iprims.begin(), bin.iprims.end());
    for (const auto& tet : tets) {
      TetrahedralInterpolant interp_templ {};
      glm::uvec4 tetra_cell = glm::uvec4(
        dedup_tetra_vert.get_idx(tet.a),
        dedup_tetra_vert.get_idx(tet.b),
        dedup_tetra_vert.get_idx(tet.c),
        dedup_tetra_vert.get_idx(tet.d));
      uint32_t itetra_cell = dedup_tetra_cell.get_idx(tetra_cell);
      interp_templ.itetra_cell = itetra_cell;

      dedup_tetra_vert.get_value(tetra_cell.x).ineighbor_cells.insert(itetra_cell);

      for (size_t i = 0; i < iprims.size();) {
        size_t iprim = iprims.at(i);
        glm::vec4 bary;
        if (contains_point_tetra(tet, points.at(iprim), bary)) {
          iprims.erase(iprims.begin() + i);

          TetrahedralInterpolant interp = interp_templ;
          interp.tetra_weights = bary;
          interps.at(iprim) = (interp);
        } else {
          ++i;
        }
      }
    }

    L_ASSERT(iprims.empty());
    tets.clear();
  }

  TetrahedralMesh tetra_mesh {};
  tetra_mesh.tetra_verts = dedup_tetra_vert.take_data();
  tetra_mesh.tetra_cells = dedup_tetra_cell.take_data();
  tetra_mesh.interps = std::move(interps);
  return tetra_mesh;
}
std::vector<glm::vec3> TetrahedralMesh::to_points() const {
  std::vector<glm::vec3> out {};
  out.reserve(interps.size());
  for (size_t i = 0; i < interps.size(); ++i) {
    const TetrahedralInterpolant& interp = interps.at(i);
    const TetrahedralCell& tetra_cell = tetra_cells.at(interp.itetra_cell);
    glm::vec3 vert =
      tetra_verts.at(tetra_cell.itetra_verts.x).pos * interp.tetra_weights.x +
      tetra_verts.at(tetra_cell.itetra_verts.y).pos * interp.tetra_weights.y +
      tetra_verts.at(tetra_cell.itetra_verts.z).pos * interp.tetra_weights.z +
      tetra_verts.at(tetra_cell.itetra_verts.w).pos * interp.tetra_weights.w;
    out.emplace_back(vert);
  }
  return out;
}

std::vector<geom::Tetrahedron> TetrahedralMesh::to_tetras() const {
  std::vector<geom::Tetrahedron> out {};
  for (const auto& tetra_cell : tetra_cells) {
    geom::Tetrahedron tetra {};
    tetra.a = tetra_verts.at(tetra_cell.itetra_verts.x).pos;
    tetra.b = tetra_verts.at(tetra_cell.itetra_verts.y).pos;
    tetra.c = tetra_verts.at(tetra_cell.itetra_verts.z).pos;
    tetra.d = tetra_verts.at(tetra_cell.itetra_verts.w).pos;
    out.emplace_back(std::move(tetra));
  }
  return out;
}
Mesh TetrahedralMesh::to_mesh() const {
  std::vector<geom::Tetrahedron> tetras = to_tetras();
  std::vector<Triangle> out {};
  out.reserve(tetras.size() * 4);
  for (const auto& tetra : tetras) {
    split_tetra2tris(tetra, out);
  }
  return Mesh::from_tris(out);
}



} // namespace mesh
} // namespace liong
