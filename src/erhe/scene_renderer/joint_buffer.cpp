// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "erhe/scene_renderer/joint_buffer.hpp"

#include "erhe/configuration/configuration.hpp"
#include "erhe/scene/node.hpp"
#include "erhe/scene/skin.hpp"
#include "erhe/scene_renderer/scene_renderer_log.hpp"
#include "erhe/toolkit/math_util.hpp"
#include "erhe/toolkit/profile.hpp"
#include "erhe/toolkit/verify.hpp"

namespace erhe::scene_renderer
{

Joint_interface::Joint_interface(
    erhe::graphics::Instance& graphics_instance
)
    : joint_block{
        graphics_instance,
        "joint",
        4,
        erhe::graphics::Shader_resource::Type::shader_storage_block
    }
    , joint_struct{graphics_instance, "Joint"}
{
    auto ini = erhe::configuration::get_ini("erhe.ini", "renderer");
    ini->get("max_joint_count", max_joint_count);

    offsets.debug_joint_indices = joint_block.add_uvec4("debug_joint_indices")->offset_in_parent();
    offsets.debug_joint_colors  = joint_block.add_vec4 ("debug_joint_colors", max_joint_count)->offset_in_parent();
    offsets.joint = {
        .world_from_bind          = joint_struct.add_mat4("world_from_bind"         )->offset_in_parent(),
        .world_from_bind_cofactor = joint_struct.add_mat4("world_from_bind_cofactor")->offset_in_parent()
    };

    offsets.joint_struct = joint_block.add_struct("joints", &joint_struct, erhe::graphics::Shader_resource::unsized_array)->offset_in_parent();
}

Joint_buffer::Joint_buffer(
    erhe::graphics::Instance& graphics_instance,
    Joint_interface&          joint_interface
)
    : Multi_buffer       {graphics_instance, "joint"}
    , m_graphics_instance{graphics_instance}
    , m_joint_interface  {joint_interface}
{
    Multi_buffer::allocate(
        gl::Buffer_target::shader_storage_buffer,
        m_joint_interface.joint_block.binding_point(),
        // TODO Separate update joint (and other) buffers outside composer renderpasses. Also consider multiple viewports
        16 * m_joint_interface.offsets.joint_struct + m_joint_interface.joint_struct.size_bytes() * m_joint_interface.max_joint_count
    );
}

auto Joint_buffer::update(
    const glm::uvec4&                                          debug_joint_indices,
    const gsl::span<glm::vec4>&                                debug_joint_colors,
    const gsl::span<const std::shared_ptr<erhe::scene::Skin>>& skins
) -> erhe::renderer::Buffer_range
{
    ERHE_PROFILE_FUNCTION();

    SPDLOG_LOGGER_TRACE(
        log_render,
        "skins.size() = {}, m_writer.write_offset = {}",
        skins.size(),
        m_writer.write_offset
    );

    std::size_t joint_count = 0;
    std::size_t skin_index = 0;
    for (const auto& skin : skins) {
        ERHE_VERIFY(skin);
        ++skin_index;

        const auto& skin_data = skin->skin_data;
        joint_count += skin_data.joints.size();
    }

    auto&             buffer             = current_buffer();
    const auto        entry_size         = m_joint_interface.joint_struct.size_bytes();
    const auto&       offsets            = m_joint_interface.offsets;
    const std::size_t max_byte_count     = offsets.joint_struct + joint_count * entry_size;
    const auto        primitive_gpu_data = m_writer.begin(&buffer, max_byte_count);

    using erhe::graphics::as_span;
    using erhe::graphics::write;

    write(primitive_gpu_data, m_writer.write_offset + offsets.debug_joint_indices, as_span(debug_joint_indices));

    if (!debug_joint_colors.empty()) {
        uint32_t color_index = 0;
        for (auto& color : debug_joint_colors) {
            write(
                primitive_gpu_data,
                m_writer.write_offset + offsets.debug_joint_colors + (color_index * 4 * sizeof(float)),
                as_span(color)
            );
            ++color_index;
        }
    }

    m_writer.write_offset += offsets.joint_struct;

    uint32_t joint_index = 0;
    for (auto& skin : skins) {
        ERHE_VERIFY(skin);

        if ((m_writer.write_offset + entry_size) > m_writer.write_end) {
            log_render->critical("joint buffer capacity {} exceeded", buffer.capacity_byte_count());
            ERHE_FATAL("joint buffer capacity exceeded");
            break;
        }

        auto& skin_data = skin->skin_data;
        skin_data.joint_buffer_index = joint_index;
        for (std::size_t i = 0, end_i = skin->skin_data.joints.size(); i < end_i; ++i) {
            const auto&     joint           = skin->skin_data.joints[i];
            const glm::mat4 joint_from_bind = skin->skin_data.inverse_bind_matrices[i];
            if ((m_writer.write_offset + entry_size) > m_writer.write_end) {
                log_render->critical("joint buffer capacity {} exceeded", buffer.capacity_byte_count());
                ERHE_FATAL("joint buffer capacity exceeded");
                break;
            }

            const glm::mat4 world_from_joint = joint->world_from_node();
            const glm::mat4 world_from_bind  = world_from_joint * joint_from_bind;

            // TODO Use compute shader
            const glm::mat4 world_from_bind_cofactor = erhe::toolkit::compute_cofactor(world_from_bind);

            write(primitive_gpu_data, m_writer.write_offset + offsets.joint.world_from_bind,          as_span(world_from_bind         ));
            write(primitive_gpu_data, m_writer.write_offset + offsets.joint.world_from_bind_cofactor, as_span(world_from_bind_cofactor));
            m_writer.write_offset += entry_size;
            ++joint_index;
            ERHE_VERIFY(m_writer.write_offset <= m_writer.write_end);
        }
    }

    m_writer.end();

    SPDLOG_LOGGER_TRACE(log_draw, "wrote {} entries to joint buffer", joint_index);

    return m_writer.range;
}

} //namespace erhe::scene_renderer
