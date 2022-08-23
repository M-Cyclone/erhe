#pragma once

#include "erhe/application/imgui/imgui_window.hpp"

#include "erhe/components/components.hpp"

#include <string_view>

namespace erhe::graphics
{
    class Blend_state_component;
    class Color_blend_state;
    class Depth_stencil_state;
    class Pipeline;
    class Rasterization_state;
    class Stencil_op_state;
}

namespace erhe::application
{

class Pipelines
    : public erhe::components::Component
    , public Imgui_window
{
public:
    static constexpr std::string_view c_type_name{"Pipelines"};
    static constexpr std::string_view c_title{"Pipelines"};
    static constexpr uint32_t c_type_hash = compiletime_xxhash::xxh32(c_type_name.data(), c_type_name.size(), {});

    Pipelines();

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return c_type_hash; }
    void declare_required_components() override;
    void initialize_component       () override;

    // Implements Imgui_window
    void imgui() override;

private:
    void rasterization(erhe::graphics::Rasterization_state& rasterization);
    void stencil_op(
        const char*                       label,
        erhe::graphics::Stencil_op_state& stencil_op
    );
    void depth_stencil(erhe::graphics::Depth_stencil_state& depth_stencil);
    void blend_state_component(
        const char*                            label,
        erhe::graphics::Blend_state_component& component
    );
    void color_blend(erhe::graphics::Color_blend_state& color_blend);
};

} // namespace erhe::application
