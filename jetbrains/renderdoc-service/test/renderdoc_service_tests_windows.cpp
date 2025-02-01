#include <cassert>

#include "RenderDocServiceApi.h"
#include "renderdoc_service_test_utils.h"

#include <numeric>
#include <set>
#include <utility>

using namespace jetbrains::renderdoc;

void assert_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({0, 0, {}}) , true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({66, 10000, {}}), true);

  // disassembly
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(784, 0, {}));

    const LineTracker line_tracker(lifetime, debug_session);

    std::vector<int> expected_lines(125);

    expected_lines[0] = 15;
    for (uint8_t i = 1; i < 107; ++i) {
      debug_session->step_over();
      expected_lines[i] = i + 15;
    }
    for (uint8_t i = 0 ; i < 17; ++i) {
      debug_session->step_into();
      expected_lines[i + 107] = i + 163;
    }

    debug_session->step_into();
    expected_lines.back() = -1;

    assert(line_tracker.lines == expected_lines);
  }

  // disassembly with another vertex
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(784, 5039, {}));

    const LineTracker line_tracker(lifetime, debug_session);

    std::vector<int> expected_lines(81);

    expected_lines[0] = 15;
    for (uint8_t i = 1; i < 22; ++i) {
      debug_session->step_over();
      expected_lines[i] = i + 15;
    }
    for (uint8_t i = 0 ; i < 58; ++i) {
      debug_session->step_into();
      expected_lines[i + 22] = i + 122;
    }

    debug_session->step_into();
    expected_lines.back() = -1;

    assert(line_tracker.lines == expected_lines);
  }

  // ShaderLab source file
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(732, 30, {}));

    const FrameTracker frame_tracker(lifetime, debug_session);

    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();

    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(732, 0, 0, 918, 918, 11, 45)},
      {model::RdcDebugStack(732, 2, 0, 226, 226, 8, 41)},
      {model::RdcDebugStack(732, 4, 0, 221, 221, 31, 80)},
      {model::RdcDebugStack(732, 11, 0, 221, 221, 8, 82)},
      {model::RdcDebugStack(732, 18, 0, 226, 226, 1, 43)},
      {model::RdcDebugStack(732, 19, 0, 918, 918, 1, 45)},
      {model::RdcDebugStack(732, 20, 0, 919, 919, 13, 35)},
      {model::RdcDebugStack(732, 21, 0, 920, 920, 1, 10)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));
    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 732),
      std::make_pair<std::size_t, int64_t>(8, -1)
    }));
  }
}

void assert_try_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 0, 15000, breakpoints }), false);

  const auto session_lifetime = lifetime.create_nested();
  const auto vertex_debug_session = replay->try_debug_vertex(session_lifetime, model::RdcDebugVertexInput(0, 35, breakpoints));

  const auto eventId = vertex_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, vertex_debug_session);

  {
    // event 715
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);

    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }
  vertex_debug_session->step_into();
  {
    // event 732
    vertex_debug_session->step_into();
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }

  // skip event 749
  vertex_debug_session->step_over();
  vertex_debug_session->step_over();

  {
    // event 765
    vertex_debug_session->step_into();
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);
    vertex_debug_session->step_into();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }
  vertex_debug_session->step_into();
  {
    // event 784
    vertex_debug_session->step_into();
    assert(vertex_debug_session->get_sourceFiles().empty());

    vertex_debug_session->step_into();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->resume();
  }

  assert(frame_tracker.frames ==
         std::vector({
           {model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},
           {model::RdcDebugStack(715, 19, 0, 883, 883, 1, 45)},
           {model::RdcDebugStack(715, 20, 0, 884, 884, 13, 28)},
           {model::RdcDebugStack(715, 21, 0, 884, 884, 13, 34)},
           {model::RdcDebugStack(715, 22, 0, 885, 885, 1, 10)},
           {model::RdcDebugStack(715, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},
           {model::RdcDebugStack(732, 0, 0, 918, 918, 11, 45)},
           {model::RdcDebugStack(732, 19, 0, 918, 918, 1, 45)},
           {model::RdcDebugStack(732, 20, 0, 919, 919, 13, 35)},
           {model::RdcDebugStack(732, 21, 0, 920, 920, 1, 10)},
           {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(749, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},
           {model::RdcDebugStack(765, 0, 0, 895, 895, 19, 58)},
           {model::RdcDebugStack(765, 7, 0, 897, 897, 20, 48)},
           {model::RdcDebugStack(765, 8, 0, 897, 897, 52, 73)},
           {model::RdcDebugStack(765, 9, 0, 897, 897, 20, 73)},
           {model::RdcDebugStack(765, 10, 0, 897, 897, 14, 75)},
           {model::RdcDebugStack(765, 11, 0, 897, 897, 14, 92)},
           {model::RdcDebugStack(765, 12, 0, 899, 899, 1, 22)},
           {model::RdcDebugStack(765, 13, 0, 901, 901, 11, 45)},
           {model::RdcDebugStack(765, 33, 0, 901, 901, 1, 45)},
           {model::RdcDebugStack(765, 34, 0, 903, 903, 1, 10)},
           {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
           {model::RdcDebugStack(784, 0, -1, 15, 15, 0, 0)},
           {model::RdcDebugStack(784, 1, -1, 16, 16, 0, 0)},
           {model::RdcDebugStack(784, 2, -1, 17, 17, 0, 0)},
           {model::RdcDebugStack(784, 3, -1, 18, 18, 0, 0)},
           rd::Wrapper<model::RdcDebugStack>(nullptr)}));
  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 715),
    std::make_pair<std::size_t, int64_t>(6, 732),
    std::make_pair<std::size_t, int64_t>(12, 749),
    std::make_pair<std::size_t, int64_t>(13, 765),
    std::make_pair<std::size_t, int64_t>(25, 784),
    std::make_pair<std::size_t, int64_t>(30, -1)
  }));
}

