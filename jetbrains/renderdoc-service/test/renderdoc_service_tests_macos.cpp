#include <cassert>

#include "RenderDocServiceApi.h"
#include "renderdoc_service_test_utils.h"

using namespace jetbrains::renderdoc;

void assert_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, uint32_t eventId) {
  const auto debug_session = replay->debug_vertex(lifetime, eventId);
  const auto name = debug_session->get_sourceFiles()[0].get()->get_name();
  assert(name.find(L"/triangle.vert") != std::wstring::npos);

  const LineTracker line_tracker(lifetime, debug_session);

  debug_session->step_into();
  debug_session->step_over();
  debug_session->step_into();
  debug_session->step_into();
  debug_session->step_into();

  assert(line_tracker.lines == std::vector({32, 33, 34, 27, 22, -1}));
}

void assert_debug_vertex_with_breakpoints(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, uint32_t eventId) {
  const auto debug_session = replay->debug_vertex(lifetime, eventId);
  const LineTracker line_tracker(lifetime, debug_session);

  debug_session->add_breakpoint(0, 34);
  debug_session->add_breakpoint(0, 22);
  debug_session->add_breakpoint(0, 27);
  debug_session->resume();
  debug_session->resume();
  debug_session->remove_breakpoint(0, 27);
  debug_session->resume();
  debug_session->resume();
  debug_session->resume();

  assert(line_tracker.lines == std::vector({32, 27, 22, 34, 22, -1}));
}

int main() {
  const rd::LifetimeDefinition test_lifetime_def;
  const auto lifetime = test_lifetime_def.lifetime;
  RenderDocService service;
  try {
    const auto file = service.open_capture_file(L"samples/macos/test.rdc");
    assert(file->get_driverName() == L"Vulkan");

    const auto replay = file->open_capture();
    assert(replay->get_api() == model::RdcGraphicsApi::Vulkan);

    assert(std::size(replay->get_rootActions()) == 6);

    const auto action = replay->get_rootActions()[2];
    assert(action->get_flags() == model::RdcActionFlags::Drawcall);

    assert_debug_vertex_step_by_step(lifetime, replay, action->get_eventId());
    assert_debug_vertex_with_breakpoints(lifetime, replay, action->get_eventId());
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
