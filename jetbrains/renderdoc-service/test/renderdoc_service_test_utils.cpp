#include "renderdoc_service_test_utils.h"
#include "RenderDocServiceApi.h"

namespace jetbrains::renderdoc {

LineTracker::LineTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session) {
  debug_session->get_currentStack().advise(lifetime, [this](auto const &stack) {
    if (stack != nullptr) {
      lines.push_back(static_cast<int32_t>(stack->get_lineStart()));
    } else {
      lines.push_back(-1);
    }
  });
}

FrameTracker::FrameTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session) {
  debug_session->get_currentStack().advise(lifetime, [this, debug_session](auto const &stack) {
    if (const auto draw_call = debug_session->get_drawCallSession().get();
      !draw_call || draw_call_sessions.empty() || draw_call != draw_call_sessions.back()) {
      draw_call_sessions.emplace_back(draw_call);
      draw_call_id_changes.emplace_back(frames.size(), stack ? static_cast<int64_t>(stack->get_drawCallId()) : -1);
    }
    frames.push_back(stack);
  });
}
} // namespace jetbrains::renderdoc