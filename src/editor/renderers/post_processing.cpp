#include "renderers/post_processing.hpp"
#include "renderers/programs.hpp"

#include "erhe/application/configuration.hpp"
#include "erhe/application/imgui_windows.hpp"
#include "erhe/application/graphics/gl_context_provider.hpp"
#include "erhe/application/graphics/shader_monitor.hpp"
#include "erhe/gl/wrapper_functions.hpp"
#include "erhe/graphics/debug.hpp"
#include "erhe/graphics/framebuffer.hpp"
#include "erhe/graphics/gpu_timer.hpp"
#include "erhe/graphics/opengl_state_tracker.hpp"
#include "erhe/graphics/shader_stages.hpp"
#include "erhe/graphics/texture.hpp"
#include "erhe/toolkit/profile.hpp"

#if defined(ERHE_GUI_LIBRARY_IMGUI)
#   include <imgui.h>
#endif

#include <algorithm>

namespace editor
{

#if 0
auto factorial(const int input) -> int
{
    int result = 1;
    for (int i = 1; i <= input; i++)
    {
        result = result * i;
    }
    return result;
}

// Computes the n-th coefficient from Pascal's triangle binomial coefficients.
auto binom(
    const int row_index,
    const int column_index = -1
) -> int
{
    return
        factorial(row_index) /
        (
            factorial(row_index - column_index) *
            factorial(column_index)
        );
}

class Kernel
{
public:
    std::vector<float> weights;
    std::vector<float> offsets;
};

// Compute discrete weights and factors
auto kernel_binom(
    const int taps,
    const int expand_by = 0,
    const int reduce_by = 0
) -> Kernel
{
    const auto row          = taps - 1 + (expand_by << 1);
    const auto coeffs_count = row + 1;
    const auto radius       = taps >> 1;

    // sanity check, avoid duped coefficients at center
    if ((coeffs_count & 1) == 0)
    {
        return {}; // ValueError("Duped coefficients at center")
    }

    // compute total weight
    // https://en.wikipedia.org/wiki/Power_of_two
    // TODO: seems to be not optimal ...
    int sum = 0;
    for (int x = 0; x < reduce_by; ++x)
    {
        sum += 2 * binom(row, x);
    }
    const auto total = float(1 << row) - sum;

    // compute final weights
    Kernel result;
    for (
        int x = reduce_by + radius;
        x > reduce_by - 1;
        --x
    )
    {
        result.weights.push_back(binom(row, x) / total);
    }
    for (
        int offset = 0;
        offset <= radius;
        ++offset
    )
    {
        result.offsets.push_back(static_cast<float>(offset));
    }
    return result;
}

// Compute linearly interpolated weights and factors
auto kernel_binom_linear(const Kernel& discrete_data) -> Kernel
{
    const auto& wd = discrete_data.weights;
    const auto& od = discrete_data.offsets;

    const int w_count = static_cast<int>(wd.size());

    // sanity checks
    const auto pairs = w_count - 1;
    if ((w_count & 1) == 0)
    {
        return {};
        //raise ValueError("Duped coefficients at center")
    }

    if ((pairs % 2 != 0))
    {
        return {};
        //raise ValueError("Can't perform bilinear reduction on non-paired texels")
    }

    Kernel result;
    result.weights.push_back(wd[0]);
    for (int x = 1; x < w_count - 1; x += 2)
    {
        result.weights.push_back(wd[x] + wd[x + 1]);
    }

    result.offsets.push_back(0);
    for (
        int x = 1;
        x < w_count - 1;
        x += 2
    )
    {
        int i = (x - 1) / 2;
        const float value =
            (
                od[x    ] * wd[x] +
                od[x + 1] * wd[x + 1]
            ) / result.weights[i + 1];
        result.offsets.push_back(value);
    }

    return result;
}
#endif

Post_processing::Offsets::Offsets(
    erhe::graphics::Shader_resource& block,
    const std::size_t                source_texture_count
)
    : texel_scale   {block.add_float("texel_scale"                         )->offset_in_parent()}
    , texture_count {block.add_uint ("texture_count"                       )->offset_in_parent()}
    , reserved0     {block.add_float("reserved0"                           )->offset_in_parent()}
    , reserved1     {block.add_float("reserved1"                           )->offset_in_parent()}
    , source_texture{block.add_uvec2("source_texture", source_texture_count)->offset_in_parent()}
{
}

Post_processing::Post_processing()
    : Component   {c_label}
    , Imgui_window{c_title, c_label}
    , m_parameter_block{
        "post_processing",
        0,
        erhe::graphics::Shader_resource::Type::uniform_block
    }
    , m_offsets{m_parameter_block, m_source_texture_count}
    , m_fragment_outputs{
        erhe::graphics::Fragment_output{
            .name     = "out_color",
            .type     = gl::Fragment_shader_output_type::float_vec4,
            .location = 0
        }
    }
{
}

void Post_processing::declare_required_components()
{
    m_shader_monitor = require<erhe::application::Shader_monitor>();

    require<erhe::application::Configuration      >();
    require<erhe::application::Imgui_windows      >();
    require<erhe::application::Gl_context_provider>();
}

void Post_processing::initialize_component()
{
    ERHE_PROFILE_FUNCTION

    using erhe::graphics::Shader_stages;

    const erhe::application::Scoped_gl_context gl_context{
        Component::get<erhe::application::Gl_context_provider>()
    };

    m_empty_vertex_input = std::make_unique<erhe::graphics::Vertex_input_state>();

    {
        ERHE_PROFILE_SCOPE("shader");

        const auto shader_path = fs::path("res") / fs::path("shaders");
        const fs::path vs_path         = shader_path / fs::path("post_processing.vert");
        const fs::path x_fs_path       = shader_path / fs::path("downsample_x.frag");
        const fs::path y_fs_path       = shader_path / fs::path("downsample_y.frag");
        const fs::path compose_fs_path = shader_path / fs::path("compose.frag");

        Shader_stages::Create_info x_create_info{
            .name             = "downsample_x",
            .interface_blocks = { &m_parameter_block },
            .fragment_outputs = &m_fragment_outputs,
            .shaders          = {
                { gl::Shader_type::vertex_shader,   vs_path   },
                { gl::Shader_type::fragment_shader, x_fs_path }
            }
        };
        Shader_stages::Create_info y_create_info{
            .name             = "downsample_y",
            .interface_blocks = { &m_parameter_block },
            .fragment_outputs = &m_fragment_outputs,
            .shaders          = {
                { gl::Shader_type::vertex_shader,   vs_path   },
                { gl::Shader_type::fragment_shader, y_fs_path }
            }
        };
        Shader_stages::Create_info compose_create_info{
            .name             = "compose",
            .interface_blocks = { &m_parameter_block },
            .fragment_outputs = &m_fragment_outputs,
            .shaders          = {
                { gl::Shader_type::vertex_shader,   vs_path         },
                { gl::Shader_type::fragment_shader, compose_fs_path }
            }
        };

        if (erhe::graphics::Instance::info.use_bindless_texture)
        {
            x_create_info.defines.emplace_back("ERHE_BINDLESS_TEXTURE", "1");
            y_create_info.defines.emplace_back("ERHE_BINDLESS_TEXTURE", "1");
            compose_create_info.defines.emplace_back("ERHE_BINDLESS_TEXTURE", "1");
            x_create_info      .extensions.push_back({gl::Shader_type::fragment_shader, "GL_ARB_bindless_texture"});
            y_create_info      .extensions.push_back({gl::Shader_type::fragment_shader, "GL_ARB_bindless_texture"});
            compose_create_info.extensions.push_back({gl::Shader_type::fragment_shader, "GL_ARB_bindless_texture"});
        }
        else
        {
            m_source_texture_count = std::min(
                m_source_texture_count,
                static_cast<size_t>(erhe::graphics::Instance::limits.max_texture_image_units)
            );

            m_downsample_source_texture = m_downsample_default_uniform_block.add_sampler(
                "s_source",
                gl::Uniform_type::sampler_2d,
                0
            );
            m_compose_source_textures = m_compose_default_uniform_block.add_sampler(
                "s_source_textures",
                gl::Uniform_type::sampler_2d,
                0,
                m_source_texture_count
            );

            m_dummy_texture = erhe::graphics::create_dummy_texture();
            x_create_info.default_uniform_block       = &m_downsample_default_uniform_block;
            y_create_info.default_uniform_block       = &m_downsample_default_uniform_block;
            compose_create_info.default_uniform_block = &m_compose_default_uniform_block;
        }

        Shader_stages::Prototype x_prototype      {x_create_info};
        Shader_stages::Prototype y_prototype      {y_create_info};
        Shader_stages::Prototype compose_prototype{compose_create_info};
        m_downsample_x_shader_stages = std::make_unique<Shader_stages>(std::move(x_prototype));
        m_downsample_y_shader_stages = std::make_unique<Shader_stages>(std::move(y_prototype));
        m_compose_shader_stages      = std::make_unique<Shader_stages>(std::move(compose_prototype));
        if (m_shader_monitor)
        {
            m_shader_monitor->add(x_create_info,       m_downsample_x_shader_stages.get());
            m_shader_monitor->add(y_create_info,       m_downsample_y_shader_stages.get());
            m_shader_monitor->add(compose_create_info, m_compose_shader_stages.get());
        }
    }

    m_downsample_x_pipeline.data =
    {
        .name           = "Post Procesisng Downsample X",
        .shader_stages  = m_downsample_x_shader_stages.get(),
        .vertex_input   = m_empty_vertex_input.get(),
        .input_assembly = erhe::graphics::Input_assembly_state::triangle_fan,
        .rasterization  = erhe::graphics::Rasterization_state::cull_mode_none,
        .depth_stencil  = erhe::graphics::Depth_stencil_state::depth_test_disabled_stencil_test_disabled,
        .color_blend    = erhe::graphics::Color_blend_state::color_blend_disabled,
    };
    m_downsample_y_pipeline.data =
    {
        .name           = "Post Processing Downsample Y",
        .shader_stages  = m_downsample_y_shader_stages.get(),
        .vertex_input   = m_empty_vertex_input.get(),
        .input_assembly = erhe::graphics::Input_assembly_state::triangle_fan,
        .rasterization  = erhe::graphics::Rasterization_state::cull_mode_none,
        .depth_stencil  = erhe::graphics::Depth_stencil_state::depth_test_disabled_stencil_test_disabled,
        .color_blend    = erhe::graphics::Color_blend_state::color_blend_disabled,
    };
    m_compose_pipeline.data =
    {
        .name           = "Post Processing Compose",
        .shader_stages  = m_compose_shader_stages.get(),
        .vertex_input   = m_empty_vertex_input.get(),
        .input_assembly = erhe::graphics::Input_assembly_state::triangle_fan,
        .rasterization  = erhe::graphics::Rasterization_state::cull_mode_none,
        .depth_stencil  = erhe::graphics::Depth_stencil_state::depth_test_disabled_stencil_test_disabled,
        .color_blend    = erhe::graphics::Color_blend_state::color_blend_disabled,
    };

    create_frame_resources();

    m_gpu_timer = std::make_unique<erhe::graphics::Gpu_timer>("Post_processing");

    get<erhe::application::Imgui_windows>()->register_imgui_window(this);
}

void Post_processing::post_initialize()
{
    m_pipeline_state_tracker = get<erhe::graphics::OpenGL_state_tracker>();
    m_programs               = get<Programs>();
}

Rendertarget::Rendertarget()
{
}

Rendertarget::Rendertarget(
    const std::string& label,
    const int          width,
    const int          height
)
{
    using erhe::graphics::Framebuffer;
    using erhe::graphics::Texture;

    texture = std::make_shared<Texture>(
        Texture::Create_info{
            .target          = gl::Texture_target::texture_2d,
            .internal_format = gl::Internal_format::rgba16f,
            //.internal_format = gl::Internal_format::rgba32f,
            .sample_count    = 0,
            .width           = width,
            .height          = height
        }
    );
    texture->set_debug_label(label);
    //const float clear_value[4] = { 1.0f, 0.0f, 1.0f, 1.0f };
    //gl::clear_tex_image(
    //    texture->gl_name(),
    //    0,
    //    gl::Pixel_format::rgba,
    //    gl::Pixel_type::float_,
    //    &clear_value[0]
    //);

    Framebuffer::Create_info create_info;
    create_info.attach(gl::Framebuffer_attachment::color_attachment0, texture.get());
    framebuffer = std::make_unique<Framebuffer>(create_info);
    framebuffer->set_debug_label(label);
}

void Rendertarget::bind_framebuffer()
{
    ERHE_PROFILE_FUNCTION

    {
        ERHE_PROFILE_SCOPE("bind");
        gl::bind_framebuffer(gl::Framebuffer_target::draw_framebuffer, framebuffer->gl_name());
    }

    static constexpr std::string_view c_invalidate_framebuffer{"invalidate framebuffer"};

    ERHE_PROFILE_GPU_SCOPE(c_invalidate_framebuffer)

    {
        ERHE_PROFILE_SCOPE("viewport");
        gl::viewport      (0, 0, texture->width(), texture->height());
    }

    //{
    //    ERHE_PROFILE_SCOPE("invalidate");
    //    const auto invalidate_attachment = gl::Invalidate_framebuffer_attachment::color_attachment0;
    //    gl::invalidate_framebuffer(gl::Framebuffer_target::draw_framebuffer, 1, &invalidate_attachment);
    //}
}

void Post_processing::create_frame_resources()
{
    for (size_t slot = 0; slot < s_frame_resources_count; ++slot)
    {
        m_frame_resources.emplace_back(
            m_parameter_block.size_bytes(),
            100,
            slot
        );
    }
}

void Post_processing::next_frame()
{
    m_current_frame_resource_slot = (m_current_frame_resource_slot + 1) % s_frame_resources_count;
    m_parameter_writer.reset();
}

auto Post_processing::current_frame_resources() -> Frame_resources&
{
    return m_frame_resources[m_current_frame_resource_slot];
}

void Post_processing::imgui()
{
#if defined(ERHE_GUI_LIBRARY_IMGUI)
    ERHE_PROFILE_FUNCTION

    //ImGui::DragInt("Taps",   &m_taps,   1.0f, 1, 32);
    //ImGui::DragInt("Expand", &m_expand, 1.0f, 0, 32);
    //ImGui::DragInt("Reduce", &m_reduce, 1.0f, 0, 32);
    //ImGui::Checkbox("Linear", &m_linear);

    //const auto discrete = kernel_binom(m_taps, m_expand, m_reduce);
    //if (ImGui::TreeNodeEx("Discrete", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    //{
    //    for (size_t i = 0; i < discrete.weights.size(); ++i)
    //    {
    //        ImGui::Text(
    //            "W: %.3f O: %.3f",
    //            discrete.weights.at(i),
    //            discrete.offsets.at(i)
    //        );
    //    }
    //    ImGui::TreePop();
    //}
    //if (m_linear)
    //{
    //    const auto linear = kernel_binom_linear(discrete);
    //    if (ImGui::TreeNodeEx("Linear", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    //    {
    //        for (size_t i = 0; i < linear.weights.size(); ++i)
    //        {
    //            ImGui::Text(
    //                "W: %.3f O: %.3f",
    //                linear.weights.at(i),
    //                linear.offsets.at(i)
    //            );
    //        }
    //        ImGui::TreePop();
    //    }
    //}

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0f, 0.0f});
    for (auto& rendertarget : m_rendertargets)
    {
        if (
            !rendertarget.texture                ||
            (rendertarget.texture->width () < 1) ||
            (rendertarget.texture->height() < 1)
        )
        {
            continue;
        }

        image(
            rendertarget.texture,
            rendertarget.texture->width (),
            rendertarget.texture->height()
        );
    }
    ImGui::PopStyleVar();
