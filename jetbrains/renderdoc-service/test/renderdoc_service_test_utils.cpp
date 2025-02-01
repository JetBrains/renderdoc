#include "renderdoc_service_test_utils.h"
#include "RenderDocServiceApi.h"

namespace jetbrains::renderdoc {

RdcDebugInput::RdcDebugInput(model::RdcDebugVertexInput &&vertex) : vertex(std::move(vertex)), type(Vertex) {}
RdcDebugInput::RdcDebugInput(model::RdcDebugPixelInput &&pixel) : pixel(std::move(pixel)), type(Pixel) {}

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

void assert_session_finishes_immediately(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const RdcDebugInput &input, bool debug_single_call) {
  const auto session_lifetime = lifetime.create_nested();
  rd::Wrapper<RenderDocDebugSession> debug_session;
  if (input.type == RdcDebugInput::Vertex) {
    if (debug_single_call)
      debug_session = replay->debug_vertex(session_lifetime, input.vertex);
    else
      debug_session = replay->try_debug_vertex(session_lifetime, input.vertex);
  } else {
    if (debug_single_call)
      debug_session = replay->debug_pixel(session_lifetime, input.pixel);
    else
      debug_session = replay->try_debug_pixel(session_lifetime, input.pixel);
  }

  const FrameTracker frame_tracker(lifetime, debug_session);
  assert(frame_tracker.frames == std::vector({rd::Wrapper<model::RdcDebugStack>(nullptr)}));
  assert(frame_tracker.draw_call_id_changes == std::vector({std::make_pair<std::size_t, int64_t>(0, -1)}));
}

void assert_float_2d_vectors_are_equal(std::vector<std::vector<float>> actual, std::vector<std::vector<float>> expected) {
  static constexpr float epsilon = 1e-5;
  assert(expected.size() == actual.size());
  for (std::size_t i = 0; i < expected.size(); ++i) {
    assert(expected[i].size() == actual[i].size());
    for (std::size_t j = 0; j < expected[i].size(); ++j)
      assert(std::fabs(expected[i][j] - actual[i][j]) < epsilon);
  }
}
} // namespace jetbrains::renderdoc