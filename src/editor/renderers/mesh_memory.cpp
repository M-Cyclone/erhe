#include "renderers/mesh_memory.hpp"
#include "graphics/gl_context_provider.hpp"
#include "renderers/program_interface.hpp"

#include "erhe/graphics/buffer.hpp"
#include "erhe/graphics/buffer_transfer_queue.hpp"
#include "erhe/primitive/buffer_sink.hpp"
#include "erhe/primitive/primitive_builder.hpp"
#include "erhe/raytrace/ibuffer.hpp"
#include "erhe/toolkit/verify.hpp"
#include "erhe/toolkit/profile.hpp"

namespace editor {

Mesh_memory::Mesh_memory()
    : Component{c_name}
{
}

Mesh_memory::~Mesh_memory() = default;

void Mesh_memory::connect()
{
    require<Gl_context_provider>();
    require<Program_interface  >();
}

void Mesh_memory::initialize_component()
{
    ERHE_PROFILE_FUNCTION

    const Scoped_gl_context gl_context{Component::get<Gl_context_provider>()};

    static constexpr gl::Buffer_storage_mask storage_mask{gl::Buffer_storage_mask::map_write_bit};

    constexpr size_t vertex_byte_count  = 256 * 1024 * 1024;
    constexpr size_t index_byte_count   =  64 * 1024 * 1024;

    gl_buffer_transfer_queue = std::make_unique<erhe::graphics::Buffer_transfer_queue>();

    {
        ERHE_PROFILE_SCOPE("GL VBO");

        gl_vertex_buffer = std::make_shared<erhe::graphics::Buffer>(
            gl::Buffer_target::array_buffer,
            vertex_byte_count,
            storage_mask

        );
    }

    {
        ERHE_PROFILE_SCOPE("GL IBO");

        gl_index_buffer = std::make_shared<erhe::graphics::Buffer>(
            gl::Buffer_target::element_array_buffer,
            index_byte_count,
            storage_mask
        );
    }
    gl_vertex_buffer->set_debug_label("Mesh Memory Vertex");
    gl_index_buffer ->set_debug_label("Mesh Memory Index");

    gl_buffer_sink = std::make_unique<erhe::primitive::Gl_buffer_sink>(
        *gl_buffer_transfer_queue.get(),
        *gl_vertex_buffer.get(),
        *gl_index_buffer.get()
    );

    build_info.buffer.buffer_sink = gl_buffer_sink.get();

    auto& format_info = build_info.format;
    auto& buffer_info = build_info.buffer;

    buffer_info.index_type = gl::Draw_elements_type::unsigned_int;

    format_info.features = {
        .fill_triangles   = true,
        .edge_lines       = true,
        .corner_points    = true,
        .centroid_points  = true,
        .position         = true,
        .normal           = true,
        .normal_smooth    = true,
        .tangent          = true,
        .bitangent        = true,
        .color            = true,
        .texcoord         = true,
        .id               = true
    };
    format_info.normal_style              = erhe::primitive::Normal_style::corner_normals;
    format_info.vertex_attribute_mappings = &Component::get<Program_interface>()->shader_resources->attribute_mappings;

    erhe::primitive::Primitive_builder::prepare_vertex_format(build_info);
}

auto Mesh_memory::gl_vertex_format() const -> erhe::graphics::Vertex_format&
{
    return *build_info.buffer.vertex_format.get();
}

auto Mesh_memory::gl_index_type() const -> gl::Draw_elements_type
{
    return build_info.buffer.index_type;
}

}