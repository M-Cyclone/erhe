#include "erhe/graphics/buffer.hpp"
#include "erhe/graphics/log.hpp"

#include <fmt/format.h>

namespace erhe::graphics
{

using erhe::log::Log;

auto Buffer::gl_name() const noexcept
-> unsigned int
{
    return m_handle.gl_name();
}

Buffer::Buffer(gl::Buffer_target       target,
               size_t                  capacity_byte_count,
               gl::Buffer_storage_mask storage_mask) noexcept
    : m_target             {target}
    , m_capacity_byte_count{capacity_byte_count}
    , m_storage_mask       {storage_mask}
{
    log_buffer.trace("Buffer::Buffer(target = {}, capacity_byte_count = {}, storage_mask = {}) name = {}\n",
                     gl::c_str(target),
                     capacity_byte_count,
                     gl::to_string(storage_mask),
                     gl_name());

    VERIFY(capacity_byte_count > 0);

    gl::named_buffer_storage(gl_name(),
                             static_cast<GLintptr>(m_capacity_byte_count),
                             nullptr,
                             storage_mask);

    Ensures(gl_name() != 0);
    Ensures(m_capacity_byte_count > 0);
}

Buffer::Buffer(gl::Buffer_target          target,
               size_t                     capacity_byte_count,
               gl::Buffer_storage_mask    storage_mask,
               gl::Map_buffer_access_mask map_buffer_access_mask) noexcept
    : m_target             {target}
    , m_capacity_byte_count{capacity_byte_count}
    , m_storage_mask       {storage_mask}
{
    log_buffer.trace("Buffer::Buffer(target = {}, capacity_byte_count = {}, storage_mask = {}, map_buffer_access_mask = {}) name = {}\n",
                     gl::c_str(target),
                     capacity_byte_count,
                     gl::to_string(storage_mask),
                     gl::to_string(map_buffer_access_mask),
                     gl_name());

    VERIFY(capacity_byte_count > 0);

    gl::named_buffer_storage(gl_name(),
                             static_cast<GLintptr>(m_capacity_byte_count),
                             nullptr,
                             storage_mask);

    map_bytes(0, capacity_byte_count, map_buffer_access_mask);

    Ensures(gl_name() != 0);
    Ensures(m_capacity_byte_count > 0);
}

auto Buffer::debug_label() const noexcept
-> const std::string&
{
    return m_debug_label;
}

auto Buffer::allocate_bytes(size_t byte_count, size_t alignment) noexcept
-> size_t
{
    while ((m_next_free_byte % alignment) != 0)
    {
        ++m_next_free_byte;
    }
    auto offset = m_next_free_byte;
    m_next_free_byte += byte_count;
    VERIFY(m_next_free_byte <= m_capacity_byte_count);
    return offset;
}

auto Buffer::map_all_bytes(gl::Map_buffer_access_mask access_mask) noexcept
-> gsl::span<std::byte>
{
    Expects(m_map.empty());
    Expects(gl_name() != 0);

    log_buffer.trace("Buffer::map_all_bytes(access_mask = {}) target = {}, name = {}\n",
                     gl::to_string(access_mask),
                     gl::c_str(m_target),
                     gl_name());
    Log::Indenter indenter;

    size_t byte_count = m_capacity_byte_count;

    m_map_byte_offset = 0;
    m_map_buffer_access_mask = access_mask;

    auto* map_pointer = reinterpret_cast<std::byte*>(gl::map_named_buffer_range(gl_name(),
                                                                                m_map_byte_offset,
                                                                                static_cast<GLsizeiptr>(byte_count),
                                                                                m_map_buffer_access_mask));
    VERIFY(map_pointer != nullptr);

    log_buffer.trace(":m_map_byte_offset = {}, m_map_byte_count = {}, m_map_pointer = {}\n",
                     m_map_byte_offset,
                     byte_count,
                     fmt::ptr(map_pointer));

    m_map = gsl::span<std::byte>(map_pointer, byte_count);

    Ensures(!m_map.empty());

    return m_map;
}

auto Buffer::map_bytes(size_t                     byte_offset,
                       size_t                     byte_count,
                       gl::Map_buffer_access_mask access_mask) noexcept
-> gsl::span<std::byte>
{
    VERIFY(byte_count > 0);
    Expects(m_map.empty());
    Expects(gl_name() != 0);

    log_buffer.trace("Buffer::map_bytes(byte_offset = {}, byte_count = {}, access_mask = {}) target = {}, name = {}\n",
                     byte_offset,
                     byte_count,
                     gl::to_string(access_mask),
                     gl::c_str(m_target),
                     gl_name());
    Log::Indenter indenter;

    VERIFY(byte_offset + byte_count <= m_capacity_byte_count);

    m_map_byte_offset = static_cast<GLsizeiptr>(byte_offset);
    m_map_buffer_access_mask = access_mask;

    auto* map_pointer = reinterpret_cast<std::byte*>(gl::map_named_buffer_range(gl_name(),
                                                                                m_map_byte_offset,
                                                                                static_cast<GLsizeiptr>(byte_count),
                                                                                m_map_buffer_access_mask));
    VERIFY(map_pointer != nullptr);

    log_buffer.trace(":m_map_byte_offset = {}, m_map_byte_count = {}, m_map_pointer = {}\n",
                     m_map_byte_offset,
                     byte_count,
                     fmt::ptr(map_pointer));

    m_map = gsl::span<std::byte>(map_pointer, byte_count);

    Ensures(!m_map.empty());

    return m_map;
}

void Buffer::unmap() noexcept
{
    Expects(!m_map.empty());
    Expects(gl_name() != 0);

    log_buffer.trace("Buffer::unmap() target = {}, byte_offset = {}, byte_count = {}, pointer = {}, name = {}\n",
                     gl::c_str(m_target),
                     m_map_byte_offset,
                     m_map.size(),
                     reinterpret_cast<intptr_t>(m_map.data()),
                     gl_name());
    Log::Indenter indented;
    Log::set_text_color(Log::Color::GREY);

    auto res = gl::unmap_named_buffer(gl_name());
    VERIFY(res == GL_TRUE);

    m_map_byte_offset = std::numeric_limits<size_t>::max();

    m_map = gsl::span<std::byte>();

    Ensures(m_map.empty());
}

void Buffer::flush_bytes(size_t byte_offset, size_t byte_count) noexcept
{
    Expects((m_map_buffer_access_mask & gl::Map_buffer_access_mask::map_flush_explicit_bit) == gl::Map_buffer_access_mask::map_flush_explicit_bit);
    Expects(gl_name() != 0);

    // unmap will do flush
    VERIFY(byte_offset + byte_count <= m_capacity_byte_count);

    log_buffer.trace("Buffer::flush(byte_offset = {}, byte_count = {}) target = {}, m_mapped_ptr = {} name = {}\n",
                     byte_offset,
                     byte_count,
                     gl::c_str(m_target),
                     reinterpret_cast<intptr_t>(m_map.data()),
                     gl_name());

    gl::flush_mapped_named_buffer_range(gl_name(),
                                        static_cast<GLintptr>(byte_offset),
                                        static_cast<GLsizeiptr>(byte_count));
}

void Buffer::dump() const noexcept
{
    Expects(gl_name() != 0);

    bool      unmap     {false};
    size_t    byte_count{m_capacity_byte_count};
    size_t    word_count{byte_count / sizeof(uint32_t)};
    int       mapped    {GL_FALSE};
    uint32_t* data      {nullptr};
    std::vector<uint32_t> storage;

    gl::get_named_buffer_parameter_iv(gl_name(), gl::Buffer_p_name::buffer_mapped, &mapped);

    if (mapped == GL_FALSE)
    {
        data = reinterpret_cast<uint32_t*>(gl::map_named_buffer_range(gl_name(),
                                                                      0,
                                                                      byte_count,
                                                                      gl::Map_buffer_access_mask::map_read_bit));
        unmap = (data != nullptr);
    }

    if (data == nullptr)
    {
        // This happens if we already had buffer mapped
        storage.resize(word_count + 1);
        data = storage.data();
        gl::get_named_buffer_sub_data(gl_name(), 0, word_count * sizeof(uint32_t), data);
    }

    for (size_t i = 0; i < word_count; ++i)
    {
        if (i % 16u == 0)
        {
            log_buffer.trace("%08x: ", static_cast<unsigned int>(i));
        }

        log_buffer.trace("%08x ", data[i]);

        if (i % 16u == 15u)
        {
            log_buffer.trace("\n");
        }
    }
    log_buffer.trace("\n");

    if (unmap)
    {
        gl::unmap_named_buffer(gl_name());
    }
}

void Buffer::flush_and_unmap_bytes(size_t byte_count) noexcept
{
    Expects(gl_name() != 0);

    bool flush_explicit = (m_map_buffer_access_mask & gl::Map_buffer_access_mask::map_flush_explicit_bit) == gl::Map_buffer_access_mask::map_flush_explicit_bit;

    log_buffer.trace("flush_and_unmap(byte_count = {}) name = {}\n", byte_count, gl_name());

    // If explicit is not selected, unmap will do full flush
    // so we do manual flush only if explicit is selected
    if (flush_explicit)
    {
        flush_bytes(0, byte_count);
    }

    unmap();
}

auto Buffer::free_capacity_bytes() const noexcept
-> size_t
{
    return m_capacity_byte_count - m_next_free_byte;
}

auto Buffer::capacity_byte_count() const noexcept
-> size_t
{
    return m_capacity_byte_count;
}

auto operator==(const Buffer& lhs, const Buffer& rhs) noexcept
-> bool
{
    return lhs.gl_name() == rhs.gl_name();
}

auto operator!=(const Buffer& lhs, const Buffer& rhs) noexcept
-> bool
{
    return !(lhs == rhs);
}

} // namespace erhe::graphics
