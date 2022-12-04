#pragma once

#include "erhe/toolkit/unique_id.hpp"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <string_view>

namespace erhe::graphics
{
    class Sampler;
    class Texture;
}

namespace erhe::primitive
{

class Material
{
public:
    Material();

    explicit Material(
        const std::string_view name,
        const glm::vec3        base_color = glm::vec3{1.0f, 1.0f, 1.0f},
        const glm::vec2        roughness  = glm::vec2{0.5f, 0.5f},
        const float            metallic   = 0.0f
    );
    ~Material() noexcept;

    // Interface similar to erhe::scene::Scene_item
    [[nodiscard]] static auto static_type_name() -> const char*;
    [[nodiscard]] auto type_name     () const -> const char*;
    [[nodiscard]] auto get_name      () const -> const std::string&;
                  void set_name      (const std::string_view name);
    [[nodiscard]] auto get_label     () const -> const std::string&;
    [[nodiscard]] auto is_shown_in_ui() const -> bool;
    [[nodiscard]] auto get_id        () const -> erhe::toolkit::Unique_id<Material>::id_type;

    uint32_t                                 material_buffer_index{0}; // updated by Material_buffer::update()

    std::string                              name;
    float                                    metallic    {0.0f};
    glm::vec2                                roughness   {0.5f, 0.5f};
    float                                    transparency{0.0f};
    glm::vec4                                base_color  {1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4                                emissive    {0.0f, 0.0f, 0.0f, 0.0f};
    std::shared_ptr<erhe::graphics::Texture> texture;
    std::shared_ptr<erhe::graphics::Sampler> sampler;

    erhe::toolkit::Unique_id<Material>       m_id;
    std::string                              m_label;
    bool                                     m_shown_in_ui{true};
};

} // namespace erhe::primitive
