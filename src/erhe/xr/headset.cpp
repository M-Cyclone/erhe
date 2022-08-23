#include "erhe/xr/headset.hpp"
#include "erhe/toolkit/verify.hpp"
#include "erhe/xr/xr_instance.hpp"
#include "erhe/xr/xr_session.hpp"
#include "erhe/toolkit/profile.hpp"

namespace erhe::xr {

Headset::Headset(erhe::toolkit::Context_window* context_window)
{
    ERHE_VERIFY(context_window != nullptr);

    m_xr_instance = std::make_unique<Xr_instance>();
    if (!m_xr_instance->is_available())
    {
        m_xr_instance.reset();
        return;
    }

    m_xr_session = std::make_unique<Xr_session>(*m_xr_instance.get(), *context_window);

    m_xr_instance->get_current_interaction_profile(*m_xr_session.get());
}

Headset::~Headset()
{
}

auto Headset::controller_pose() const -> Pose
{
    return m_controller_pose;
}

auto Headset::get_hand_tracking_joint(const XrHandEXT hand, const XrHandJointEXT joint) const -> Hand_tracking_joint
{
    return m_xr_session
        ? m_xr_session->get_hand_tracking_joint(hand, joint)
        : Hand_tracking_joint{
            .location = {
                .locationFlags = 0
            },
            .velocity = {
                .velocityFlags = 0
            }
        };
}

auto Headset::get_hand_tracking_active(const XrHandEXT hand) const -> bool
{
    return m_xr_session
        ? m_xr_session->get_hand_tracking_active(hand)
        : false;
}

auto Headset::trigger_value() const -> float
{
    return m_xr_instance
        ? m_xr_instance->actions.trigger_value_state.currentState
        : 0.0f;
}

auto Headset::squeeze_click() const -> bool
{
    return m_xr_instance
        ? (m_xr_instance->actions.squeeze_click_state.currentState == XR_TRUE)
        : false;
}

auto Headset::begin_frame() -> Frame_timing
{
    ERHE_PROFILE_FUNCTION

    Frame_timing result{0, 0, false};
    if (!m_xr_instance)
    {
        return result;
    }
    if (!m_xr_instance->poll_xr_events(*m_xr_session.get()))
    {
        return result;
    }

    if (!m_xr_instance->update_actions(*m_xr_session.get()))
    {
        return result;
    }

    m_xr_session->update_hand_tracking();

    m_controller_pose.orientation = to_glm(m_xr_instance->actions.aim_pose_space_location.pose.orientation);
    m_controller_pose.position    = to_glm(m_xr_instance->actions.aim_pose_space_location.pose.position);

    auto* xr_frame_state = m_xr_session->wait_frame();
    if (xr_frame_state == nullptr)
    {
        return result;
    }

    if (!m_xr_session->begin_frame())
    {
        return result;
    }

    result.predicted_display_time   = xr_frame_state->predictedDisplayTime;
    result.predicted_display_pediod = xr_frame_state->predictedDisplayPeriod;
    result.should_render            = xr_frame_state->shouldRender;

    return result;
}

auto Headset::render(std::function<bool(Render_view&)> render_view_callback) -> bool
{
    ERHE_PROFILE_FUNCTION

    if (!m_xr_session)
    {
        return false;
    }
    if (!m_xr_session->render_frame(render_view_callback))
    {
        return false;
    }
    return true;
}

auto Headset::end_frame() -> bool
{
    ERHE_PROFILE_FUNCTION

    if (!m_xr_instance)
    {
        return false;
    }

    return m_xr_session->end_frame();
}

}