void assert_try_debug_vertex_step_over(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, uint32_t vert_id, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints, const std::set<uint32_t> &events_should_skip) {
  const auto session_lifetime = lifetime.create_nested();
  const auto vertex_debug_session = replay->try_debug_vertex(session_lifetime, model::RdcDebugVertexInput(0, vert_id, breakpoints));

  const auto eventId = vertex_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, vertex_debug_session);

  {
    // event 715
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);

    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }

  // event 732
  vertex_debug_session->step_into();

  // event 749
  vertex_debug_session->step_over();

  // event 765
  vertex_debug_session->step_over();

  // event 784
  vertex_debug_session->step_over();

  vertex_debug_session->resume();

  assert(frame_tracker.frames ==
         std::vector({{model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},
           {model::RdcDebugStack(715, 19, 0, 883, 883, 1, 45)},
           {model::RdcDebugStack(715, 20, 0, 884, 884, 13, 28)},
           {model::RdcDebugStack(715, 21, 0, 884, 884, 13, 34)},
           {model::RdcDebugStack(715, 22, 0, 885, 885, 1, 10)},
           {model::RdcDebugStack(715, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(749, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
           rd::Wrapper<model::RdcDebugStack>(nullptr)
         }));
  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 715),
    std::make_pair<std::size_t, int64_t>(6, 732),
    std::make_pair<std::size_t, int64_t>(7, 749),
    std::make_pair<std::size_t, int64_t>(8, 765),
    std::make_pair<std::size_t, int64_t>(9, 784),
    std::make_pair<std::size_t, int64_t>(10, -1)
  }));

  for (std::size_t i = 0; i + 1 < frame_tracker.draw_call_sessions.size(); ++i) {
    if (frame_tracker.draw_call_sessions[i])
      continue;

    assert(events_should_skip.find(frame_tracker.draw_call_id_changes[i].second) != events_should_skip.end());
  }
}

