#pragma once

#include "scene/scene_view.hpp"
#include "scene/viewport_window.hpp"

#include "erhe/imgui/imgui_window.hpp"
#include "erhe/rendergraph/rendergraph_node.hpp"
#include "erhe/toolkit/math_util.hpp"

#include <array>

namespace erhe::graphics {
    class Instance;
}
namespace erhe::imgui {
    class Imgui_windows;
}
namespace erhe::renderer {
    class Line_renderer_set;
    class Text_renderer;
}
namespace erhe::scene {
    class Camera;
    class Node;
}
namespace erhe::scene_renderer {
    class Forward_renderer;
    class Shadow_renderer;
}
namespace erhe::toolkit {
    class Context_window;
}

namespace editor
{

class Editor_context;
class Editor_message_bus;
class Editor_rendering;
class Hud;
class Mesh_memory;
class Scene_builder;
class Scene_root;
class Tools;

class Controller_input
{;
public:
    glm::vec3 position     {0.0f, 0.0f, 0.0f};
    glm::vec3 direction    {0.0f, 0.0f, 0.0f};
    float     trigger_value{0.0f};
};


class Headset_view
    : public Scene_view
    , public std::enable_shared_from_this<Headset_view>
{
public:
    Headset_view(
        erhe::graphics::Instance&              graphics_instance,
        erhe::rendergraph::Rendergraph&        rendergraph,
        erhe::scene_renderer::Shadow_renderer& shadow_renderer,
        erhe::toolkit::Context_window&         context_window,
        Editor_context&                        editor_context,
        Editor_rendering&                      editor_rendering,
        Mesh_memory&                           mesh_memory,
        Scene_builder&                         scene_builder
    );

    // Public API
    void render_headset  ();

    void begin_frame     ();
    //void add_finger_input(const Finger_point& finger_point);
    void end_frame       ();

    [[nodiscard]] auto finger_to_viewport_distance_threshold() const -> float;
    [[nodiscard]] auto get_headset           () const -> void*;

    [[nodiscard]] auto get_root_node() const -> std::shared_ptr<erhe::scene::Node>;

    void render(const Render_context&);

    // Implements Scene_view
    [[nodiscard]] auto get_scene_root        () const -> std::shared_ptr<Scene_root>                    override;
    [[nodiscard]] auto get_camera            () const -> std::shared_ptr<erhe::scene::Camera>           override;
    [[nodiscard]] auto get_shadow_render_node() const -> Shadow_render_node*                            override;
    [[nodiscard]] auto get_rendergraph_node  () -> erhe::rendergraph::Rendergraph_node*       override;

private:
};

} // namespace editor
