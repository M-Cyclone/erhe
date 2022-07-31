#include "editor_rendering.hpp"
#include "editor_scenes.hpp"
#include "editor_view_client.hpp"
#include "graphics/icon_set.hpp"
#include "graphics/image_transfer.hpp"
#include "operations/operation_stack.hpp"
#include "rendertarget_node.hpp"
#include "rendertarget_imgui_viewport.hpp"

#include "renderers/forward_renderer.hpp"
#if defined(ERHE_XR_LIBRARY_OPENXR)
#   include "xr/headset_renderer.hpp"
#   include "xr/hand_tracker.hpp"
#   include "xr/theremin.hpp"
#endif
#include "renderers/id_renderer.hpp"
#include "renderers/mesh_memory.hpp"
#include "renderers/post_processing.hpp"
#include "renderers/program_interface.hpp"
#include "renderers/programs.hpp"
#include "renderers/shadow_renderer.hpp"

#include "tools/debug_visualizations.hpp"
#include "tools/fly_camera_tool.hpp"
#include "tools/grid_tool.hpp"
#include "tools/hover_tool.hpp"
#include "tools/material_paint_tool.hpp"
#include "tools/physics_tool.hpp"
#include "tools/selection_tool.hpp"
#include "tools/tools.hpp"
#include "tools/trs_tool.hpp"

#include "windows/brushes.hpp"
#include "windows/debug_view_window.hpp"
#include "windows/layers_window.hpp"
#include "windows/material_properties.hpp"
#include "windows/materials_window.hpp"
#include "windows/mesh_properties.hpp"
#include "windows/node_properties.hpp"
#include "windows/node_tree_window.hpp"
#include "windows/operations.hpp"
#include "windows/physics_window.hpp"
#include "windows/render_graph_window.hpp"
#include "windows/tool_properties_window.hpp"
#include "windows/viewport_config.hpp"
#include "windows/viewport_window.hpp"
#include "windows/viewport_windows.hpp"

#include "scene/debug_draw.hpp"
#include "scene/scene_builder.hpp"
#include "scene/scene_commands.hpp"
#include "scene/scene_root.hpp"

#include "erhe/application/application.hpp"
#include "erhe/application/configuration.hpp"
#include "erhe/application/application_log.hpp"
#include "erhe/application/imgui_windows.hpp"
#include "erhe/application/render_graph.hpp"
#include "erhe/application/time.hpp"
#include "erhe/application/view.hpp"
#include "erhe/application/window.hpp"
#include "erhe/application/graphics/gl_context_provider.hpp"
#include "erhe/application/graphics/shader_monitor.hpp"
#if defined(ERHE_GUI_LIBRARY_IMGUI)
#   include "erhe/application/renderers/imgui_renderer.hpp"
#endif
#include "erhe/application/renderers/line_renderer.hpp"
#include "erhe/application/renderers/text_renderer.hpp"
#include "erhe/application/windows/imgui_demo_window.hpp"
#include "erhe/application/windows/log_window.hpp"
#include "erhe/application/windows/performance_window.hpp"
#include "erhe/application/windows/pipelines.hpp"
#include "erhe/application/renderdoc_capture_support.hpp"
#include "erhe/gl/wrapper_functions.hpp"
#include "erhe/graphics/debug.hpp"
#include "erhe/graphics/opengl_state_tracker.hpp"
#include "erhe/graphics/pipeline.hpp"
#include "erhe/graphics/state/vertex_input_state.hpp"
#include "erhe/physics/iworld.hpp"
#include "erhe/scene/mesh.hpp"
#include "erhe/scene/scene.hpp"
#include "erhe/toolkit/math_util.hpp"
#include "erhe/toolkit/profile.hpp"
#include "erhe/toolkit/window.hpp"

#include <future>