void assert_try_debug_uncommon_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  const auto session_lifetime = lifetime.create_nested();
  const auto vertex_debug_session = replay->try_debug_vertex(session_lifetime, model::RdcDebugVertexInput(0, 100, breakpoints));

  const auto eventId = vertex_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, vertex_debug_session);

  {
    // event 715
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);

    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }
  vertex_debug_session->step_into();

  // event 732, no vertex 100
  vertex_debug_session->step_into();

  // event 749, no vertex 100
  vertex_debug_session->step_into();

  {
    // event 765
    vertex_debug_session->step_into();
    const auto name = vertex_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);
    vertex_debug_session->step_into();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
  }
  vertex_debug_session->step_into();
  {
    // event 784
    vertex_debug_session->step_into();
    assert(vertex_debug_session->get_sourceFiles().empty());

    vertex_debug_session->step_into();
    vertex_debug_session->step_over();
    vertex_debug_session->step_over();
    vertex_debug_session->resume();
  }

  assert(frame_tracker.frames ==
         std::vector({
           {model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},
           {model::RdcDebugStack(715, 19, 0, 883, 883, 1, 45)},
           {model::RdcDebugStack(715, 20, 0, 884, 884, 13, 28)},
           {model::RdcDebugStack(715, 21, 0, 884, 884, 13, 34)},
           {model::RdcDebugStack(715, 22, 0, 885, 885, 1, 10)},
           {model::RdcDebugStack(715, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(749, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},
           {model::RdcDebugStack(765, 0, 0, 895, 895, 19, 58)},
           {model::RdcDebugStack(765, 7, 0, 897, 897, 20, 48)},
           {model::RdcDebugStack(765, 8, 0, 897, 897, 52, 73)},
           {model::RdcDebugStack(765, 9, 0, 897, 897, 20, 73)},
           {model::RdcDebugStack(765, 10, 0, 897, 897, 14, 75)},
           {model::RdcDebugStack(765, 11, 0, 897, 897, 14, 92)},
           {model::RdcDebugStack(765, 12, 0, 899, 899, 1, 22)},
           {model::RdcDebugStack(765, 13, 0, 901, 901, 11, 45)},
           {model::RdcDebugStack(765, 33, 0, 901, 901, 1, 45)},
           {model::RdcDebugStack(765, 34, 0, 903, 903, 1, 10)},
           {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

           {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
           {model::RdcDebugStack(784, 0, -1, 15, 15, 0, 0)},
           {model::RdcDebugStack(784, 1, -1, 16, 16, 0, 0)},
           {model::RdcDebugStack(784, 2, -1, 17, 17, 0, 0)},
           {model::RdcDebugStack(784, 3, -1, 18, 18, 0, 0)},
           rd::Wrapper<model::RdcDebugStack>(nullptr)}));

  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 715),
    std::make_pair<std::size_t, int64_t>(6, 732),
    std::make_pair<std::size_t, int64_t>(7, 749),
    std::make_pair<std::size_t, int64_t>(8, 765),
    std::make_pair<std::size_t, int64_t>(20, 784),
    std::make_pair<std::size_t, int64_t>(25, -1)
  }));

  for (std::size_t i = 0; i + 1 < frame_tracker.draw_call_sessions.size(); ++i) {
    if (frame_tracker.draw_call_sessions[i])
      continue;

    assert(frame_tracker.draw_call_id_changes[i].second == 732 || frame_tracker.draw_call_id_changes[i].second == 749);
  }
}

void assert_try_debug_vertex_with_breakpoints(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  const auto session_lifetime = lifetime.create_nested();
  const auto vertex_debug_session = replay->try_debug_vertex(session_lifetime, model::RdcDebugVertexInput(0, 17, breakpoints));

  const auto eventId = vertex_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, vertex_debug_session);

  vertex_debug_session->resume();
  vertex_debug_session->add_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/ShaderForSphere.shader"), 22));
  vertex_debug_session->resume();
  vertex_debug_session->resume();
  vertex_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 44));
  vertex_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 62));
  vertex_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Waves.shader"), 47));
  vertex_debug_session->add_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Waves.shader"), 53));
  vertex_debug_session->resume();
  vertex_debug_session->resume();
  vertex_debug_session->step_over();
  vertex_debug_session->step_over();
  vertex_debug_session->step_into();
  vertex_debug_session->add_breakpoint(-1, 22);
  vertex_debug_session->add_breakpoint(-1, 167);
  vertex_debug_session->add_breakpoint(-1, 125);
  vertex_debug_session->remove_breakpoint(-1, 22);
  vertex_debug_session->resume();
  vertex_debug_session->resume();

  assert(frame_tracker.frames == std::vector({
    {model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},
    {model::RdcDebugStack(715, 19, 0, 883, 883, 1, 45)},
    {model::RdcDebugStack(715, 22, 0, 885, 885, 1, 10)},

    {model::RdcDebugStack(732, 20, 0, 919, 919, 13, 35)},

    {model::RdcDebugStack(749, 24, 0, 904, 904, 8, 12)},

    {model::RdcDebugStack(765, 34, 0, 903, 903, 1, 10)},
    {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
    {model::RdcDebugStack(784, 0, -1, 15, 15, 0, 0)},
    {model::RdcDebugStack(784, 111, -1, 167, 167, 0, 0)},
    rd::Wrapper<model::RdcDebugStack>(nullptr)}));

  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 715),
    std::make_pair<std::size_t, int64_t>(3, 732),
    std::make_pair<std::size_t, int64_t>(4, 749),
    std::make_pair<std::size_t, int64_t>(5, 765),
    std::make_pair<std::size_t, int64_t>(7, 784),
    std::make_pair<std::size_t, int64_t>(10, -1)
  }));

  for (std::size_t i = 0; i + 1 < frame_tracker.draw_call_sessions.size(); ++i) {
    assert(frame_tracker.draw_call_sessions[i]);
  }
}