#endif
}

[[nodiscard]] auto Post_processing::get_output() -> std::shared_ptr<erhe::graphics::Texture>
{
    if (m_rendertargets.empty())
    {
        return {};
    }
    return m_rendertargets.front().texture;
}

static constexpr std::string_view c_post_processing{"Post_processing"};

void Post_processing::post_process(
    erhe::graphics::Texture* source_texture
)
{
    ERHE_PROFILE_FUNCTION
    ERHE_PROFILE_GPU_SCOPE(c_post_processing)

    if (
        (source_texture == nullptr)    ||
        (source_texture->width () < 1) ||
        (source_texture->height() < 1)
    )
    {
        return;
    }

    erhe::graphics::Scoped_debug_group pass_scope{"Post Processing"};
    erhe::graphics::Scoped_gpu_timer   timer     {*m_gpu_timer.get()};

    const auto& configuration = get<erhe::application::Configuration>();

    if (
        (m_source_width  != source_texture->width ()) ||
        (m_source_height != source_texture->height())
    )
    {
        m_source_width  = source_texture->width ();
        m_source_height = source_texture->height();
        m_rendertargets.clear();

        int width  = source_texture->width ();
        int height = source_texture->height();
        if (configuration->imgui.enabled)
        {
            m_rendertargets.emplace_back("Post Processing Compose", width, height);
        }
        else
        {
            m_rendertargets.emplace_back();
        }

        for (;;)
        {
            if (width > 1)
            {
                width = width / 2;
                m_rendertargets.emplace_back("Post Processing Downsample X", width, height);
                if ((width + height) == 2)
                {
                    break;
                }
            }
            if (height > 1)
            {
                height = height / 2;
                m_rendertargets.emplace_back("Post Processing Downsample Y", width, height);
                if ((width + height) == 2)
                {
                    break;
                }
            }
        }
    }

    {
        std::size_t i      = 1;
        int         width  = source_texture->width ();
        int         height = source_texture->height();
        const erhe::graphics::Texture* source = source_texture;
        for (;;)
        {
            if (width > 1)
            {
                width = width / 2;
                erhe::graphics::Scoped_debug_group downsample_scope{"Downsample X"};
                downsample(source, m_rendertargets.at(i), m_downsample_x_pipeline);
                source = m_rendertargets.at(i).texture.get();
                i++;
                if ((width + height) == 2)
                {
                    break;
                }
            }

            if (height > 1)
            {
                height = height / 2;
                erhe::graphics::Scoped_debug_group downsample_scope{"Downsample Y"};
                downsample(source, m_rendertargets.at(i), m_downsample_y_pipeline);
                source = m_rendertargets.at(i).texture.get();
                i++;
                if ((width + height) == 2)
                {
                    break;
                }
            }
        }
    }

    compose(source_texture);
}

