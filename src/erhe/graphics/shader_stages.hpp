#pragma once

#include "erhe/graphics/shader_resource.hpp"
#include "erhe/graphics/gl_objects.hpp"
#include "erhe/toolkit/filesystem.hpp"
#include "erhe/toolkit/optional.hpp"

#include <map>

namespace erhe::graphics
{

class Fragment_outputs;
class Shader_resource;
class Vertex_attribute_mappings;
class Gl_shader;

class Shader_stage_extension
{
public:
    gl::Shader_type shader_stage;
    std::string     extension;
};

// Shader program
class Shader_stages
{
public:

    // Contains all necessary information for creating a shader program
    class Create_info
    {
    public:
        class Shader_stage
        {
        public:
            // user-provided constructors are needed for vector::emplace_back()
            // Sadly, this disables using designated initializers
            Shader_stage(
                gl::Shader_type        type,
                const std::string_view source
            )
                : type  {type}
                , source{source}
            {
            }

            Shader_stage(
                gl::Shader_type type,
                const fs::path  path
            )
                : type{type}
                , path{path}
            {
            }

            gl::Shader_type type;
            std::string     source;
            fs::path        path;
        };

        // Adds #version, #extensions, #defines, fragment outputs, uniform blocks, samplers,
        // and source (possibly read from file).
        [[nodiscard]] auto final_source           (const Shader_stage& shader) const -> std::string;
        [[nodiscard]] auto attributes_source      () const -> std::string;
        [[nodiscard]] auto fragment_outputs_source() const -> std::string;
        [[nodiscard]] auto struct_types_source    () const -> std::string;
        [[nodiscard]] auto interface_blocks_source() const -> std::string;
        [[nodiscard]] auto interface_source       () const -> std::string;

        void add_interface_block(gsl::not_null<const Shader_resource*> uniform_block);

        std::string                                      name;

        std::vector<std::string>                         pragmas                       {};
        std::vector<std::pair<std::string, std::string>> defines                       {};
        std::vector<Shader_stage_extension>              extensions                    {};
        std::vector<const Shader_resource*>              interface_blocks              {};
        std::vector<const Shader_resource*>              struct_types                  {};
        const Vertex_attribute_mappings*                 vertex_attribute_mappings     {nullptr};
        const Fragment_outputs*                          fragment_outputs              {nullptr};
        const Shader_resource*                           default_uniform_block         {nullptr}; // contains sampler uniforms
        std::vector<std::string>                         transform_feedback_varyings   {};
        gl::Transform_feedback_buffer_mode               transform_feedback_buffer_mode{gl::Transform_feedback_buffer_mode::separate_attribs};
        std::vector<Shader_stage>                        shaders                       {};
        bool                                             dump_reflection               {false};
        bool                                             dump_interface                {false};
        bool                                             dump_final_source             {false};
    };

    class Prototype final
    {
    public:
        explicit Prototype(const Create_info& create_info);
        ~Prototype        () noexcept = default;
        Prototype         (const Prototype&) = delete;
        void operator=    (const Prototype&) = delete;

        [[nodiscard]] auto is_valid() const -> bool
        {
            return m_link_succeeded;
        }

        void dump_reflection() const;

    private:
        [[nodiscard]] static auto try_compile_shader(
            const Shader_stages::Create_info&               create_info,
            const Shader_stages::Create_info::Shader_stage& shader
        ) -> nonstd::optional<Gl_shader>;

        friend class Shader_stages;

        std::string            m_name;
        Gl_program             m_handle;
        std::vector<Gl_shader> m_attached_shaders;
        bool                   m_link_succeeded{false};

        Shader_resource        m_default_uniform_block;
        std::map<std::string, Shader_resource, std::less<>> m_resources;
    };

    // Creates Shader_stages by consuming prototype
    explicit Shader_stages(Prototype&& prototype);

    // Reloads program by consuming prototype
    void reload(Prototype&& prototype);

    [[nodiscard]] auto name() const -> const std::string&
    {
        return m_name;
    }

    [[nodiscard]] auto gl_name() const -> unsigned int;

private:
    [[nodiscard]] static auto format(const std::string& source) -> std::string;

    std::string            m_name;
    Gl_program             m_handle;
    std::vector<Gl_shader> m_attached_shaders;
};

class Shader_stages_hash
{
public:
    [[nodiscard]] auto operator()(const Shader_stages& program) const noexcept -> size_t
    {
        return static_cast<size_t>(program.gl_name());
    }
};

[[nodiscard]] auto operator==(
    const Shader_stages& lhs,
    const Shader_stages& rhs
) noexcept -> bool;

[[nodiscard]] auto operator!=(
    const Shader_stages& lhs,
    const Shader_stages& rhs
) noexcept -> bool;


class Shader_stages_tracker
{
public:
    void reset  ();
    void execute(const Shader_stages* state);

private:
    unsigned int m_last{0};
};

} // namespace erhe::graphics