void assert_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 0, 0, 0, {} }), true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 739, 826, 914, {} }), true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 749, 826, 914, {} }), true);

  // disassembly
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto pixel_debug_session = replay->debug_pixel(session_lifetime, model::RdcDebugPixelInput(1043, 1133, 664, {}));

    const FrameTracker frame_tracker(lifetime, pixel_debug_session);

    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_into();

    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(1043, 0, -1, 10, 10, 0, 0)},
      {model::RdcDebugStack(1043, 1, -1, 11, 11, 0, 0)},
      {model::RdcDebugStack(1043, 2, -1, 12, 12, 0, 0)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));

    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 1043),
      std::make_pair<std::size_t, int64_t>(3, -1)
    }));
  }

  // ShaderLab source file
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto pixel_debug_session = replay->debug_pixel(session_lifetime, model::RdcDebugPixelInput(732, 1133, 664, {}));

    const FrameTracker frame_tracker(lifetime, pixel_debug_session);

    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_into();
    pixel_debug_session->step_over();

    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(732, 0, 0, 932, 932, 1, 9)},
      {model::RdcDebugStack(732, 1, 0, 933, 933, 1, 10)},
      {model::RdcDebugStack(732, 2, 0, 934, 934, 28, 40)},
      {model::RdcDebugStack(732, 4, 0, 934, 934, 8, 48)},
      {model::RdcDebugStack(732, 7, 0, 934, 934, 1, 50)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));

    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 732),
      std::make_pair<std::size_t, int64_t>(5, -1)
    }));
  }
}

