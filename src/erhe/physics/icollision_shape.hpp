#pragma once

#include "erhe/physics/transform.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace erhe::physics
{

enum class Axis : int
{
    X,
    Y,
    Z
};

class ICollision_shape;

class Compound_child
{
public:
    std::shared_ptr<ICollision_shape>& shape;
    Transform                          transform;
};

class Compound_shape_create_info
{
public:
    std::vector<Compound_child> children;
};

class Mass_properties
{
public:
    float     mass          {0.0f};
    glm::mat4 inertia_tensor{0.0f};
};

class ICollision_shape
{
public:
    virtual ~ICollision_shape() noexcept {};

    [[nodiscard]] static auto create_empty_shape                 () -> ICollision_shape*;
    [[nodiscard]] static auto create_empty_shape_shared          () -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_box_shape                   (const glm::vec3 half_extents) -> ICollision_shape*;
    [[nodiscard]] static auto create_box_shape_shared            (const glm::vec3 half_extents) -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_capsule_shape               (const Axis axis, const float radius, const float length) -> ICollision_shape*;
    [[nodiscard]] static auto create_capsule_shape_shared        (const Axis axis, const float radius, const float length) -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_compound_shape              (const Compound_shape_create_info& create_info) -> ICollision_shape*;
    [[nodiscard]] static auto create_compound_shape_shared       (const Compound_shape_create_info& create_info) -> std::shared_ptr<ICollision_shape>;

#if 0
    [[nodiscard]] static auto create_cone_shape                  (const Axis axis, const float base_radius, const float height) -> ICollision_shape*;
    [[nodiscard]] static auto create_cone_shape_shared           (const Axis axis, const float base_radius, const float height) -> std::shared_ptr<ICollision_shape>;
#endif

    [[nodiscard]] static auto create_convex_hull_shape           (const float* points, const int point_count, const int stride) -> ICollision_shape*;
    [[nodiscard]] static auto create_convex_hull_shape_shared    (const float* points, const int point_count, const int stride) -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_cylinder_shape              (const Axis axis, const glm::vec3 half_extents) -> ICollision_shape*;
    [[nodiscard]] static auto create_cylinder_shape_shared       (const Axis axis, const glm::vec3 half_extents) -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_sphere_shape                (const float radius) -> ICollision_shape*;
    [[nodiscard]] static auto create_sphere_shape_shared         (const float radius) -> std::shared_ptr<ICollision_shape>;

    [[nodiscard]] static auto create_uniform_scaling_shape       (ICollision_shape* shape, const float scale) -> ICollision_shape*;
    [[nodiscard]] static auto create_uniform_scaling_shape_shared(ICollision_shape* shape, const float scale) -> std::shared_ptr<ICollision_shape>;

                  virtual void calculate_local_inertia           (const float mass, glm::mat4& inertia) const = 0;
    [[nodiscard]] virtual auto is_convex                         () const -> bool = 0;
    [[nodiscard]] virtual auto get_center_of_mass                () const -> glm::vec3 = 0;
    [[nodiscard]] virtual auto get_mass_properties               () const -> Mass_properties = 0;

    //virtual void calculate_principal_axis_transform(
    //    const std::vector<float>& child_masses,
    //    Transform&                principal_transform,
    //    glm::mat4&                inertia
    //) = 0;

};


} // namespace erhe::physics
