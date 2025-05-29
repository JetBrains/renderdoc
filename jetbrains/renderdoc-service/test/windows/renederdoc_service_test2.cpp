#include "renderdoc_windows_tests.h"
#include "RenderDocServiceApi.h"
#include "../renderdoc_service_test_utils.h"

namespace {
using namespace jetbrains::renderdoc;

void assert_actions_collection(const rd::Wrapper<RenderDocReplay> &replay) {
  assert(std::size(replay->get_rootActions()) == 9);

  const auto file_usages = get_file_usages_in_events(replay);

  assert(file_usages.size() == 1);

  const auto &entrypoints = std::set<std::wstring>({L"Assets/Shaders/050-059/050_Glass.shader"});
  const auto &includes = std::set<std::wstring>({
    L"C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/HLSLSupport.cginc",
    L"C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderVariables.cginc",
    L"C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityShaderUtilities.cginc",
    L"C:/Program Files/Unity/Hub/Editor/2022.3.45f1/Editor/Data/CGIncludes/UnityCG.cginc"
    });

  assert_source_file_usages(file_usages.at(1231), entrypoints, includes);
}

void assert_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({0, 0, {}}), true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({1231, 36, {}}) , true);

  // disassembly
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(1124, 2, {}));

    const LineTracker line_tracker(session_lifetime, debug_session);

    std::vector<int> expected_lines(44);

    expected_lines[0] = 22;
    for (uint8_t i = 1; i < 44; ++i) {
      if (i % 3 == 0)
        debug_session->step_over();
      else if (i % 3 == 1)
        debug_session->step_into();
      else
        debug_session->step_out();
      expected_lines[i] = i + 22;
    }

    expected_lines.back() = -1;

    assert(line_tracker.lines == expected_lines);
  }

  // ShaderLab source file
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(1231, 35, {}));

    const FrameTracker frame_tracker(session_lifetime, debug_session);

    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();

    assert(frame_tracker.frames == std::vector({
          {model::RdcDebugStack(1231, 0, 0, 905, 905, 14, 48)},
          {model::RdcDebugStack(1231, 2, 0, 203, 203, 8, 41)},
          {model::RdcDebugStack(1231, 18, 0, 203, 203, 1, 43)},
          {model::RdcDebugStack(1231, 19, 0, 905, 905, 1, 48)},
          {model::RdcDebugStack(1231, 20, 0, 907, 907, 7, 19)},
          {model::RdcDebugStack(1231, 21, 0, 909, 909, 47, 68)},
          {model::RdcDebugStack(1231, 22, 0, 909, 909, 21, 87)},
          {model::RdcDebugStack(1231, 24, 0, 909, 909, 19, 95)},
          {model::RdcDebugStack(1231, 25, 0, 910, 910, 1, 33)},
          {model::RdcDebugStack(1231, 26, 0, 911, 911, 12, 41)},
          {model::RdcDebugStack(1231, 27, 0, 911, 911, 12, 60)},
          {model::RdcDebugStack(1231, 28, 0, 912, 912, 16, 45)},
          {model::RdcDebugStack(1231, 29, 0, 912, 912, 16, 64)},
          {model::RdcDebugStack(1231, 30, 0, 913, 913, 1, 10)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));
    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 1231),
      std::make_pair<std::size_t, int64_t>(14, -1)
    }));
  }
}

