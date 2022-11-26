#pragma once

namespace editor
{

class Render_context;
class Scene_view;

class Tool
{
public:
    [[nodiscard]] virtual auto description  () -> const char* = 0;
    [[nodiscard]] virtual auto tool_priority() const -> int { return 999; }

    virtual void tool_render            (const Render_context& context) { static_cast<void>(context); }
    virtual void cancel_ready           () {}
    virtual void tool_properties        () {}
    virtual void on_enable_state_changed() {}

    [[nodiscard]] auto is_enabled() const -> bool;
    void set_enable_state(const bool enable_state);

private:
    bool m_enabled{true};
};

} // namespace editor
