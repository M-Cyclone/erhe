#include "editor_view_client.hpp"

#include "editor_rendering.hpp"
#include "editor_scenes.hpp"
#include "scene/scene_builder.hpp"
#include "scene/scene_root.hpp"
#include "scene/viewport_window.hpp"
#include "scene/viewport_windows.hpp"
#include "tools/tools.hpp"
#include "windows/physics_window.hpp"

#include "erhe/commands/commands.hpp"
#include "erhe/configuration/configuration.hpp"
#include "erhe/imgui/imgui_renderer.hpp"
#include "erhe/imgui/imgui_windows.hpp"
#include "erhe/rendergraph/rendergraph.hpp"
#include "time.hpp"
#include "erhe/toolkit/window_event_handler.hpp"
#include "erhe/graphics/buffer_transfer_queue.hpp"
#include "erhe/graphics/debug.hpp"
#include "erhe/physics/iworld.hpp"
#include "erhe/scene/scene.hpp"
#include "erhe/toolkit/profile.hpp"

namespace editor
{

Editor_view_client* g_editor_view_client{nullptr};

Editor_view_client::Editor_view_client()
    : Component{c_type_name}
{
}

Editor_view_client::~Editor_view_client() noexcept
{
    ERHE_VERIFY(g_editor_view_client == this);
    g_editor_view_client = nullptr;
}

void Editor_view_client::declare_required_components()
{
    //// require<View>();
}

void Editor_view_client::initialize_component()
{
    ERHE_PROFILE_FUNCTION();
    ERHE_VERIFY(g_editor_view_client == nullptr);

    g_view->set_client(this);
    g_editor_view_client = this;
}

// TODO Something nicer
void Editor_view_client::update_fixed_step(const erhe::components::Time_context& time_context)
{
    const auto& test_scene_root = g_scene_builder->get_scene_root();

    if (g_physics_window->config.static_enable) {
        test_scene_root->physics_world().update_fixed_step(time_context.dt);
    }
}

void Editor_view_client::update(View& view)
{
    static_cast<void>(view);
    {
        // TODO something nicer
        g_scene_builder->buffer_transfer_queue().flush();
        // animate_lights(time_context.time);
    }

    g_time->update_once_per_frame();

    g_editor_rendering->begin_frame();
    g_editor_scenes->update_node_transforms();
    g_editor_scenes->update_network();
    erhe::imgui::g_imgui_windows ->imgui_windows();
    m_rendergraph->execute();
    erhe::imgui::g_imgui_renderer->next_frame   ();
    g_editor_rendering->end_frame();
    erhe::commands::g_commands->on_update();
 }

} // namespace hextiles

