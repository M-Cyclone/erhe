#include "scene/scene_root.hpp"
#include "scene/helpers.hpp"
#include "scene/node_physics.hpp"
#include "scene/node_raytrace.hpp"
#include "log.hpp"

#include "erhe/graphics/buffer.hpp"
#include "erhe/primitive/material.hpp"
#include "erhe/physics/iworld.hpp"
#include "erhe/raytrace/iscene.hpp"
#include "erhe/scene/camera.hpp"
#include "erhe/scene/light.hpp"
#include "erhe/scene/mesh.hpp"
#include "erhe/scene/node.hpp"
#include "erhe/scene/scene.hpp"
#include "erhe/toolkit/math_util.hpp"
#include "erhe/toolkit/profile.hpp"

#include <glm/gtx/color_space.hpp>

#include <imgui.h>

#include <algorithm>

namespace editor
{

using namespace erhe::graphics;
using namespace erhe::geometry;
using namespace erhe::scene;
using namespace erhe::primitive;
using namespace std;
using namespace glm;


Camera_rig::Camera_rig(
    Scene_root&                          /*scene_root*/,
    std::shared_ptr<erhe::scene::Camera> camera
)
    : position{camera}
{
    // position_fps_heading           = make_shared<erhe::scene::Camera>("Camera");
    // position_fps_heading_elevation = make_shared<erhe::scene::Camera>("Camera");
    // position_free                  = make_shared<erhe::scene::Camera>("Camera");
    // *position_fps_heading          ->projection() = *camera->projection();
    // *position_fps_heading_elevation->projection() = *camera->projection();
    // *position_free                 ->projection() = *camera->projection();
    // scene_root.scene().cameras.push_back(position_fps_heading          );
    // scene_root.scene().cameras.push_back(position_fps_heading_elevation);
    // scene_root.scene().cameras.push_back(position_fps_heading);
    //
    // auto position_fps_heading_node = make_shared<erhe::scene::Node>();
    // scene_root.scene().nodes.emplace_back(position_fps_heading_node);
    // const glm::mat4 identity{1.0f};
    // position_fps_heading_node->transforms.parent_from_node.set(identity);
    // position_fps_heading_node->update();
    // position_fps_heading_node->attach(position_fps_heading);
}


Scene_root::Scene_root()
    : Component{c_name}
{
}

Scene_root::~Scene_root() = default;

void Scene_root::connect()
{
}

void Scene_root::initialize_component()
{
    ERHE_PROFILE_FUNCTION

    // Layer configuration
    m_content_layer    = make_shared<Mesh_layer>("content");
    m_controller_layer = make_shared<Mesh_layer>("controller");
    m_tool_layer       = make_shared<Mesh_layer>("tool");
    m_brush_layer      = make_shared<Mesh_layer>("brush");
    m_light_layer      = make_shared<Light_layer>("lights");

    m_scene            = std::make_unique<Scene>();

    m_scene->mesh_layers .push_back(m_content_layer);
    m_scene->mesh_layers .push_back(m_controller_layer);
    m_scene->mesh_layers .push_back(m_tool_layer);
    m_scene->mesh_layers .push_back(m_brush_layer);
    m_scene->light_layers.push_back(m_light_layer);

    m_all_layers         .push_back(m_content_layer.get());
    m_all_layers         .push_back(m_controller_layer.get());
    m_all_layers         .push_back(m_tool_layer.get());
    m_all_layers         .push_back(m_brush_layer.get());
    m_content_layers     .push_back(m_content_layer.get());
    m_content_fill_layers.push_back(m_content_layer.get());
    m_content_fill_layers.push_back(m_controller_layer.get());
    m_tool_layers        .push_back(m_tool_layer.get());
    m_brush_layers       .push_back(m_brush_layer.get());

    m_physics_world = erhe::physics::IWorld::create_unique();

    m_raytrace_scene = erhe::raytrace::IScene::create_unique("root");
}

auto Scene_root::materials() -> vector<shared_ptr<Material>>&
{
    return m_materials;
}

auto Scene_root::materials() const -> const vector<shared_ptr<Material>>&
{
    return m_materials;
}

auto Scene_root::physics_world() -> erhe::physics::IWorld&
{
    ERHE_VERIFY(m_physics_world);
    return *m_physics_world.get();
}

auto Scene_root::raytrace_scene() -> erhe::raytrace::IScene&
{
    ERHE_VERIFY(m_raytrace_scene);
    return *m_raytrace_scene.get();
}

auto Scene_root::scene() -> erhe::scene::Scene&
{
    ERHE_VERIFY(m_scene);
    return *m_scene.get();
}

auto Scene_root::scene() const -> const erhe::scene::Scene&
{
    ERHE_VERIFY(m_scene);
    return *m_scene.get();
}

auto Scene_root::content_layer() -> erhe::scene::Mesh_layer&
{
    ERHE_VERIFY(m_content_layer);
    return *m_content_layer.get();
}

void Scene_root::add(
    const std::shared_ptr<erhe::scene::Mesh>& mesh,
    Mesh_layer*                               layer
)
{
    std::lock_guard<std::mutex> lock{m_scene_mutex};

    if (layer == nullptr)
    {
        layer = m_content_layer.get();
    }
    add_to_scene_layer(scene(), *layer, mesh);
}

auto Scene_root::brush_layer() const -> std::shared_ptr<erhe::scene::Mesh_layer>
{
    return m_brush_layer;
}

auto Scene_root::content_layer() const -> std::shared_ptr<erhe::scene::Mesh_layer>
{
    return m_content_layer;
}

auto Scene_root::controller_layer() const -> std::shared_ptr<erhe::scene::Mesh_layer>
{
    return m_controller_layer;
}

auto Scene_root::tool_layer() const -> std::shared_ptr<erhe::scene::Mesh_layer>
{
    return m_tool_layer;
}

auto Scene_root::light_layer() const -> std::shared_ptr<erhe::scene::Light_layer>
{
    return m_light_layer;
}

auto Scene_root::camera_combo(
    const char*            label,
    erhe::scene::ICamera*& selected_camera,
    const bool             nullptr_option
) const -> bool
{
    int selected_camera_index = 0;
    int index = 0;
    std::vector<const char*>           names;
    std::vector<erhe::scene::ICamera*> cameras;
    if (nullptr_option || (selected_camera == nullptr))
    {
        names.push_back("(none)");
        cameras.push_back(nullptr);
    }
    for (auto camera : scene().cameras)
    {
        names.push_back(camera->name().c_str());
        cameras.push_back(camera.get());
        if (selected_camera == camera.get())
        {
            selected_camera_index = index;
        }
        ++index;
    }

    const bool camera_changed =
        ImGui::Combo(
            label,
            &selected_camera_index,
            names.data(),
            static_cast<int>(names.size())
        ) &&
        (selected_camera != cameras[selected_camera_index]);
    if (camera_changed)
    {
        selected_camera = cameras[selected_camera_index];
    }
    return camera_changed;
}

namespace
{

[[nodiscard]]
auto sort_value(const Light::Type light_type) -> int
{
    switch (light_type)
    {
        case Light::Type::directional: return 0;
        case Light::Type::point:       return 1;
        case Light::Type::spot:        return 2;
        default: return 3;
    }
}

class Light_comparator
{
public:
    [[nodiscard]]
    inline auto operator()(
        const shared_ptr<Light>& lhs,
        const shared_ptr<Light>& rhs
    ) -> bool
    {
        return sort_value(lhs->type) < sort_value(rhs->type);
    }
};

}

void Scene_root::sort_lights()
{
    sort(
        light_layer()->lights.begin(),
        light_layer()->lights.end(),
        Light_comparator()
    );
}


} // namespace editor