static constexpr std::string_view c_downsample{"Post_processing::downsample"};

void Post_processing::downsample(
    const erhe::graphics::Texture*  source_texture,
    Rendertarget&                   rendertarget,
    const erhe::graphics::Pipeline& pipeline
)
{
    ERHE_PROFILE_FUNCTION
    ERHE_PROFILE_GPU_SCOPE(c_downsample)

    auto& parameter_buffer   = current_frame_resources().parameter_buffer;
    auto  parameter_gpu_data = parameter_buffer.map();

    m_parameter_writer.begin(parameter_buffer.target());

    std::byte* const          start      = parameter_gpu_data.data() + m_parameter_writer.write_offset;
    const std::size_t         byte_count = parameter_gpu_data.size_bytes();
    const std::size_t         word_count = byte_count / sizeof(float);
    const gsl::span<float>    gpu_float_data{reinterpret_cast<float*   >(start), word_count};
    const gsl::span<uint32_t> gpu_uint_data {reinterpret_cast<uint32_t*>(start), word_count};

    const uint64_t handle = erhe::graphics::get_handle(
        *source_texture,
        *m_programs->linear_sampler.get()
    );

    gpu_float_data[m_parameter_writer.write_offset + m_offsets.texel_scale       ] = 1.0f / source_texture->width();
    gpu_uint_data [m_parameter_writer.write_offset + m_offsets.texture_count     ] = 1;
    gpu_float_data[m_parameter_writer.write_offset + m_offsets.reserved0         ] = 0.0f;
    gpu_float_data[m_parameter_writer.write_offset + m_offsets.reserved1         ] = 0.0f;
    gpu_uint_data [m_parameter_writer.write_offset + m_offsets.source_texture    ] = (handle & 0xffffffffu);
    gpu_uint_data [m_parameter_writer.write_offset + m_offsets.source_texture + 1] = handle >> 32u;
    m_parameter_writer.write_offset += m_parameter_block.size_bytes();
    m_parameter_writer.end();

    rendertarget.bind_framebuffer();

    m_pipeline_state_tracker->execute(pipeline);

    {
        ERHE_PROFILE_SCOPE("bind parameter buffer");

        gl::bind_buffer_range(
            parameter_buffer.target(),
            static_cast<GLuint>    (m_parameter_block.binding_point()),
            static_cast<GLuint>    (parameter_buffer.gl_name()),
            static_cast<GLintptr>  (m_parameter_writer.range.first_byte_offset),
            static_cast<GLsizeiptr>(m_parameter_writer.range.byte_count)
        );
    }

    if (erhe::graphics::Instance::info.use_bindless_texture)
    {
        ERHE_PROFILE_SCOPE("make input texture resident");
        gl::make_texture_handle_resident_arb(handle);
    }
    else
    {
        gl::bind_texture_unit(0, source_texture->gl_name());
        gl::bind_sampler     (0, m_programs->linear_sampler->gl_name());
    }

    {
        static constexpr std::string_view c_draw_arrays{"draw arrays"};

        ERHE_PROFILE_GPU_SCOPE(c_draw_arrays)
        gl::draw_arrays(pipeline.data.input_assembly.primitive_topology, 0, 3);
    }
    {
        ERHE_PROFILE_SCOPE("bind fbo");
        gl::bind_framebuffer(gl::Framebuffer_target::draw_framebuffer, 0);
    }

    if (erhe::graphics::Instance::info.use_bindless_texture)
    {
        ERHE_PROFILE_SCOPE("make input texture non resident");
        gl::make_texture_handle_non_resident_arb(handle);
    }
}

