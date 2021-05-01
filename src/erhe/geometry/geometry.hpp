#pragma once

#include "erhe/geometry/property_map.hpp"
#include "erhe/geometry/property_map_collection.hpp"
#include "erhe/geometry/log.hpp"

#include <glm/glm.hpp>
#include <gsl/assert>

#include "Tracy.hpp"

#include <optional>
#include <set>
#include <vector>

namespace erhe::geometry
{

inline constexpr const Property_map_descriptor c_point_locations     { "point_locations"     , Transform_mode::matrix                                       , Interpolation_mode::linear };
inline constexpr const Property_map_descriptor c_point_normals       { "point_normals"       , Transform_mode::normalize_inverse_transpose_matrix           , Interpolation_mode::normalized };
inline constexpr const Property_map_descriptor c_point_normals_smooth{ "point_normals_smooth", Transform_mode::normalize_inverse_transpose_matrix           , Interpolation_mode::normalized  };
inline constexpr const Property_map_descriptor c_point_texcoords     { "point_texcoords"     , Transform_mode::none                                         , Interpolation_mode::linear };
inline constexpr const Property_map_descriptor c_point_tangents      { "point_tangents"      , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::normalized_vec3_float };
inline constexpr const Property_map_descriptor c_point_bitangents    { "point_bitangents"    , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::normalized_vec3_float };
inline constexpr const Property_map_descriptor c_point_colors        { "point_colors"        , Transform_mode::none                                         , Interpolation_mode::linear };
inline constexpr const Property_map_descriptor c_corner_normals      { "corner_normals"      , Transform_mode::normalize_inverse_transpose_matrix           , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_corner_texcoords    { "corner_texcoords"    , Transform_mode::none                                         , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_corner_tangents     { "corner_tangents"     , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_corner_bitangents   { "corner_bitangents"   , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_corner_colors       { "corner_colors"       , Transform_mode::none                                         , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_corner_indices      { "corner_indices"      , Transform_mode::none                                         , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_centroids   { "polygon_centroids"   , Transform_mode::matrix                                       , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_normals     { "polygon_normals"     , Transform_mode::normalize_inverse_transpose_matrix           , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_tangents    { "polygon_tangents"    , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_bitangents  { "polygon_bitangents"  , Transform_mode::normalize_inverse_transpose_matrix_vec3_float, Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_colors      { "polygon_colors"      , Transform_mode::none                                         , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_ids_vec3    { "polygon_ids_vec"     , Transform_mode::none                                         , Interpolation_mode::none };
inline constexpr const Property_map_descriptor c_polygon_ids_uint    { "polygon_ids_uint"    , Transform_mode::none                                         , Interpolation_mode::none };

using Corner_id         = uint32_t; // index to Geometry::corners
using Point_id          = uint32_t; // index to Geometry::points
using Polygon_id        = uint32_t; // index to Geometry::polygons
using Edge_id           = uint32_t; // index to Geometry::edges
using Point_corner_id   = uint32_t; // index to Geometry::point_corners
using Polygon_corner_id = uint32_t; // index to Geometry::polygon_corners
using Edge_polygon_id   = uint32_t; // index to Geometry::edge_polygons

struct Point;
struct Polygon;
class Geometry;
struct Edge;

struct Corner
{
    Point_id   point_id{0};
    Polygon_id polygon_id{0};

    template<typename T>
    void smooth_normalize(Corner_id                                  this_corner_id,
                          const Geometry&                            geometry,
                          Property_map<Corner_id, T>&                corner_attribute,
                          const Property_map<Polygon_id, T>&         polygon_attribute,
                          const Property_map<Polygon_id, glm::vec3>& polygon_normals,
                          float                                      cos_max_smoothing_angle) const;

    template<typename T>
    void smooth_average(Corner_id                                 this_corner_id,
                        const Geometry&                           geometry,
                        Property_map<Corner_id, T>&               new_corner_attribute,
                        const Property_map<Corner_id, T>&         old_corner_attribute,
                        const Property_map<Corner_id, glm::vec3>& corner_normals,
                        const Property_map<Point_id, glm::vec3>&  point_normals) const;
};

struct Point
{
    Point_corner_id first_point_corner_id{0};
    uint32_t        corner_count{0};
    uint32_t        reserved_corner_count{0};
};

struct Polygon
{
    Polygon_corner_id first_polygon_corner_id{0};
    uint32_t          corner_count{0};

    // Copies polygon property to corners
    template <typename T>
    void copy_to_corners(Polygon_id                         this_polygon_id,
                         const Geometry&                    geometry,
                         Property_map<Corner_id, T>&        corner_attribute,
                         const Property_map<Polygon_id, T>& polygon_attribute) const;

    template <typename T>
    void smooth_normalize(const Geometry&                            geometry,
                          Property_map<Corner_id, T>&                corner_attribute,
                          const Property_map<Polygon_id, T>&         polygon_attribute,
                          const Property_map<Polygon_id, glm::vec3>& polygon_normals,
                          float                                      cos_max_smoothing_angle) const;

    template <typename T>
    void smooth_average(const Geometry&                           geometry,
                        Property_map<Corner_id, T>&               new_corner_attribute,
                        const Property_map<Corner_id, T>&         old_corner_attribute,
                        const Property_map<Corner_id, glm::vec3>& corner_normals,
                        const Property_map<Point_id, glm::vec3>&  point_normals) const;

    void compute_normal(Polygon_id                               this_polygon_id,
                        const Geometry&                          geometry,
                        Property_map<Polygon_id, glm::vec3>&     polygon_normals,
                        const Property_map<Point_id, glm::vec3>& point_locations) const;

    auto compute_normal(const Geometry&                          geometry,
                        const Property_map<Point_id, glm::vec3>& point_locations) const -> glm::vec3;

    auto compute_centroid(const Geometry&                          geometry,
                          const Property_map<Point_id, glm::vec3>& point_locations) const -> glm::vec3;

    void compute_centroid(Polygon_id                               this_polygon_id,
                          const Geometry&                          geometry,
                          Property_map<Polygon_id, glm::vec3>&     polygon_centroids,
                          const Property_map<Point_id, glm::vec3>& point_locations) const;

    void compute_planar_texture_coordinates(Polygon_id                                 this_polygon_id,
                                            const Geometry&                            geometry,
                                            Property_map<Corner_id, glm::vec2>&        corner_texcoords,
                                            const Property_map<Polygon_id, glm::vec3>& polygon_centroids,
                                            const Property_map<Polygon_id, glm::vec3>& polygon_normals,
                                            const Property_map<Point_id, glm::vec3>&   point_locations,
                                            bool                                       overwrite = false) const;

    auto corner(const Geometry& geometry, Point_id point_id) -> Corner_id;

    auto next_corner(const Geometry& geometry, Corner_id anchor_corner_id) -> Corner_id;

    auto prev_corner(const Geometry& geometry, Corner_id corner_id) -> Corner_id;

    void reverse(Geometry& geometry);
};

struct Edge
{
    Point_id        a;
    Point_id        b;
    Edge_polygon_id first_edge_polygon_id;
    uint32_t        polygon_count;
};

class Geometry
{
public:
    std::vector<Corner>     corners;
    std::vector<Point>      points;
    std::vector<Polygon>    polygons;
    std::vector<Edge>       edges;
    std::vector<Corner_id>  point_corners;
    std::vector<Corner_id>  polygon_corners;
    std::vector<Polygon_id> edge_polygons;

    using Point_property_map_collection   = Property_map_collection<Point_id>;
    using Corner_property_map_collection  = Property_map_collection<Corner_id>;
    using Polygon_property_map_collection = Property_map_collection<Polygon_id>;
    using Edge_property_map_collection    = Property_map_collection<Edge_id>;

    struct Mesh_info
    {
        size_t polygon_count              {0};
        size_t corner_count               {0};
        size_t triangle_count             {0};
        size_t edge_count                 {0};
        size_t vertex_count_corners       {0};
        size_t vertex_count_centroids     {0};
        size_t index_count_fill_triangles {0};
        size_t index_count_edge_lines     {0};
        size_t index_count_corner_points  {0};
        size_t index_count_centroid_points{0};

        auto operator+=(const Mesh_info& o)
        -> Mesh_info&;

        void trace(erhe::log::Log::Category& log) const;
    };

    void promise_has_normals()
    {
        m_serial_point_normals  = m_serial;
        m_serial_corner_normals = m_serial;
    }

    void promise_has_tangents()
    {
        m_serial_polygon_tangents = m_serial;
        m_serial_point_tangents   = m_serial;
        m_serial_corner_tangents  = m_serial;
    }

    void promise_has_bitangents()
    {
        m_serial_polygon_bitangents = m_serial;
        m_serial_point_bitangents   = m_serial;
        m_serial_corner_bitangents  = m_serial;
    }

    void promise_has_texture_coordinates()
    {
        m_serial_point_texture_coordinates = m_serial;
        m_serial_corner_texture_coordinates = m_serial;
    }

private:
    Corner_id         m_next_corner_id           {0};
    Point_id          m_next_point_id            {0};
    Polygon_id        m_next_polygon_id          {0};
    Edge_id           m_next_edge_id             {0};
    Point_corner_id   m_next_point_corner_reserve{0};
    Polygon_corner_id m_next_polygon_corner_id   {0};
    Edge_polygon_id   m_next_edge_polygon_id     {0};
    Polygon_id        m_polygon_corner_polygon   {0};
    Edge_id           m_edge_polygon_edge        {0};

    constexpr static size_t s_grow = 4096;

public:
    uint32_t corner_count () const { return m_next_corner_id; }
    uint32_t point_count  () const { return m_next_point_id; }
    uint32_t polygon_count() const { return m_next_polygon_id; }
    uint32_t edge_count   () const { return m_next_edge_id; }

    std::optional<Edge> find_edge(Point_id a, Point_id b)
    {
        if (b < a)
        {
            std::swap(a, b);
        }
        for (const auto& edge : edges)
        {
            if (edge.a == a && edge.b == b)
            {
                return edge;
            }
        }
        return {};
    }

    // Allocates new Corner / Corner_id
    // - Point must be allocated.
    // - Polygon must be allocated
    auto make_corner(Point_id point_id, Polygon_id polygon_id) -> Corner_id;

    // Allocates new Point / Point_id
    auto make_point() -> Point_id;

    // Allocates new Polygon / Polygon_id
    auto make_polygon() -> Polygon_id;

    // Allocates new Edge / Edge_id
    // - Points must be already allocated
    // - Points must be ordered
    auto make_edge(Point_id a, Point_id b) -> Edge_id;

    // Reserves new point corner.
    // - Point must be already allocated.
    // - Corner must be already allocated.
    // - Does not actually create point corner, only allocates space
    void reserve_point_corner(Point_id point_id, Corner_id corner_id);

    // Allocates new edge polygon.
    // - Edge must be already allocated.
    // - Polygon must be already allocated.
    auto make_edge_polygon(Edge_id edge_id, Polygon_id polygon_id) -> Edge_polygon_id;

    // Allocates new polygon corner.
    // - Polygon must be already allocated.
    // - Corner must be already allocated.
    auto make_polygon_corner_(Polygon_id polygon_id, Corner_id corner_id) -> Polygon_corner_id;

    // Allocates new polygon corner.
    // - Polygon must be already allocated.
    // - Point must be already allocated.
    auto make_polygon_corner(Polygon_id polygon_id, Point_id point_id) -> Corner_id;

    Geometry() = default;

    explicit Geometry(const std::string& name)
        : m_name{name}
    {
    }

    Geometry(const Geometry&) = delete;

    Geometry& operator=(const Geometry&) = delete;

    Geometry(Geometry&& other) noexcept
        : corners                             {std::move(other.corners)}
        , points                              {std::move(other.points)}
        , polygons                            {std::move(other.polygons)}
        , edges                               {std::move(other.edges)}
        , point_corners                       {std::move(other.point_corners)}
        , polygon_corners                     {std::move(other.polygon_corners)}
        , edge_polygons                       {std::move(other.edge_polygons)}
        , m_next_corner_id                    {other.m_next_corner_id           }
        , m_next_point_id                     {other.m_next_point_id            }
        , m_next_polygon_id                   {other.m_next_polygon_id          }
        , m_next_edge_id                      {other.m_next_edge_id             }
        , m_next_point_corner_reserve         {other.m_next_point_corner_reserve}
        , m_next_polygon_corner_id            {other.m_next_polygon_corner_id   }
        , m_next_edge_polygon_id              {other.m_next_edge_polygon_id     }
        , m_polygon_corner_polygon            {other.m_polygon_corner_polygon   }
        , m_edge_polygon_edge                 {other.m_edge_polygon_edge        }
        , m_name                              {std::move(other.m_name)}
        , m_point_property_map_collection     {std::move(other.m_point_property_map_collection)}
        , m_corner_property_map_collection    {std::move(other.m_corner_property_map_collection)}
        , m_polygon_property_map_collection   {std::move(other.m_polygon_property_map_collection)}
        , m_edge_property_map_collection      {std::move(other.m_edge_property_map_collection)}
        , m_serial                            {other.m_serial}
        , m_serial_edges                      {other.m_serial_edges                      }
        , m_serial_polygon_normals            {other.m_serial_polygon_normals            }
        , m_serial_polygon_centroids          {other.m_serial_polygon_centroids          }
        , m_serial_polygon_tangents           {other.m_serial_polygon_tangents           }
        , m_serial_polygon_bitangents         {other.m_serial_polygon_bitangents         }
        , m_serial_polygon_texture_coordinates{other.m_serial_polygon_texture_coordinates}
        , m_serial_point_normals              {other.m_serial_point_normals              }
        , m_serial_point_tangents             {other.m_serial_point_tangents             }
        , m_serial_point_bitangents           {other.m_serial_point_bitangents           }
        , m_serial_point_texture_coordinates  {other.m_serial_point_texture_coordinates  }
        , m_serial_smooth_point_normals       {other.m_serial_smooth_point_normals       }
        , m_serial_corner_normals             {other.m_serial_corner_normals             }
        , m_serial_corner_tangents            {other.m_serial_corner_tangents            }
        , m_serial_corner_bitangents          {other.m_serial_corner_bitangents          }
        , m_serial_corner_texture_coordinates {other.m_serial_corner_texture_coordinates }
    {
    }

    auto name() const -> const std::string&
    {
        return m_name;
    }

    auto count_polygon_triangles() const -> size_t;

    void info(Mesh_info& info) const;

    auto point_attributes() -> Point_property_map_collection&
    {
        return m_point_property_map_collection;
    }

    auto corner_attributes() -> Corner_property_map_collection&
    {
        return m_corner_property_map_collection;
    }

    auto polygon_attributes() -> Polygon_property_map_collection&
    {
        return m_polygon_property_map_collection;
    }

    auto edge_attributes() -> Edge_property_map_collection&
    {
        return m_edge_property_map_collection;
    }

    auto point_attributes() const -> const Point_property_map_collection&
    {
        return m_point_property_map_collection;
    }

    auto corner_attributes() const -> const Corner_property_map_collection&
    {
        return m_corner_property_map_collection;
    }

    auto polygon_attributes() const -> const Polygon_property_map_collection&
    {
        return m_polygon_property_map_collection;
    }

    auto edge_attributes() const -> const Edge_property_map_collection&
    {
        return m_edge_property_map_collection;
    }

    void reserve_points(size_t point_count);

    void reserve_polygons(size_t polygon_count);

    auto make_point(float x, float y, float z) -> Point_id;

    auto make_point(float x, float y, float z, float s, float t) -> Point_id;

    auto make_point(double x, double y, double z) -> Point_id;

    auto make_point(double x, double y, double z, double s, double t) -> Point_id;

    auto make_polygon(const std::initializer_list<Point_id> point_list) -> Polygon_id;

    // Requires point locations.
    // Returns false if point locations are not available.
    // Returns true on success.
    auto compute_polygon_normals() -> bool;

    // Requires point locations.
    // Returns false if point locations are not available.
    // Returns true on success.
    auto compute_polygon_centroids() -> bool;

    // Calculates point normal from polygon normals
    // Returns incorrect data if there are missing polygon normals.
    auto compute_point_normal(Point_id point_id) -> glm::vec3;

    // Calculates point normals from polygon normals.
    // If polygon normals are not up to date before this call,
    // also updates polygon normals.
    // Returns false if unable to calculate polygon normals
    // (due to missing point locations).
    auto compute_point_normals(const Property_map_descriptor& descriptor) -> bool;

    auto compute_tangents(bool corner_tangents    = true,
                          bool corner_bitangents  = true,
                          bool polygon_tangents   = false,
                          bool polygon_bitangents = false,
                          bool make_polygons_flat = true,
                          bool override_existing  = false) -> bool;

    auto generate_polygon_texture_coordinates(bool overwrite_existing_texture_coordinates = false) -> bool;

    // Experimental
    void generate_texture_coordinates_spherical();

    template <typename T>
    void smooth_normalize(Property_map<Corner_id, T>&                corner_attribute,
                          const Property_map<Polygon_id, T>&         polygon_attribute,
                          const Property_map<Polygon_id, glm::vec3>& polygon_normals,
                          float                                      max_smoothing_angle_radians) const;

    template <typename T>
    void smooth_average(Property_map<Corner_id, T>&                smoothed_corner_attribute,
                        const Property_map<Corner_id, T>&          corner_attribute,
                        const Property_map<Corner_id, glm::vec3>&  corner_normals,
                        const Property_map<Polygon_id, glm::vec3>& point_normals) const;

    void make_point_corners();

    void build_edges();

    void transform(const glm::mat4& m);

    void reverse_polygons();

    void debug_trace();

private:
    std::string                     m_name;
    Point_property_map_collection   m_point_property_map_collection;
    Corner_property_map_collection  m_corner_property_map_collection;
    Polygon_property_map_collection m_polygon_property_map_collection;
    Edge_property_map_collection    m_edge_property_map_collection;
    uint64_t                        m_serial{1};
    uint64_t                        m_serial_edges{0};
    uint64_t                        m_serial_polygon_normals{0};
    uint64_t                        m_serial_polygon_centroids{0};
    uint64_t                        m_serial_polygon_tangents{0};
    uint64_t                        m_serial_polygon_bitangents{0};
    uint64_t                        m_serial_polygon_texture_coordinates{0};
    uint64_t                        m_serial_point_normals{0};
    uint64_t                        m_serial_point_tangents{0};   // never generated
    uint64_t                        m_serial_point_bitangents{0}; // never generated
    uint64_t                        m_serial_point_texture_coordinates{0}; // never generated
    uint64_t                        m_serial_smooth_point_normals{0};
    uint64_t                        m_serial_corner_normals{0}; 
    uint64_t                        m_serial_corner_tangents{0};
    uint64_t                        m_serial_corner_bitangents{0};
    uint64_t                        m_serial_corner_texture_coordinates{0}; 
};

} // namespace erhe::geometry

#include "corner.inl"
#include "polygon.inl"
#include "geometry.inl"
