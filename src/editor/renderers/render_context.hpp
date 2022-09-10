#pragma once

#include "erhe/scene/viewport.hpp"

namespace erhe::scene
{
    class Camera;
}

namespace editor
{

class Scene_viewport;
class Viewport_config;
class Viewport_window;

class Render_context
{
public:
    Scene_viewport*                scene_viewport        {nullptr};
    Viewport_window*               viewport_window       {nullptr};
    Viewport_config*               viewport_config       {nullptr};
    erhe::scene::Camera*           camera                {nullptr};
    erhe::scene::Viewport          viewport              {0, 0, 0, 0, true};
    erhe::graphics::Shader_stages* override_shader_stages{nullptr};
};

} // namespace editor