static constexpr std::string_view c_compose{"Post_processing::compose"};

void Post_processing::compose(const erhe::graphics::Texture* source_texture)
{
    ERHE_PROFILE_FUNCTION
    ERHE_PROFILE_GPU_SCOPE(c_compose)

    auto& parameter_buffer   = current_frame_resources().parameter_buffer;
    auto  parameter_gpu_data = parameter_buffer.map();

    m_parameter_writer.begin(parameter_buffer.target());

    std::byte* const          start      = parameter_gpu_data.data();
    const std::size_t         byte_count = parameter_gpu_data.size_bytes();
    const std::size_t         word_count = byte_count / sizeof(float);
    const gsl::span<float>    gpu_float_data{reinterpret_cast<float*   >(start), word_count};
    const gsl::span<uint32_t> gpu_uint_data {reinterpret_cast<uint32_t*>(start), word_count};

    using erhe::graphics::write;
    //const uint32_t texture_count = static_cast<uint32_t>(m_rendertargets.size());

    write(gpu_float_data, m_parameter_writer.write_offset + m_offsets.texel_scale,   0.0f);
    write(gpu_uint_data,  m_parameter_writer.write_offset + m_offsets.texture_count, static_cast<uint32_t>(m_rendertargets.size()));
    write(gpu_float_data, m_parameter_writer.write_offset + m_offsets.reserved0,     0.0f);
    write(gpu_float_data, m_parameter_writer.write_offset + m_offsets.reserved1,     0.0f);

    constexpr std::size_t uvec4_size = 4 * sizeof(uint32_t);

    {
        ERHE_PROFILE_SCOPE("make textures resident");
        {
            const uint64_t handle = erhe::graphics::get_handle(
                *source_texture,
                *m_programs->linear_sampler.get()
            );

            const uint32_t texture_handle[2] =
            {
                static_cast<uint32_t>((handle & 0xffffffffu)),
                static_cast<uint32_t>(handle >> 32u)
            };
            const gsl::span<const uint32_t> texture_handle_cpu_data{&texture_handle[0], 2};

            if (erhe::graphics::Instance::info.use_bindless_texture)
            {
                gl::make_texture_handle_resident_arb(handle);
            }
            else
            {
                gl::bind_texture_unit(0, source_texture->gl_name());
                gl::bind_sampler     (0, m_programs->nearest_sampler->gl_name()); // check if this is good
            }

            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + m_offsets.source_texture    , texture_handle_cpu_data);
            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + m_offsets.source_texture + 2, 0);
            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + m_offsets.source_texture + 3, 0);
        }

        const auto end = std::min(m_rendertargets.size(), m_source_texture_count);
        for (size_t i = 1; i < end; ++i)
        {
            const auto&    source = m_rendertargets.at(i);
            const uint64_t handle = erhe::graphics::get_handle(
                *source.texture.get(),
                *m_programs->linear_sampler.get()
            );

            const uint32_t texture_handle[2] =
            {
                static_cast<uint32_t>((handle & 0xffffffffu)),
                static_cast<uint32_t>(handle >> 32u)
            };
            const gsl::span<const uint32_t> texture_handle_cpu_data{&texture_handle[0], 2};

            if (erhe::graphics::Instance::info.use_bindless_texture)
            {
                gl::make_texture_handle_resident_arb(handle);
            }
            else
            {
                gl::bind_texture_unit(static_cast<GLuint>(i), source.texture->gl_name());
                gl::bind_sampler     (static_cast<GLuint>(i), m_programs->linear_sampler->gl_name());
            }

            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + (i * uvec4_size) + m_offsets.source_texture    , texture_handle_cpu_data);
            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + (i * uvec4_size) + m_offsets.source_texture + 2, 0);
            write<uint32_t>(gpu_uint_data, m_parameter_writer.write_offset + (i * uvec4_size) + m_offsets.source_texture + 3, 0);
        }
        if (!erhe::graphics::Instance::info.use_bindless_texture)
        {
            for (size_t i = end; i < m_source_texture_count; ++i)
            {
                gl::bind_texture_unit(static_cast<GLuint>(i), m_dummy_texture->gl_name());
                gl::bind_sampler     (static_cast<GLuint>(i), m_programs->nearest_sampler->gl_name());
            }
        }

    }
    m_parameter_writer.write_offset += m_parameter_block.size_bytes();
    m_parameter_writer.end();

    const auto& configuration = get<erhe::application::Configuration>();

    if (configuration->imgui.enabled)
    {
        auto& rendertarget = m_rendertargets.at(0);
        rendertarget.bind_framebuffer();
    }
    else
    {
        // Compose output directly to default framebuffer
        gl::bind_framebuffer(gl::Framebuffer_target::draw_framebuffer, 0);
        gl::clear_color     (0.0f, 0.0f, 0.0f, 0.0f);
        gl::clear           (gl::Clear_buffer_mask::color_buffer_bit);
        gl::viewport        (0, 0, source_texture->width(), source_texture->height());
    }

    const auto& pipeline = m_compose_pipeline;
    m_pipeline_state_tracker->execute(pipeline);

    {
        ERHE_PROFILE_SCOPE("bind parameter buffer");
        gl::bind_buffer_range(
            parameter_buffer.target(),
            static_cast<GLuint>    (m_parameter_block.binding_point()),
            static_cast<GLuint>    (parameter_buffer.gl_name()),
            static_cast<GLintptr>  (m_parameter_writer.range.first_byte_offset),
            static_cast<GLsizeiptr>(m_parameter_writer.range.byte_count)
        );
    }

    {
        ERHE_PROFILE_SCOPE("draw arrays");
        gl::draw_arrays(pipeline.data.input_assembly.primitive_topology, 0, 3);
    }

    {
        ERHE_PROFILE_SCOPE("unbind fbo");
        gl::bind_framebuffer(gl::Framebuffer_target::draw_framebuffer, 0);
    }

    if (erhe::graphics::Instance::info.use_bindless_texture)
    {
        ERHE_PROFILE_SCOPE("make textures non resident");

        {
            const uint64_t handle = erhe::graphics::get_handle(
                *source_texture,
                *m_programs->linear_sampler.get()
            );

            gl::make_texture_handle_non_resident_arb(handle);
        }

        for (
            std::size_t i = 1, end = std::min(m_rendertargets.size(), size_t{32});
            i < end;
            ++i
        )
        {
            const auto&    source = m_rendertargets.at(i);
            const uint64_t handle = erhe::graphics::get_handle(
                *source.texture.get(),
                *m_programs->linear_sampler.get()
            );

            gl::make_texture_handle_non_resident_arb(handle);
        }
    }
}

} // namespace editor
