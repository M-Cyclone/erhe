#pragma once

#include "erhe/components/components.hpp"

#include "concurrentqueue.h"

#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace erhe::graphics
{
    class OpenGL_state_tracker;
}

namespace erhe::toolkit
{
    class Context_window;
}

namespace erhe::application {

class Gl_worker_context
{
public:
    int                            id{0};
    erhe::toolkit::Context_window* context{nullptr};
};

class Gl_context_provider
    : public erhe::components::Component
{
public:
    static constexpr std::string_view c_type_name{"Gl_context_provider"};
    static constexpr uint32_t c_type_hash = compiletime_xxhash::xxh32(c_type_name.data(), c_type_name.size(), {});

    Gl_context_provider ();
    ~Gl_context_provider() noexcept override;
    Gl_context_provider (const Gl_context_provider&) = delete;
    void operator=      (const Gl_context_provider&) = delete;
    Gl_context_provider (Gl_context_provider&&)      = delete;
    void operator=      (Gl_context_provider&&)      = delete;

    // Implements Component
    [[nodiscard]] auto get_type_hash() const -> uint32_t override { return c_type_hash; }

    // Public API
    [[nodiscard]] auto acquire_gl_context() -> Gl_worker_context;
    void release_gl_context     (Gl_worker_context context);
    void provide_worker_contexts(
        const std::shared_ptr<erhe::graphics::OpenGL_state_tracker>& opengl_state_tracker,
        erhe::toolkit::Context_window*                               main_window,
        std::function<bool()>                                        worker_contexts_still_needed_callback
    );

private:
    std::shared_ptr<erhe::graphics::OpenGL_state_tracker>       m_opengl_state_tracker;
    erhe::toolkit::Context_window*                              m_main_window{nullptr};
    std::thread::id                                             m_main_thread_id;
    std::mutex                                                  m_mutex;
    std::condition_variable                                     m_condition_variable;
    moodycamel::ConcurrentQueue<Gl_worker_context>              m_worker_context_pool;
    std::vector<std::shared_ptr<erhe::toolkit::Context_window>> m_contexts;
};

class Scoped_gl_context
{
public:
    explicit Scoped_gl_context(const std::shared_ptr<Gl_context_provider>& context_provider);
    ~Scoped_gl_context        () noexcept;
    Scoped_gl_context         (const Scoped_gl_context&) = delete;
    auto operator=            (const Scoped_gl_context&) = delete;
    Scoped_gl_context         (Scoped_gl_context&&)      = delete;
    auto operator=            (Scoped_gl_context&&)      = delete;

private:
    std::shared_ptr<Gl_context_provider> m_context_provider;
    Gl_worker_context                    m_context;
};

} // namespace erhe::application