void assert_debug_vertex_step_out(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  // ShaderLab source file
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(1231, 30, {}));

    const FrameTracker frame_tracker(session_lifetime, debug_session);

    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_out();
    debug_session->step_out();
    debug_session->step_over();
    debug_session->step_out();

    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(1231, 0, 0, 905, 905, 14, 48)},
      {model::RdcDebugStack(1231, 2, 0, 203, 203, 8, 41)},
      {model::RdcDebugStack(1231, 4, 0, 198, 198, 31, 80)},
      {model::RdcDebugStack(1231, 18, 0, 203, 203, 1, 43)},
      {model::RdcDebugStack(1231, 19, 0, 905, 905, 1, 48)},
      {model::RdcDebugStack(1231, 20, 0, 907, 907, 7, 19)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));
    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 1231),
      std::make_pair<std::size_t, int64_t>(6, -1)
    }));
  }

  // ShaderLab source file, another scenario
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto debug_session = replay->debug_vertex(session_lifetime, model::RdcDebugVertexInput(1231, 30, {}));

    const FrameTracker frame_tracker(session_lifetime, debug_session);

    debug_session->step_into();
    debug_session->step_out();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_out();
    assert(frame_tracker.frames.size() == 7);
    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(1231, 0, 0, 905, 905, 14, 48)},
      {model::RdcDebugStack(1231, 2, 0, 203, 203, 8, 41)},
      {model::RdcDebugStack(1231, 19, 0, 905, 905, 1, 48)},
      {model::RdcDebugStack(1231, 20, 0, 907, 907, 7, 19)},
      {model::RdcDebugStack(1231, 21, 0, 909, 909, 47, 68)},
      {model::RdcDebugStack(1231, 22, 0, 909, 909, 21, 87)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));
    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 1231),
      std::make_pair<std::size_t, int64_t>(6, -1)
    }));
  }
}

void assert_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  // instantly finishing sessions
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 0, 0, 0, {} }), true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput({ 1159, 297, 336, {} }), true);

  // ShaderLab source file
  {
    const auto session_lifetime = lifetime.create_nested();
    const auto pixel_debug_session = replay->debug_pixel(session_lifetime, model::RdcDebugPixelInput(1231, 392, 28, {}));

    const FrameTracker frame_tracker(lifetime, pixel_debug_session);

    pixel_debug_session->step_into();
    pixel_debug_session->step_into();
    pixel_debug_session->step_into();
    pixel_debug_session->step_into();
    pixel_debug_session->step_over();
    pixel_debug_session->step_out();
    pixel_debug_session->step_out();
    pixel_debug_session->step_over();
    pixel_debug_session->step_out();


    assert(frame_tracker.frames == std::vector({
      {model::RdcDebugStack(1231, 0, 0, 918, 918, 14, 61)},
      {model::RdcDebugStack(1231, 1, 0, 918, 918, 29, 59)},
      {model::RdcDebugStack(1231, 2, 0, 918, 918, 14, 61)},
      {model::RdcDebugStack(1231, 3, 0, 706, 706, 8, 45)},
      {model::RdcDebugStack(1231, 5, 0, 696, 696, 1, 36)},
      {model::RdcDebugStack(1231, 6, 0, 699, 699, 15, 35)},
      {model::RdcDebugStack(1231, 12, 0, 706, 706, 1, 47)},
      {model::RdcDebugStack(1231, 13, 0, 918, 918, 7, 66)},
      {model::RdcDebugStack(1231, 14, 0, 919, 919, 17, 27)},
      rd::Wrapper<model::RdcDebugStack>(nullptr)}));

    assert(frame_tracker.draw_call_id_changes == std::vector({
      std::make_pair<std::size_t, int64_t>(0, 1231),
      std::make_pair<std::size_t, int64_t>(9, -1)
    }));
  }
}
}

void Test2::test(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocFile> &file, const rd::Wrapper<RenderDocReplay> &replay) {
  // The following draw calls use user's shader files:
  // 1130, 1148, 1163, 1199 - Assets/PostProcessing/Resources/Shaders/AmbientOcclusion.cginc

  assert(file->get_driverName() == L"D3D11");

  assert(replay->get_api() == model::RdcGraphicsApi::D3D11);

  assert_actions_collection(replay);
  assert_debug_vertex_step_by_step(lifetime, replay);
  assert_debug_vertex_step_out(lifetime, replay);
  assert_debug_pixel_step_by_step(lifetime, replay);
}
