#pragma once

#include "tools/tool.hpp"

#include "erhe/application/configuration.hpp"
#include "erhe/application/imgui/imgui_window.hpp"
#include "erhe/components/components.hpp"
#include "erhe/primitive/enums.hpp"
#include "erhe/scene/transform.hpp"

#include <glm/glm.hpp>

#include <memory>

namespace erhe::application
{
    class Line_renderer_set;
}

namespace erhe::scene
{
    class Node;
}

namespace editor
{

class Brush;
class Brush_data;
class Editor_scenes;
class Content_library_window;
class Mesh_memory;
class Operation_stack;
class Scene_commands;
class Selection_tool;

class Brush_create
{
public:
    virtual void render_preview(
        const Render_context&                 context,
        erhe::application::Line_renderer_set& line_renderer_set,
        const erhe::scene::Transform&         transform
    ) = 0;

    virtual void imgui() = 0;

    [[nodiscard]] virtual auto create(Brush_data& brush_create_info) const -> std::shared_ptr<Brush> = 0;
};

class Create_uv_sphere
    : public Brush_create
{
public:
    void render_preview(
        const Render_context&                 context,
        erhe::application::Line_renderer_set& line_renderer_set,
        const erhe::scene::Transform&         transform
    ) override;

    void imgui() override;

    [[nodiscard]] auto create(Brush_data& brush_create_info) const -> std::shared_ptr<Brush> override;

private:
    int   m_slice_count{8};
    int   m_stack_count{8};
    float m_radius     {1.0f};
};

class Create_cone
    : public Brush_create
{
public:
    void render_preview(
        const Render_context&                 context,
        erhe::application::Line_renderer_set& line_renderer_set,
        const erhe::scene::Transform&         transform
    ) override;

    void imgui() override;

    [[nodiscard]] auto create(Brush_data& brush_create_info) const -> std::shared_ptr<Brush> override;

private:
    int   m_slice_count  {32};
    int   m_stack_count  {1};
    float m_height       {1.33f};
    float m_bottom_radius{1.0f};
    float m_top_radius   {0.5f};
};

class Create_torus
    : public Brush_create
{
public:
    void render_preview(
        const Render_context&                 context,
        erhe::application::Line_renderer_set& line_renderer_set,
        const erhe::scene::Transform&         transform
    ) override;

    void imgui() override;

    [[nodiscard]] auto create(Brush_data& brush_create_info) const -> std::shared_ptr<Brush> override;

private:
    float m_major_radius{1.0f};
    float m_minor_radius{0.5f};
    int   m_major_steps {32};
    int   m_minor_steps {28};

    bool      m_use_debug_camera{false};
    glm::vec3 m_debug_camera    {0.0f, 0.0f, 0.0f};
    int       m_debug_major     {0};
    int       m_debug_minor     {0};
    float     m_epsilon         {0.004f};
};

class Create_box
    : public Brush_create
{
public:
    void render_preview(
        const Render_context&                 context,
        erhe::application::Line_renderer_set& line_renderer_set,
        const erhe::scene::Transform&         transform
    ) override;

    void imgui() override;

    [[nodiscard]] auto create(Brush_data& brush_create_info) const -> std::shared_ptr<Brush> override;

private:
    glm::vec3  m_size   {1.0f, 1.0f, 1.0f};
    glm::ivec3 m_steps  {3, 3, 3};
    float      m_power  {1.0f};
};

class Create
    : public erhe::components::Component
    , public Tool
    , public erhe::application::Imgui_window
{
public:
    static constexpr int              c_priority {4};
    static constexpr std::string_view c_type_name{"Create"};
    static constexpr std::string_view c_title    {"Create"};
    static constexpr uint32_t c_type_hash = compiletime_xxhash::xxh32(c_type_name.data(), c_type_name.size(), {});

    Create ();
    ~Create() noexcept override;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return c_type_hash; }
    void declare_required_components() override;
    void initialize_component       () override;
    void post_initialize            () override;

    // Implements Tool
    [[nodiscard]] auto tool_priority() const -> int   override { return c_priority; }
    [[nodiscard]] auto description  () -> const char* override { return c_title.data(); }
    void tool_render            (const Render_context& context) override;

    // Implements Imgui_window
    void imgui() override;

    [[nodiscard]] auto find_parent() -> std::shared_ptr<erhe::scene::Node>;

private:
    void brush_create_button(const char* label, Brush_create* brush_create);

    // Component dependencies
    std::shared_ptr<erhe::application::Line_renderer_set> m_line_renderer_set;
    std::shared_ptr<Editor_scenes         > m_editor_scenes;
    std::shared_ptr<Content_library_window> m_content_library_window;
    std::shared_ptr<Mesh_memory           > m_mesh_memory;
    std::shared_ptr<Operation_stack       > m_operation_stack;
    std::shared_ptr<Scene_commands        > m_scene_commands;
    std::shared_ptr<Selection_tool        > m_selection_tool;

    erhe::primitive::Normal_style m_normal_style{erhe::primitive::Normal_style::point_normals};
    float                         m_density     {1.0f};

    Create_uv_sphere m_create_uv_sphere;
    Create_cone      m_create_cone;
    Create_torus     m_create_torus;
    Create_box       m_create_box;
    Brush_create*    m_brush_create{nullptr};

    std::string            m_brush_name;
    std::shared_ptr<Brush> m_brush;
};

} // namespace editor
