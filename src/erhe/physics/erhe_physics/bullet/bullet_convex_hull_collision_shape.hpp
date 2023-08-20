#pragma once

#include "erhe_physics/bullet/bullet_collision_shape.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace erhe::physics
{

class Bullet_convex_hull_collision_shape
    : public Bullet_collision_shape
{
public:
    Bullet_convex_hull_collision_shape(
        const float* points,
        const int    num_points,
        const int    stride
    );

private:
    btConvexHullShape m_convex_hull_shape;
};

} // namespace erhe::physics
