#include <cassert>

#include "RenderDocServiceApi.h"
#include "renderdoc_service_test_utils.h"

#include <numeric>

using namespace jetbrains::renderdoc;

void assert_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {

  // disassembly
  {
    const auto debug_session = replay->debug_vertex(lifetime, 784);

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

  // ShaderLab source file
  {
    const auto debug_session = replay->debug_vertex(lifetime, 732);

    const FrameTracker frame_tracker(lifetime, debug_session);

    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_over();
    debug_session->step_over();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();
    debug_session->step_into();

    assert(frame_tracker.frames == std::vector({{model::RdcDebugStack(732, 0, 0, 918, 918, 11, 45)},
                                                {model::RdcDebugStack(732, 2, 0, 226, 226, 8, 41)},
                                                {model::RdcDebugStack(732, 4, 0, 221, 221, 31, 80)},
                                                {model::RdcDebugStack(732, 11, 0, 221, 221, 8, 82)},
                                                {model::RdcDebugStack(732, 18, 0, 226, 226, 1, 43)},
                                                {model::RdcDebugStack(732, 19, 0, 918, 918, 1, 45)},
                                                {model::RdcDebugStack(732, 20, 0, 919, 919, 13, 35)},
                                                {model::RdcDebugStack(732, 21, 0, 920, 920, 1, 10)},
                                                rd::Wrapper<model::RdcDebugStack>(nullptr)}));
  }
}

void assert_try_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  const auto vertex_debug_session = replay->try_debug_vertex(lifetime, breakpoints);

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
         std::vector({{model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},  {model::RdcDebugStack(715, 19, 0, 883, 883, 1, 45)}, {model::RdcDebugStack(715, 20, 0, 884, 884, 13, 28)},
                      {model::RdcDebugStack(715, 21, 0, 884, 884, 13, 34)}, {model::RdcDebugStack(715, 22, 0, 885, 885, 1, 10)}, {model::RdcDebugStack(715, -1, -1, 0, 0, 0, 0)},

                      {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},      {model::RdcDebugStack(732, 0, 0, 918, 918, 11, 45)}, {model::RdcDebugStack(732, 19, 0, 918, 918, 1, 45)},
                      {model::RdcDebugStack(732, 20, 0, 919, 919, 13, 35)}, {model::RdcDebugStack(732, 21, 0, 920, 920, 1, 10)}, {model::RdcDebugStack(732, -1, -1, 0, 0, 0, 0)},

                      {model::RdcDebugStack(749, -1, -1, 0, 0, 0, 0)},

                      {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},      {model::RdcDebugStack(765, 0, 0, 895, 895, 19, 58)}, {model::RdcDebugStack(765, 7, 0, 897, 897, 20, 48)},
                      {model::RdcDebugStack(765, 8, 0, 897, 897, 52, 73)},  {model::RdcDebugStack(765, 9, 0, 897, 897, 20, 73)}, {model::RdcDebugStack(765, 10, 0, 897, 897, 14, 75)},
                      {model::RdcDebugStack(765, 11, 0, 897, 897, 14, 92)}, {model::RdcDebugStack(765, 12, 0, 899, 899, 1, 22)}, {model::RdcDebugStack(765, 13, 0, 901, 901, 11, 45)},
                      {model::RdcDebugStack(765, 33, 0, 901, 901, 1, 45)},  {model::RdcDebugStack(765, 34, 0, 903, 903, 1, 10)}, {model::RdcDebugStack(765, -1, -1, 0, 0, 0, 0)},

                      {model::RdcDebugStack(784, -1, -1, 0, 0, 0, 0)},      {model::RdcDebugStack(784, 0, -1, 15, 15, 0, 0)},    {model::RdcDebugStack(784, 1, -1, 16, 16, 0, 0)},
                      {model::RdcDebugStack(784, 2, -1, 17, 17, 0, 0)},     {model::RdcDebugStack(784, 3, -1, 18, 18, 0, 0)},    rd::Wrapper<model::RdcDebugStack>(nullptr)}));
}

