#pragma once

#include "erhe/application/imgui/imgui_window.hpp"
#include "erhe/components/components.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace erhe::graphics
{
    class Texture;
}

namespace erhe::scene
{
    class Node;
    enum class Light_type : unsigned int;
}

namespace editor
{

class Editor_scenes;
class Icon_set;
class Selection_tool;

class Layers_window
    : public erhe::components::Component
    , public erhe::application::Imgui_window
{
public:
    static constexpr std::string_view c_type_name{"Layers_window"};
    static constexpr std::string_view c_title{"Layers"};
    static constexpr uint32_t c_type_hash = compiletime_xxhash::xxh32(c_type_name.data(), c_type_name.size(), {});

    Layers_window ();
    ~Layers_window() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return c_type_hash; }
    void declare_required_components() override;
    void initialize_component       () override;
    void post_initialize            () override;

    // Implements Imgui_window
    void imgui() override;

private:
    // Component dependencies
    std::shared_ptr<Editor_scenes>  m_editor_scenes;
    std::shared_ptr<Selection_tool> m_selection_tool;
    std::shared_ptr<Icon_set>       m_icon_set;

    std::shared_ptr<erhe::scene::Node> m_node_clicked;
};

} // namespace editor
