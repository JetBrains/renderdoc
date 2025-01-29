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
  debug_session->get_currentStack().advise(lifetime, [this](auto const &stack) {
    frames.push_back(stack);
  });
}
} // namespace jetbrains::renderdoc