void assert_try_debug_vertex_with_breakpoints(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  const auto vertex_debug_session = replay->try_debug_vertex(lifetime, breakpoints);

  const auto eventId = vertex_debug_session->get_currentStack().get()->get_drawCallId();
  assert(eventId == 715);

  const FrameTracker frame_tracker(lifetime, vertex_debug_session);

  vertex_debug_session->resume();
  vertex_debug_session->add_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/ShaderForSphere.shader"), 22));
  vertex_debug_session->resume();
  vertex_debug_session->resume();
  vertex_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 44));
  vertex_debug_session->remove_source_breakpoint(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Cube Shader.shader"), 62));
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

  assert(frame_tracker.frames == std::vector({{model::RdcDebugStack(715, 0, 0, 883, 883, 11, 45)},
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
}

void assert_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {

  // disassembly
  {
    const auto pixel_debug_session = replay->debug_pixel(lifetime, model::RdcDebugPixelInput(1043, 1133, 664, {}));

    const FrameTracker frame_tracker(lifetime, pixel_debug_session);

    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_into();

    assert(frame_tracker.frames == std::vector({{model::RdcDebugStack(1043, 0, -1, 10, 10, 0, 0)},
                                                {model::RdcDebugStack(1043, 1, -1, 11, 11, 0, 0)},
                                                {model::RdcDebugStack(1043, 2, -1, 12, 12, 0, 0)},
                                                rd::Wrapper<model::RdcDebugStack>(nullptr)}));
  }

  // ShaderLab source file
  {
    const auto pixel_debug_session = replay->debug_pixel(lifetime, model::RdcDebugPixelInput(732, 1133, 664, {}));

    const FrameTracker frame_tracker(lifetime, pixel_debug_session);

    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
    pixel_debug_session->step_into();
    pixel_debug_session->step_over();

    assert(frame_tracker.frames == std::vector({{model::RdcDebugStack(732, 0, 0, 932, 932, 1, 9)},
                                                {model::RdcDebugStack(732, 1, 0, 933, 933, 1, 10)},
                                                {model::RdcDebugStack(732, 2, 0, 934, 934, 28, 40)},
                                                {model::RdcDebugStack(732, 4, 0, 934, 934, 8, 48)},
                                                {model::RdcDebugStack(732, 7, 0, 934, 934, 1, 50)},
                                                rd::Wrapper<model::RdcDebugStack>(nullptr)}));
  }
}

void assert_try_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  const auto pixel_debug_session = replay->try_debug_pixel(lifetime, model::RdcDebugPixelInput(0, 921, 541, breakpoints));

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

    pixel_debug_session->step_into();
    pixel_debug_session->step_over();
    pixel_debug_session->step_over();
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
    rd::Wrapper<model::RdcDebugStack>(nullptr)}));
}

void assert_try_debug_pixel_with_breakpoints(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) {
  breakpoints.emplace_back(model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/ShaderForSphere.shader"), 27));
  const auto pixel_debug_session = replay->try_debug_pixel(lifetime, model::RdcDebugPixelInput(0, 914, 535, breakpoints));

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
        {model::RdcSourceBreakpoint(rd::wrapper::make_wrapper<std::wstring>(L"Assets/Waves.shader"), 58)},
    };

    assert_debug_vertex_step_by_step(lifetime, replay);
    assert_try_debug_vertex_step_by_step(lifetime, replay, breakpoints);
    assert_try_debug_vertex_with_breakpoints(lifetime, replay, breakpoints);

    assert_debug_pixel_step_by_step(lifetime, replay);
    assert_try_debug_pixel_step_by_step(lifetime, replay, breakpoints);
    assert_try_debug_pixel_with_breakpoints(lifetime, replay, breakpoints);
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
