#include "windows/brushes.hpp"
#include "editor_tools.hpp"
#include "editor_view.hpp"
#include "log.hpp"
#include "rendering.hpp"

#include "operations/operation_stack.hpp"
#include "operations/insert_operation.hpp"
#include "renderers/line_renderer.hpp"
#include "scene/brush.hpp"
#include "scene/helpers.hpp"
#include "scene/node_physics.hpp"
#include "scene/node_raytrace.hpp"
#include "scene/scene_root.hpp"
#include "tools/grid_tool.hpp"
#include "tools/selection_tool.hpp"
#include "windows/materials.hpp"
#include "windows/operations.hpp"

#include "erhe/geometry/operation/clone.hpp"
#include "erhe/geometry/geometry.hpp"
#include "erhe/imgui/imgui_helpers.hpp"
#include "erhe/primitive/material.hpp"
#include "erhe/primitive/primitive_builder.hpp"
#include "erhe/scene/mesh.hpp"
#include "erhe/scene/scene.hpp"

#include <glm/gtx/transform.hpp>
#include <imgui.h>

namespace editor
{

using glm::mat4;
using glm::vec3;
using glm::vec4;
using erhe::geometry::Polygon; // Resolve conflict with wingdi.h BOOL Polygon(HDC,const POINT *,int)
using erhe::geometry::Polygon_id;
using erhe::scene::Node;

auto Brush_tool_preview_command::try_call(Command_context& context) -> bool
{
    static_cast<void>(context);

    if (
        (state() != State::Ready) ||
        !m_brushes.is_enabled()
    )
    {
        return false;
    }
    m_brushes.on_motion();
    return true;
}

void Brush_tool_insert_command::try_ready(Command_context& context)
{
    if (m_brushes.try_insert_ready())
    {
        set_ready(context);
    }
}

auto Brush_tool_insert_command::try_call(Command_context& context) -> bool
{
    if (state() != State::Ready)
    {
        return false;
    }
    const bool consumed = m_brushes.try_insert();
    set_inactive(context);
    return consumed;
}

Brushes::Brushes()
    : erhe::components::Component{c_name}
    , Imgui_window               {c_title}
    , m_preview_command          {*this}
    , m_insert_command           {*this}
{
}

Brushes::~Brushes() = default;

void Brushes::connect()
{
    m_grid_tool       = get<Grid_tool>();
    m_materials       = get<Materials>();
    m_operation_stack = get<Operation_stack>();
    m_pointer_context = get<Pointer_context>();
    m_scene_root      = require<Scene_root>();
    m_selection_tool  = get<Selection_tool>();
}

void Brushes::initialize_component()
{
    m_selected_brush_index = 0;

    get<Editor_tools>()->register_tool(this);

    const auto view = get<Editor_view>();
    view->register_command(&m_preview_command);
    view->register_command(&m_insert_command);
    view->bind_command_to_mouse_motion(&m_preview_command);
    view->bind_command_to_mouse_click (&m_insert_command, erhe::toolkit::Mouse_button_right);

    get<Operations>()->register_active_tool(this);
}

auto Brushes::allocate_brush(
    erhe::primitive::Build_info& build_info
) -> std::shared_ptr<Brush>
{
    const std::lock_guard<std::mutex> lock{m_brush_mutex};

    const auto brush = std::make_shared<Brush>(build_info);
    m_brushes.push_back(brush);
    return brush;
}

auto Brushes::make_brush(
    erhe::geometry::Geometry&&                              geometry,
    const Brush_create_context&                             context,
    const std::shared_ptr<erhe::physics::ICollision_shape>& collision_shape
) -> std::shared_ptr<Brush>
{
    ERHE_PROFILE_FUNCTION

    const auto shared_geometry = std::make_shared<erhe::geometry::Geometry>(
        std::move(geometry)
    );
    return make_brush(shared_geometry, context, collision_shape);
}

auto Brushes::make_brush(
    const std::shared_ptr<erhe::geometry::Geometry>&        geometry,
    const Brush_create_context&                             context,
    const std::shared_ptr<erhe::physics::ICollision_shape>& collision_shape
) -> std::shared_ptr<Brush>
{
    ERHE_PROFILE_FUNCTION

    geometry->build_edges();
    geometry->compute_polygon_normals();
    geometry->compute_tangents();
    geometry->compute_polygon_centroids();
    geometry->compute_point_normals(erhe::geometry::c_point_normals_smooth);

    const auto brush = allocate_brush(context.build_info);
    brush->initialize(
        Brush::Create_info{
            .geometry        = geometry,
            .build_info      = context.build_info,
            .normal_style    = context.normal_style,
            .density         = 1.0f,
            .volume          = geometry->get_mass_properties().volume,
            .collision_shape = collision_shape
        }
    );
    return brush;
}

auto Brushes::make_brush(
    const std::shared_ptr<erhe::geometry::Geometry>& geometry,
    const Brush_create_context&                      context,
    const Collision_volume_calculator                collision_volume_calculator,
    const Collision_shape_generator                  collision_shape_generator
) -> std::shared_ptr<Brush>
{
    ERHE_PROFILE_FUNCTION

    geometry->build_edges();
    geometry->compute_polygon_normals();
    geometry->compute_tangents();
    geometry->compute_polygon_centroids();
    geometry->compute_point_normals(erhe::geometry::c_point_normals_smooth);
    const Brush::Create_info create_info{
        .geometry                    = geometry,
        .build_info                  = context.build_info,
        .normal_style                = context.normal_style,
        .density                     = 1.0f,
        .volume                      = 1.0f,
        .collision_shape             = {},
        .collision_volume_calculator = collision_volume_calculator,
        .collision_shape_generator   = collision_shape_generator
    };

    const auto brush = allocate_brush(context.build_info);
    brush->initialize(create_info);
    return brush;
}

void Brushes::remove_brush_mesh()
{
    if (m_brush_mesh)
    {
        log_brush.trace("removing brush mesh\n");
        remove_from_scene_layer(
            m_scene_root->scene(),
            *m_scene_root->brush_layer().get(),
            m_brush_mesh
        );
        m_brush_mesh->unparent();
        m_brush_mesh.reset();
    }
}

auto Brushes::try_insert_ready() -> bool
{
    return is_enabled() && m_hover_content;
}

auto Brushes::try_insert() -> bool
{
    if (
        !m_brush_mesh ||
        !m_hover_position.has_value() ||
        (m_brush == nullptr)
    )
    {
        return false;
    }

    do_insert_operation();
    remove_brush_mesh();
    return true;
}

void Brushes::on_enable_state_changed()
{
    if (is_enabled())
    {
        on_motion();
    }
    else
    {
        m_hover_content     = false;
        m_hover_tool        = false;
        m_hover_mesh    .reset();
        m_hover_primitive   = 0;
        m_hover_local_index = 0;
        m_hover_geometry    = nullptr;
        m_hover_position.reset();
        m_hover_normal  .reset();
        remove_brush_mesh();
    }
}

void Brushes::on_motion()
{
    m_hover_content     = m_pointer_context->hovering_over_content();
    m_hover_tool        = m_pointer_context->hovering_over_tool();
    m_hover_mesh        = m_pointer_context->hover_mesh();
    m_hover_primitive   = m_pointer_context->hover_primitive();
    m_hover_local_index = m_pointer_context->hover_local_index();
    m_hover_geometry    = m_pointer_context->hover_geometry();

    m_hover_position = m_hover_content
        ? m_pointer_context->position_in_world()
        : std::optional<vec3>{};

    m_hover_normal = m_hover_content
        ? m_pointer_context->hover_normal()
        : std::optional<vec3>{};

    if (m_hover_mesh && m_hover_position.has_value())
    {
        m_hover_position = m_hover_mesh->transform_direction_from_world_to_local(m_hover_position.value());
    }
}

// Returns transform which places brush in parent (hover) mesh space.
auto Brushes::get_brush_transform() -> mat4
{
    if (
        (m_hover_mesh == nullptr) ||
        (m_hover_geometry == nullptr) ||
        (m_brush == nullptr)
    )
    {
        return mat4{1};
    }

    const Polygon_id polygon_id = static_cast<const Polygon_id>(m_hover_local_index);
    const Polygon&   polygon    = m_hover_geometry->polygons[polygon_id];
    Reference_frame  hover_frame(*m_hover_geometry, polygon_id);
    Reference_frame  brush_frame = m_brush->get_reference_frame(polygon.corner_count);
    hover_frame.N *= -1.0f;
    hover_frame.B *= -1.0f;

    ERHE_VERIFY(brush_frame.scale() != 0.0f);

    const float scale = hover_frame.scale() / brush_frame.scale();

    m_transform_scale = scale;
    if (scale != 1.0f)
    {
        const mat4 scale_transform = erhe::toolkit::create_scale(scale);
        brush_frame.transform_by(scale_transform);
    }

    debug_info.hover_frame_scale = hover_frame.scale();
    debug_info.brush_frame_scale = brush_frame.scale();
    debug_info.transform_scale   = scale;

    if (
        !m_snap_to_hover_polygon &&
        m_hover_position.has_value()
    )
    {
        hover_frame.centroid = m_hover_position.value();
        if (m_snap_to_grid)
        {
            hover_frame.centroid = m_grid_tool->snap(hover_frame.centroid);
        }
    }

    const mat4 hover_transform = hover_frame.transform();
    const mat4 brush_transform = brush_frame.transform();
    const mat4 inverse_brush   = inverse(brush_transform);
    const mat4 align           = hover_transform * inverse_brush;

    return align;
}

void Brushes::update_mesh_node_transform()
{
    if (!m_hover_position.has_value())
    {
        return;
    }

    ERHE_VERIFY(m_brush_mesh);
    ERHE_VERIFY(m_hover_mesh);

    const auto  transform    = get_brush_transform();
    const auto& brush_scaled = m_brush->get_scaled(m_transform_scale);
    if (m_brush_mesh->parent() != m_hover_mesh.get())
    {
        if (m_brush_mesh->parent())
        {
            log_brush.trace(
                "m_brush_mesh->parent() = {} ({})\n",
                m_brush_mesh->parent()->name(),
                m_brush_mesh->parent()->node_type()
            );
        }
        else
        {
            log_brush.trace("m_brush_mesh->parent() = (none)\n");
        }

        if (m_hover_mesh)
        {
            log_brush.trace(
                "m_hover_mesh = {} ({})\n",
                m_hover_mesh->name(),
                m_hover_mesh->node_type()
            );
        }
        else
        {
            log_brush.trace("m_hover_mesh = (none)\n");
        }

        if (m_hover_mesh)
        {
            m_hover_mesh->attach(m_brush_mesh);
        }
    }
    m_brush_mesh->set_parent_from_node(transform);

    auto& primitive = m_brush_mesh->data.primitives.front();
    primitive.gl_primitive_geometry = brush_scaled.gl_primitive_geometry;
    primitive.rt_primitive_geometry = brush_scaled.rt_primitive->primitive_geometry;
    primitive.rt_vertex_buffer      = brush_scaled.rt_primitive->vertex_buffer;
    primitive.rt_index_buffer       = brush_scaled.rt_primitive->index_buffer;
}

void Brushes::do_insert_operation()
{
    if (
        !m_hover_position.has_value() ||
        (m_brush == nullptr)
    )
    {
        return;
    }

    log_brush.trace("{} scale = {}\n", __func__, m_transform_scale);

    const auto hover_from_brush = get_brush_transform();
    const auto world_from_brush = m_hover_mesh->world_from_node() * hover_from_brush;
    const auto material         = m_materials->selected_material();
    const auto instance         = m_brush->make_instance(
        world_from_brush,
        material,
        m_transform_scale
    );
    instance.mesh->visibility_mask() &= ~Node::c_visibility_brush;
    instance.mesh->visibility_mask() |=
        (Node::c_visibility_content     |
         Node::c_visibility_shadow_cast |
         Node::c_visibility_id);

    auto op = std::make_shared<Mesh_insert_remove_operation>(
        Mesh_insert_remove_operation::Context{
            .scene          = m_scene_root->scene(),
            .layer          = m_scene_root->content_layer(),
            .physics_world  = m_scene_root->physics_world(),
            .mesh           = instance.mesh,
            .node_physics   = instance.node_physics,
            .parent         = m_hover_mesh,
            .mode           = Scene_item_operation::Mode::insert,
            .selection_tool = m_selection_tool.get()
        }
    );
    m_operation_stack->push(op);
}

void Brushes::add_brush_mesh()
{
    if (
        (m_brush == nullptr) ||
        !m_hover_position.has_value()
    )
    {
        return;
    }

    const auto material = m_materials->selected_material();
    if (!material)
    {
        return;
    }

    const auto& brush_scaled = m_brush->get_scaled(m_transform_scale);
    m_brush_mesh = std::make_shared<erhe::scene::Mesh>(
        m_brush->name(),
        erhe::primitive::Primitive{
            .material              = material,
            .gl_primitive_geometry = brush_scaled.gl_primitive_geometry,
            .rt_primitive_geometry = brush_scaled.rt_primitive->primitive_geometry,
            .rt_vertex_buffer      = brush_scaled.rt_primitive->vertex_buffer,
            .rt_index_buffer       = brush_scaled.rt_primitive->index_buffer,
            .source_geometry       = brush_scaled.geometry,
            .normal_style          = m_brush->normal_style
        }
    );
    m_scene_root->add(m_brush_mesh, m_scene_root->brush_layer().get());

    m_brush_mesh->visibility_mask() &= ~(Node::c_visibility_id);
    m_brush_mesh->visibility_mask() |= Node::c_visibility_brush;
    update_mesh_node_transform();
}

void Brushes::update_mesh()
{
    if (!m_brush_mesh)
    {
        add_brush_mesh();
        return;
    }

    if (
        (m_brush == nullptr) ||
        !m_hover_position.has_value()
    )
    {
        remove_brush_mesh();
    }

    update_mesh_node_transform();
}

void Brushes::tool_properties()
{
    using erhe::imgui::make_check_box;
    using erhe::imgui::Item_mode;

    ImGui::InputFloat("Hover scale",     &debug_info.hover_frame_scale);
    ImGui::InputFloat("Brush scale",     &debug_info.brush_frame_scale);
    ImGui::InputFloat("Transform scale", &debug_info.transform_scale);
    ImGui::SliderFloat("Scale", &m_scale, 0.0f, 2.0f);
    make_check_box("Snap to Polygon", &m_snap_to_hover_polygon);
    make_check_box(
        "Snap to Grid",
        &m_snap_to_grid,
        m_snap_to_hover_polygon
            ? Item_mode::disabled
            : Item_mode::normal
    );
}

void Brushes::imgui()
{
    using erhe::imgui::make_button;
    using erhe::imgui::Item_mode;

    const size_t brush_count = m_brushes.size();

    {
        const ImVec2 button_size{ImGui::GetContentRegionAvail().x, 0.0f};
        for (int i = 0; i < static_cast<int>(brush_count); ++i)
        {
            auto* brush = m_brushes[i].get();
            const bool button_pressed = make_button(
                brush->geometry->name.c_str(),
                (m_selected_brush_index == i)
                    ? Item_mode::active
                    : Item_mode::normal,
                button_size
            );
            if (button_pressed)
            {
                m_selected_brush_index = i;
                m_brush = brush;
            }
        }
    }
}

}
