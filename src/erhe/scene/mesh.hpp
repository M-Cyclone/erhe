#pragma once

#include "erhe/scene/node.hpp"
#include "erhe/scene/scene.hpp"
#include "erhe/primitive/primitive.hpp"
#include "erhe/toolkit/unique_id.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace erhe::geometry
{
    class Geometry;
}

namespace erhe::scene
{

class Mesh_data
{
public:
    erhe::toolkit::Unique_id<Mesh_layer>::id_type layer_id;
    std::vector<erhe::primitive::Primitive>       primitives;
    float                                         point_size{3.0f};
    float                                         line_width{1.0f};
};

class Mesh
    : public Node
{
public:
    Mesh         ();
    explicit Mesh(const std::string_view name);
    Mesh         (const std::string_view name, const erhe::primitive::Primitive primitive);
    ~Mesh        () noexcept override;

    [[nodiscard]] auto node_type() const -> const char* override;

    Mesh_data mesh_data;

    erhe::toolkit::Unique_id<Mesh> m_id;
};

[[nodiscard]] auto operator<(const Mesh& lhs, const Mesh& rhs) -> bool;

[[nodiscard]] auto is_mesh(const Node* const node) -> bool;
[[nodiscard]] auto is_mesh(const std::shared_ptr<Node>& node) -> bool;
[[nodiscard]] auto as_mesh(Node* const node) -> Mesh*;
[[nodiscard]] auto as_mesh(const std::shared_ptr<Node>& node) -> std::shared_ptr<Mesh>;

} // namespace erhe::scene