namespace erhe::application {

using erhe::graphics::OpenGL_state_tracker;
using std::shared_ptr;
using std::make_shared;

auto Application::initialize_components(int argc, char** argv) -> bool
{
    ERHE_PROFILE_FUNCTION

    auto configuration             = make_shared<erhe::application::Configuration>(argc, argv);
    auto renderdoc_capture_support = make_shared<erhe::application::Renderdoc_capture_support>();
    auto window                    = make_shared<erhe::application::Window>();
    auto gl_context_provider       = make_shared<erhe::application::Gl_context_provider>();
    auto opengl_state_tracker      = make_shared<erhe::graphics::OpenGL_state_tracker>();

    {
        ERHE_PROFILE_SCOPE("add components");

        m_components.add(configuration);
        m_components.add(renderdoc_capture_support);
        m_components.add(window);
        m_components.add(shared_from_this());
        m_components.add(gl_context_provider);
        m_components.add(make_shared<erhe::application::Imgui_windows     >());
        m_components.add(make_shared<erhe::application::Time              >());
        m_components.add(make_shared<erhe::application::View              >());
        m_components.add(make_shared<erhe::application::Log_window        >());
#if defined(ERHE_GUI_LIBRARY_IMGUI)
        m_components.add(make_shared<erhe::application::Imgui_demo_window >());
        m_components.add(make_shared<erhe::application::Imgui_renderer    >());
#endif
        m_components.add(make_shared<erhe::application::Performance_window>());
        m_components.add(make_shared<erhe::application::Pipelines         >());
        m_components.add(make_shared<erhe::application::Render_graph      >());
        m_components.add(make_shared<erhe::application::Shader_monitor    >());
        m_components.add(make_shared<erhe::application::Text_renderer     >());
        m_components.add(make_shared<erhe::application::Line_renderer_set >());

        m_components.add(make_shared<erhe::graphics::OpenGL_state_tracker >());

        m_components.add(make_shared<editor::Program_interface     >());
        m_components.add(make_shared<editor::Programs              >());
        m_components.add(make_shared<editor::Image_transfer        >());
        m_components.add(make_shared<editor::Brushes               >());
        m_components.add(make_shared<editor::Debug_draw            >());
        m_components.add(make_shared<editor::Debug_view_window     >());
        m_components.add(make_shared<editor::Debug_visualizations  >());
        m_components.add(make_shared<editor::Editor_rendering      >());
        m_components.add(make_shared<editor::Editor_scenes         >());
        m_components.add(make_shared<editor::Editor_view_client    >());
        m_components.add(make_shared<editor::Fly_camera_tool       >());
        m_components.add(make_shared<editor::Forward_renderer      >());
        m_components.add(make_shared<editor::Grid_tool             >());
        m_components.add(make_shared<editor::Hover_tool            >());
        m_components.add(make_shared<editor::Icon_set              >());
        m_components.add(make_shared<editor::Id_renderer           >());
        m_components.add(make_shared<editor::Layers_window         >());
        m_components.add(make_shared<editor::Material_paint_tool   >());
        m_components.add(make_shared<editor::Material_properties   >());
        m_components.add(make_shared<editor::Materials_window      >());
        m_components.add(make_shared<editor::Mesh_memory           >());
        m_components.add(make_shared<editor::Mesh_properties       >());
        m_components.add(make_shared<editor::Node_properties       >());
        m_components.add(make_shared<editor::Node_tree_window      >());
        m_components.add(make_shared<editor::Operation_stack       >());
        m_components.add(make_shared<editor::Operations            >());
        m_components.add(make_shared<editor::Physics_tool          >());
        m_components.add(make_shared<editor::Physics_window        >());
        m_components.add(make_shared<editor::Post_processing       >());
        m_components.add(make_shared<editor::Render_graph_window   >());
        m_components.add(make_shared<editor::Scene_builder         >());
        m_components.add(make_shared<editor::Scene_commands        >());
        m_components.add(make_shared<editor::Selection_tool        >());
        m_components.add(make_shared<editor::Shadow_renderer       >());
        m_components.add(make_shared<editor::Tools                 >());
        m_components.add(make_shared<editor::Tool_properties_window>());
        m_components.add(make_shared<editor::Trs_tool              >());
        m_components.add(make_shared<editor::Viewport_config       >());
        m_components.add(make_shared<editor::Viewport_windows      >());

#if defined(ERHE_XR_LIBRARY_OPENXR)
        m_components.add(make_shared<Hand_tracker    >());
        m_components.add(make_shared<Headset_renderer>());
        m_components.add(make_shared<Theremin        >());
#endif
    }

    configuration->initialize_component();
    renderdoc_capture_support->initialize_component();

    if (!window->create_gl_window())
    {
        log_startup->error("GL window creation failed, aborting");
        return false;
    }

    m_components.launch_component_initialization(configuration->threading.parallel_initialization);

    if (configuration->threading.parallel_initialization)
    {
        gl_context_provider->provide_worker_contexts(
            opengl_state_tracker,
            window->get_context_window(),
            [this]() -> bool {
                return !m_components.is_component_initialization_complete();
            }
        );
    }

    m_components.wait_component_initialization_complete();

    component_initialization_complete(true);

    const auto& config = configuration->windows;

#if defined(ERHE_GUI_LIBRARY_IMGUI)
    if (m_components.get<erhe::application::Imgui_demo_window >()) m_components.get<erhe::application::Imgui_demo_window >()->hide();
#endif
    if (m_components.get<erhe::application::Log_window        >() && !config.log        ) m_components.get<erhe::application::Log_window        >()->hide();
    if (m_components.get<erhe::application::Performance_window>() && !config.performance) m_components.get<erhe::application::Performance_window>()->hide();
    if (m_components.get<erhe::application::Pipelines         >() && !config.pipelines  ) m_components.get<erhe::application::Pipelines         >()->hide();
    if (m_components.get<erhe::application::View              >() && !config.view       ) m_components.get<erhe::application::View              >()->hide();
    //if (m_components.get<erhe::application::Tool_properties_window>()) m_components.get<Terhe::application::ool_properties_window>()->hide();

    if (m_components.get<editor::Brushes               >() && !config.brushes            ) m_components.get<editor::Brushes               >()->hide();
    if (m_components.get<editor::Debug_view_window     >() && !config.debug_view         ) m_components.get<editor::Debug_view_window     >()->hide();
    if (m_components.get<editor::Fly_camera_tool       >() && !config.fly_camera         ) m_components.get<editor::Fly_camera_tool       >()->hide();
    if (m_components.get<editor::Grid_tool             >() && !config.grid               ) m_components.get<editor::Grid_tool             >()->hide();
    if (m_components.get<editor::Layers_window         >() && !config.layers             ) m_components.get<editor::Layers_window         >()->hide();
    if (m_components.get<editor::Material_properties   >() && !config.material_properties) m_components.get<editor::Material_properties   >()->hide();
    if (m_components.get<editor::Materials_window      >() && !config.materials          ) m_components.get<editor::Materials_window      >()->hide();
    if (m_components.get<editor::Mesh_properties       >() && !config.mesh_properties    ) m_components.get<editor::Mesh_properties       >()->hide();
    if (m_components.get<editor::Node_properties       >() && !config.node_properties    ) m_components.get<editor::Node_properties       >()->hide();
    if (m_components.get<editor::Node_tree_window      >() && !config.node_tree          ) m_components.get<editor::Node_tree_window      >()->hide();
    if (m_components.get<editor::Operation_stack       >() && !config.operation_stack    ) m_components.get<editor::Operation_stack       >()->hide();
    if (m_components.get<editor::Operations            >() && !config.operations         ) m_components.get<editor::Operations            >()->hide();
    if (m_components.get<editor::Physics_window        >() && !config.physics            ) m_components.get<editor::Physics_window        >()->hide();
    if (m_components.get<editor::Post_processing       >() && !config.post_processing    ) m_components.get<editor::Post_processing       >()->hide();
    if (m_components.get<editor::Render_graph_window   >() && !config.render_graph       ) m_components.get<editor::Render_graph_window   >()->hide();
    if (m_components.get<editor::Trs_tool              >() && !config.trs                ) m_components.get<editor::Trs_tool              >()->hide();
    if (m_components.get<editor::Tool_properties_window>() && !config.tool_properties    ) m_components.get<editor::Tool_properties_window>()->hide();
    if (m_components.get<editor::Viewport_config       >() && !config.viewport_config    ) m_components.get<editor::Viewport_config       >()->hide();

    if (configuration->physics.enabled)
    {
        const auto& scene_builder   = m_components.get<editor::Scene_builder   >();
        const auto& test_scene_root = scene_builder->get_scene_root();
        test_scene_root->physics_world().enable_physics_updates();
    }

#if defined(ERHE_GUI_LIBRARY_IMGUI)
    if (m_components.get<erhe::application::Imgui_demo_window>())
    {
        const auto& imgui_windows           = m_components.get<erhe::application::Imgui_windows>();
        const auto& scene_builder           = m_components.get<editor::Scene_builder   >();
        const auto& primary_viewport_window = scene_builder->get_primary_viewport_window();
        const auto& test_scene_root         = scene_builder->get_scene_root();

        auto rendertarget_node_1 = test_scene_root->create_rendertarget_node(
            m_components,
            *primary_viewport_window.get(),
            1920,
            1080,
            320.0
        );

        test_scene_root->scene().add_to_mesh_layer(
            *test_scene_root->layers().rendertarget(),
            rendertarget_node_1
        );

        rendertarget_node_1->set_world_from_node(
            erhe::toolkit::create_look_at(
                glm::vec3{-3.0f, 2.0f,  0.0f},
                glm::vec3{-4.0f, 2.0f, -2.0f},
                glm::vec3{ 0.0f, 1.0f,  0.0f}
            )
        );

        auto imgui_viewport_1 = std::make_shared<editor::Rendertarget_imgui_viewport>(
            rendertarget_node_1.get(),
            "Rendertarget ImGui Viewport 1",
            m_components
        );

        imgui_windows->register_imgui_viewport(imgui_viewport_1);

        const auto& grid_tool = m_components.get<editor::Grid_tool       >();
        grid_tool->set_viewport(imgui_viewport_1.get()); grid_tool->show();

#if 1
        const auto camera_b = scene_builder->make_camera(
            "Camera B",
            glm::vec3{-7.0f, 1.0f, 0.0f},
            glm::vec3{ 0.0f, 0.5f, 0.0f}
        );
        camera_b->node_data.wireframe_color = glm::vec4{0.3f, 0.6f, 1.00f, 1.0f};

        const auto& viewport_windows = m_components.get<editor::Viewport_windows>();
        auto secondary_viewport_window = viewport_windows->create_window(
            "Secondary Viewport",
            test_scene_root,
            camera_b.get()
        );
        secondary_viewport_window->set_post_processing_enable(false); // TODO Post processing currently only handles one viewport
#endif

#if 1
        auto rendertarget_node_2 = test_scene_root->create_rendertarget_node(
            m_components,
            *primary_viewport_window.get(),
            1920,
            1080,
            320.0
        );

        test_scene_root->scene().add_to_mesh_layer(
            *test_scene_root->layers().rendertarget(),
            rendertarget_node_2
        );

        rendertarget_node_2->set_world_from_node(
            erhe::toolkit::create_look_at(
                glm::vec3{3.0f, 2.0f,  0.0f},
                glm::vec3{4.0f, 2.0f, -2.0f},
                glm::vec3{0.0f, 1.0f,  0.0f}
            )
        );

        auto imgui_viewport_2 = std::make_shared<editor::Rendertarget_imgui_viewport>(
            rendertarget_node_2.get(),
            "Rendertarget ImGui Viewport 2",
            m_components
        );
        imgui_windows->register_imgui_viewport(imgui_viewport_2);

        secondary_viewport_window->set_viewport(imgui_viewport_2.get()); secondary_viewport_window->show();
#endif
    }
#endif

    gl::clip_control(gl::Clip_control_origin::lower_left, gl::Clip_control_depth::zero_to_one);
    gl::disable     (gl::Enable_cap::primitive_restart);
    gl::enable      (gl::Enable_cap::primitive_restart_fixed_index);
    gl::enable      (gl::Enable_cap::texture_cube_map_seamless);
    gl::enable      (gl::Enable_cap::framebuffer_srgb);

    opengl_state_tracker->on_thread_enter();

    return true;
}

void Application::component_initialization_complete(const bool initialization_succeeded)
{
    if (initialization_succeeded)
    {
        gl::enable(gl::Enable_cap::primitive_restart);
        gl::primitive_restart_index(0xffffu);

        const auto window = get<Window>();
        if (!window)
        {
            return;
        }

        auto* const context_window = window->get_context_window();
        if (context_window == nullptr)
        {
            return;
        }

        auto& root_view = context_window->get_root_view();

        root_view.reset_view(get<erhe::application::View>().get());
    }
}

}