void assert_try_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 0, 0, 0, breakpoints }), false);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 0, 826, 914, breakpoints }), false);

  const auto session_lifetime = lifetime.create_nested();
  const auto pixel_debug_session = replay->try_debug_pixel(session_lifetime, model::RdcDebugPixelInput(0, 914, 534, breakpoints));

  const auto eventId = pixel_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 749);

  const FrameTracker frame_tracker(lifetime, pixel_debug_session);

  {
    // event 749
    const auto name = pixel_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);

    pixel_debug_session->step_into();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
  }
  pixel_debug_session->step_over();
  {
    // event 765
    pixel_debug_session->step_into();
    const auto name = pixel_debug_session->get_sourceFiles().at(0)->get_name();
    assert(name.find(L"unnamed_shader") != std::wstring::npos);
    pixel_debug_session->step_over();
  }
  pixel_debug_session->step_over();

  {
    // event 784
    pixel_debug_session->step_into();
    assert(pixel_debug_session->get_sourceFiles().empty());

    for (uint8_t i = 0; i < 17; ++i)
      pixel_debug_session->step_over();
  }
  pixel_debug_session->step_into();
  {
    // event 811
    pixel_debug_session->step_into();
    assert(pixel_debug_session->get_sourceFiles().empty());

    for (uint8_t i = 0; i < 18; ++i)
      pixel_debug_session->step_over();
  }
  {
    // event 811 and 824 should be skipped, no (914, 534) pixel in the event
    pixel_debug_session->step_into();
    pixel_debug_session->step_into();
    pixel_debug_session->resume();
  }

  assert(frame_tracker.frames == std::vector({
    {model::RdcDebugStack(749, 1, 0, 944, 944, 8, 23)},
    {model::RdcDebugStack(749, 4, 0, 900, 900, 8, 12)},
    {model::RdcDebugStack(749, 6, 0, 944, 944, 27, 50)},
    {model::RdcDebugStack(749, 7, 0, 944, 944, 8, 50)},
    {model::RdcDebugStack(749, 8, 0, 944, 944, 1, 52)},
    {model::RdcDebugStack(749, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},
    {model::RdcDebugStack(765, 0, 0, 908, 908, 1, 15)},
    {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
    {model::RdcDebugStack(784, 0, -1, 12, 12, 0, 0)},
    {model::RdcDebugStack(784, 1, -1, 13, 13, 0, 0)},
    {model::RdcDebugStack(784, 2, -1, 14, 14, 0, 0)},
    {model::RdcDebugStack(784, 3, -1, 15, 15, 0, 0)},
    {model::RdcDebugStack(784, 4, -1, 16, 16, 0, 0)},
    {model::RdcDebugStack(784, 5, -1, 17, 17, 0, 0)},
    {model::RdcDebugStack(784, 6, -1, 18, 18, 0, 0)},
    {model::RdcDebugStack(784, 7, -1, 19, 19, 0, 0)},
    {model::RdcDebugStack(784, 8, -1, 20, 20, 0, 0)},
    {model::RdcDebugStack(784, 9, -1, 21, 21, 0, 0)},
    {model::RdcDebugStack(784, 10, -1, 22, 22, 0, 0)},
    {model::RdcDebugStack(784, 11, -1, 23, 23, 0, 0)},
    {model::RdcDebugStack(784, 12, -1, 24, 24, 0, 0)},
    {model::RdcDebugStack(784, 13, -1, 25, 25, 0, 0)},
    {model::RdcDebugStack(784, 14, -1, 26, 26, 0, 0)},
    {model::RdcDebugStack(784, 15, -1, 27, 27, 0, 0)},
    {model::RdcDebugStack(784, 16, -1, 28, 28, 0, 0)},
    {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(811, -1, -1, 0, 0, 0, 0)},
    {model::RdcDebugStack(811, 0, -1, 10, 10, 0, 0)},
    {model::RdcDebugStack(811, 1, -1, 11, 11, 0, 0)},
    {model::RdcDebugStack(811, 2, -1, 12, 12, 0, 0)},
    {model::RdcDebugStack(811, 3, -1, 13, 13, 0, 0)},
    {model::RdcDebugStack(811, 4, -1, 14, 14, 0, 0)},
    {model::RdcDebugStack(811, 5, -1, 15, 15, 0, 0)},
    {model::RdcDebugStack(811, 6, -1, 16, 16, 0, 0)},
    {model::RdcDebugStack(811, 7, -1, 17, 17, 0, 0)},
    {model::RdcDebugStack(811, 8, -1, 18, 18, 0, 0)},
    {model::RdcDebugStack(811, 9, -1, 19, 19, 0, 0)},
    {model::RdcDebugStack(811, 10, -1, 20, 20, 0, 0)},
    {model::RdcDebugStack(811, 11, -1, 21, 21, 0, 0)},
    {model::RdcDebugStack(811, 12, -1, 22, 22, 0, 0)},
    {model::RdcDebugStack(811, 13, -1, 23, 23, 0, 0)},
    {model::RdcDebugStack(811, 14, -1, 24, 24, 0, 0)},
    {model::RdcDebugStack(811, 15, -1, 25, 25, 0, 0)},
    {model::RdcDebugStack(811, 16, -1, 26, 26, 0, 0)},
    {model::RdcDebugStack(811, 17, -1, 27, 27, 0, 0)},
    {model::RdcDebugStack(811, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(824, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(837, -1, -1, 0, 0, 0, 0)},

    rd::Wrapper<model::RdcDebugStack>(nullptr)}));

  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 749),
    std::make_pair<std::size_t, int64_t>(6, 765),
    std::make_pair<std::size_t, int64_t>(9, 784),
    std::make_pair<std::size_t, int64_t>(28, 811),
    std::make_pair<std::size_t, int64_t>(48, 824),
    std::make_pair<std::size_t, int64_t>(49, 837),
    std::make_pair<std::size_t, int64_t>(50, -1)
  }));

  for (std::size_t i = 0; i + 1 < frame_tracker.draw_call_sessions.size(); ++i) {
    if (frame_tracker.draw_call_sessions[i])
      continue;

    assert(frame_tracker.draw_call_id_changes[i].second == 824 || frame_tracker.draw_call_id_changes[i].second == 837);
  }
}

void assert_try_debug_pixel_with_breakpoints(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  breakpoints.emplace_back(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/ShaderForSphere.shader"), 27));
  const auto session_lifetime = lifetime.create_nested();
  const auto pixel_debug_session = replay->try_debug_pixel(session_lifetime, model::RdcDebugPixelInput(0, 914, 535, breakpoints));

  const auto eventId = pixel_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, pixel_debug_session);

  pixel_debug_session->resume();
  pixel_debug_session->add_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 69));
  pixel_debug_session->resume();
  pixel_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 72));
  pixel_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/mult.hlsl"), 3));
  pixel_debug_session->resume();
  pixel_debug_session->step_over();
  pixel_debug_session->step_over();
  pixel_debug_session->step_into();
  pixel_debug_session->add_breakpoint(-1, 15);
  pixel_debug_session->add_breakpoint(-1, 28);
  pixel_debug_session->remove_breakpoint(-1, 15);
  pixel_debug_session->resume();
  pixel_debug_session->resume();

  assert(frame_tracker.frames == std::vector({
    {model::RdcDebugStack(715, 0, 0, 890, 890, 8, 30)},
    {model::RdcDebugStack(715, 1, 0, 890, 890, 1, 32)},

    {model::RdcDebugStack(749, 0, 0, 941, 943, 13, 1)},

    {model::RdcDebugStack(765, 0, 0, 908, 908, 1, 15)},
    {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

    {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},
    {model::RdcDebugStack(784, 0, -1, 12, 12, 0, 0)},
    {model::RdcDebugStack(784, 16, -1, 28, 28, 0, 0)},
    rd::Wrapper<model::RdcDebugStack>(nullptr)}));

  assert(frame_tracker.draw_call_id_changes == std::vector({
    std::make_pair<std::size_t, int64_t>(0, 715),
    std::make_pair<std::size_t, int64_t>(2, 749),
    std::make_pair<std::size_t, int64_t>(3, 765),
    std::make_pair<std::size_t, int64_t>(5, 784),
    std::make_pair<std::size_t, int64_t>(8, -1)
  }));

  for (std::size_t i = 0; i + 1 < frame_tracker.draw_call_sessions.size(); ++i) {
    assert(frame_tracker.draw_call_sessions[i]);
  }
}

void assert_vertices_table(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 787);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 97);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 697);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 112);
    assert(vertices != nullptr);
    assert(vertices->get_input_indices() == std::vector<uint32_t>({ 22561, 22563, 22562, 22562, 22563, 22564 }));
    assert(vertices->get_input_columns() == std::vector({rd::Wrapper<std::wstring>(L"POSITION"), rd::Wrapper<std::wstring>(L"COLOR"), rd::Wrapper<std::wstring>(L"TEXCOORD0")}));
    assert(vertices->get_output_columns() == std::vector({rd::Wrapper<std::wstring>(L"SV_POSITION"), rd::Wrapper<std::wstring>(L"COLOR"), rd::Wrapper<std::wstring>(L"TEXCOORD0"), rd::Wrapper<std::wstring>(L"TEXCOORD1")}));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[0],  std::vector<std::vector<float>>({ { -0.000671386719, 761.333008, 0.00 }, { 0.0980392172, 0.0980392172, 0.0980392172, 1.00 }, { 0.00, 0.00 } }));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[5], std::vector<std::vector<float>>({ { 1356.66626, 0.00, 0.00 }, { 0.0980392172, 0.0980392172, 0.0980392172, 1.00 }, { 1.00, 1.00 } }));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[2], std::vector<std::vector<float>>({ { 0.99999988, -1.00, 0.990099012, 1.00 }, { 0.0980392172, 0.0980392172, 0.0980392172, 1.00 }, { 1.00, 0.00 }, { 0.93749994, 0.937500059 } }));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[3], std::vector<std::vector<float>>({ { 0.99999988, -1.00, 0.990099012, 1.00 }, { 0.0980392172, 0.0980392172, 0.0980392172, 1.00 }, { 1.00, 0.00 }, { 0.93749994, 0.937500059 } }));
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 677);
    assert(vertices != nullptr);
    assert(vertices->get_input_indices() == std::vector<uint32_t>({0, 1, 2}));
    assert(vertices->get_output_indices() == std::vector<uint32_t>({0, 1, 2}));
    assert(vertices->get_input_columns().empty());
    assert(vertices->get_output_columns() == std::vector({rd::Wrapper<std::wstring>(L"SV_POSITION"), rd::Wrapper<std::wstring>(L"TEXCOORD")}));
    assert(vertices->get_inputs() == std::vector<std::vector<std::vector<float>>>({{}, {}, {}}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[0], std::vector<std::vector<float>>({{ -1.0, -1.0, 1.0, 1.0 }, { 0.0, 1.0 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[1], std::vector<std::vector<float>>({{ 3.0, -1.0, 1.0, 1.0 }, { 2.0, 1.0 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[2], std::vector<std::vector<float>>({{ -1.0, 3.0, 1.0, 1.0 }, { 0.0, -1.0 }}));
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 715);
    assert(vertices != nullptr);
    assert(vertices->get_input_indices().size() == 2304);
    assert(vertices->get_output_indices().size() == 2304);
    assert(vertices->get_input_indices() == vertices->get_output_indices());
    assert(vertices->get_input_indices()[0] == 177 && vertices->get_input_indices()[1] == 386 && vertices->get_input_indices()[1442] == 298 && vertices->get_input_indices()[2303] == 510);
    assert(vertices->get_input_columns() == std::vector({rd::Wrapper<std::wstring>(L"POSITION"), rd::Wrapper<std::wstring>(L"NORMAL"), rd::Wrapper<std::wstring>(L"TEXCOORD0")}));
    assert(vertices->get_output_columns() == std::vector({rd::Wrapper<std::wstring>(L"SV_POSITION"), rd::Wrapper<std::wstring>(L"COLOR")}));
    assert(vertices->get_inputs().size() == 2304);
    assert(vertices->get_outputs().size() == 2304);
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[0], std::vector<std::vector<float>>({{ 0.19199951, -0.450263649, -0.0945074409 }, { 0.382818788, -0.904339671, -0.188731149 }, { 0.425961286, 0.175267562 }}));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[2227], std::vector<std::vector<float>>({{ 0.0905187949, 0.275551766, -0.406967759 }, { 0.180491149, 0.550244927, -0.815262854 }, { 0.283699751, 0.675781726 }}));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[2303], std::vector<std::vector<float>>({{ -0.288835436, 0.287900388, -0.288835436 }, { -0.577053428, 0.577943504, -0.577053428 }, { 0.123985529, 0.683609068 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[0], std::vector<std::vector<float>>({{ -0.892122149, 0.0177013576, 0.0561662987, 5.5933671 }, { 0.691409409, 0.0478301644, 0.405634433 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[227], std::vector<std::vector<float>>({{ -0.817703127, -0.542326808, 0.0561641343, 6.02090645 }, { 0.211324871, 0.211324871, 0.211324841 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[524], std::vector<std::vector<float>>({{ -0.670523405, -1.33369005, 0.0561684333, 5.1719327 }, { 0.683429419, 0.927442729, 0.683429419 }}));
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 765);
    assert(vertices != nullptr);
    {
      const auto &vertices_grouped = replay->get_vertices_inoutputs(lifetime, 739);
      assert(vertices == vertices_grouped);
    }
    assert(vertices->get_input_indices().size() == 600);
    assert(vertices->get_output_indices().size() == 600);
    assert(vertices->get_input_indices() == vertices->get_output_indices());
    assert(vertices->get_input_indices()[0] == 9 && vertices->get_input_indices()[1] == 21 && vertices->get_input_indices()[526] == 59 && vertices->get_input_indices()[530] == 60);
    assert(vertices->get_input_columns() == std::vector({rd::Wrapper<std::wstring>(L"POSITION"), rd::Wrapper<std::wstring>(L"NORMAL")}));
    assert(vertices->get_output_columns() == std::vector({rd::Wrapper<std::wstring>(L"SV_POSITION")}));
    assert(vertices->get_inputs().size() == 600);
    assert(vertices->get_outputs().size() == 600);
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[0], std::vector<std::vector<float>>({{ -4.00000048, -1.11022302e-16, 5 }, { 0, 1, 0 }}));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[123], std::vector<std::vector<float>>({{ 0, -6.66133841e-17, 3 }, { 0, 1, 0 }}));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[599], std::vector<std::vector<float>>({{ 0.99999994, -6.66133841e-17, 3 }, { 0, 1, 0 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[0], std::vector<std::vector<float>>({{ 5.94775963, 0.726316333, 0.0561287366, 13.0075474 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[123], std::vector<std::vector<float>>({{ 2.5044744, 3.02407265, 0.0561392419, 10.9337492 }}));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[599], std::vector<std::vector<float>>({{ 2.034446, 2.76309252, 0.0561441183, 9.97161197 }}));
  }
}

void assert_texture_outputs(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  const auto &outputs_root = replay->get_textureRGBBuffer(lifetime, -1);

  {
    const auto &outputs_last = replay->get_textureRGBBuffer(lifetime, 1671);
    assert(outputs_root == outputs_last);
    assert(outputs_root->get_colorOutputs().size() == 1);
    assert(!outputs_root->get_depthOutput());
    assert(outputs_root->get_colorOutputs()[0]->get_width() == 2035);
    assert(outputs_root->get_colorOutputs()[0]->get_height() == 1142);
  }
  {
    const auto &outputs_grouped = replay->get_textureRGBBuffer(lifetime, 97);
    const auto &outputs_leaf = replay->get_textureRGBBuffer(lifetime, 1638);
    assert(outputs_grouped == outputs_leaf);
    assert(outputs_grouped->get_colorOutputs().size() == 1);
    assert(outputs_grouped->get_depthOutput());
    assert(outputs_root->get_colorOutputs()[0]->get_width() == 2035);
    assert(outputs_root->get_colorOutputs()[0]->get_height() == 1142);
  }
  {
    const auto &outputs = replay->get_textureRGBBuffer(lifetime, 0);
    assert(outputs->get_colorOutputs().size() == 1);
    assert(outputs->get_depthOutput());
    assert(outputs->get_colorOutputs()[0]->get_width() == 2035);
    assert(outputs->get_colorOutputs()[0]->get_height() == 1142);
  }
  {
    const auto &outputs_grouped = replay->get_textureRGBBuffer(lifetime, 543);
    assert(outputs_grouped->get_colorOutputs().size() == 1);
    assert(outputs_grouped->get_depthOutput());
    assert(outputs_grouped->get_colorOutputs()[0]->get_width() == 2032);
    assert(outputs_grouped->get_colorOutputs()[0]->get_height() == 1070);
  }
  {
    const auto &outputs_grouped = replay->get_textureRGBBuffer(lifetime, 696);
    {
      const auto &outputs_grouped_1 = replay->get_textureRGBBuffer(lifetime, 739);
      assert(outputs_grouped == outputs_grouped_1);
      assert(outputs_grouped->get_colorOutputs().size() == 1);
      assert(outputs_grouped->get_depthOutput());
      assert(outputs_grouped->get_colorOutputs()[0]->get_width() == 2032);
      assert(outputs_grouped->get_colorOutputs()[0]->get_height() == 1070);
    }
    {
      const auto &outputs_leaf = replay->get_textureRGBBuffer(lifetime, 715);
      assert(outputs_leaf->get_colorOutputs().size() == 1);
      assert(outputs_leaf->get_depthOutput());
      assert(outputs_leaf->get_colorOutputs()[0]->get_width() == 2032);
      assert(outputs_leaf->get_colorOutputs()[0]->get_height() == 1070);
    }
    {
      const auto &outputs_grouped_1 = replay->get_textureRGBBuffer(lifetime, 697);
      assert(outputs_grouped_1 != outputs_grouped);

      assert(outputs_grouped_1->get_colorOutputs().size() == 1);
      assert(outputs_grouped_1->get_depthOutput());
      assert(outputs_grouped_1->get_colorOutputs()[0]->get_width() == 2032);
      assert(outputs_grouped_1->get_colorOutputs()[0]->get_height() == 1070);

      const auto &outputs_end_event = replay->get_textureRGBBuffer(lifetime, 738);
      assert(outputs_grouped_1 == outputs_end_event);
    }
  }
  {
    const auto &outputs_leaf = replay->get_textureRGBBuffer(lifetime, 1244);
    assert(outputs_leaf->get_colorOutputs().size() == 1);
    assert(outputs_leaf->get_depthOutput());
    assert(outputs_leaf->get_colorOutputs()[0]->get_width() == 2032);
    assert(outputs_leaf->get_colorOutputs()[0]->get_height() == 1070);
  }
}

int main() {
  const rd::LifetimeDefinition test_lifetime_def;
  const auto lifetime = test_lifetime_def.lifetime;
  RenderDocService service;
  try {
    const auto file = service.open_capture_file(L"samples/windows/test.rdc");
    assert(file->get_driverName() == L"D3D11");

    const auto replay = file->open_capture();
    assert(replay->get_api() == model::RdcGraphicsApi::D3D11);

    assert(std::size(replay->get_rootActions()) == 13);

    std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> breakpoints = {
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 44)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 62)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 72)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/NewShader.shader"), 44)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/NewShader.shader"), 59)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/ShaderForSphere.shader"), 20)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/mult.hlsl"), 3)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/mult.hlsl"), 7)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Waves.shader"), 47)},
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Waves.shader"), 58)},
    };

    assert_debug_vertex_step_by_step(lifetime, replay);
    assert_try_debug_vertex_step_over(lifetime, replay, 35, breakpoints, {});
    assert_try_debug_vertex_step_over(lifetime, replay, 100, breakpoints, {732, 749});
    assert_try_debug_vertex_step_by_step(lifetime, replay, breakpoints);
    assert_try_debug_uncommon_vertex_step_by_step(lifetime, replay, breakpoints);
    assert_try_debug_vertex_with_breakpoints(lifetime, replay, breakpoints);

    assert_debug_pixel_step_by_step(lifetime, replay);
    assert_try_debug_pixel_step_by_step(lifetime, replay, breakpoints);
    assert_try_debug_pixel_with_breakpoints(lifetime, replay, breakpoints);

    assert_vertices_table(lifetime, replay);
    assert_texture_outputs(lifetime, replay);
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
