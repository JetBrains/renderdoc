#include <cassert>

#include "../renderdoc_service_test_utils.h"
#include "RenderDocServiceApi.h"

using namespace jetbrains::renderdoc;

void assert_actions_collection(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
    assert(std::size(replay->get_rootActions()) == 6);

    const auto file_usages = get_file_usages_in_events(replay);

    assert(file_usages.size() == 0);

    const auto action = replay->get_rootActions()[2];
    assert(action->get_flags() == model::RdcActionFlags::Drawcall);
}

void assert_debug_vertex_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, uint32_t eventId) {
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugVertexInput(eventId, 30, {})), true);

  const auto debug_session = replay->debug_vertex(lifetime, model::RdcDebugVertexInput(eventId, 0, {}));
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
  const auto debug_session = replay->debug_vertex(lifetime, model::RdcDebugVertexInput(eventId, 0, {}));
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

void assert_try_debug_vertex(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugVertexInput(0, 30, {})), false);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugVertexInput(0, 0, {})), false);
}

void assert_debug_pixel_step_by_step(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, uint32_t eventId) {
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugPixelInput(eventId, 350, 122, {})), true);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugPixelInput(eventId, 2000, 2000, {})), true);
}

void assert_try_debug_pixel(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugPixelInput(0, 350, 122, {})), false);
  assert_session_finishes_immediately(lifetime, replay, RdcDebugInput(model::RdcDebugPixelInput(0, 2000, 2000, {})), false);
}

void assert_vertices_table(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, -1);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 6);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 14);
    assert(vertices == nullptr);
  }
  {
    const auto &vertices = replay->get_vertices_inoutputs(lifetime, 13);
    assert(vertices != nullptr);
    assert(vertices->get_input_indices() == std::vector<uint32_t>({ 0, 1, 2 }));
    assert(vertices->get_input_columns() == std::vector({rd::Wrapper<std::wstring>(L"inPos"), rd::Wrapper<std::wstring>(L"inColor")}));
    assert(vertices->get_output_columns() == std::vector({rd::Wrapper<std::wstring>(L"gl_Position"), rd::Wrapper<std::wstring>(L"outColor") }));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[0], std::vector<std::vector<float>>({ { 1, 1, 0 }, { 1, 0, 0 } }));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[1], std::vector<std::vector<float>>({ { -1, 1, 0 }, { 0, 1, 0 } }));
    assert_float_2d_vectors_are_equal(vertices->get_inputs()[2], std::vector<std::vector<float>>({ { 0, -1, 0 }, { 0, 0, 1 } }));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[0], std::vector<std::vector<float>>({ { 0.974279, 1.73205, 1.50588, 2.5 }, { 1, 0, 0 } }));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[1], std::vector<std::vector<float>>({ { -0.974279, 1.73205, 1.50588, 2.5 }, { 0, 1, 0 } }));
    assert_float_2d_vectors_are_equal(vertices->get_outputs()[2], std::vector<std::vector<float>>({ { 0, -1.73205, 1.50588, 2.5 }, { 0, 0, 1 } }));
  }
}

void assert_texture_outputs(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay) {
  const auto &outputs_root = replay->get_pixel_inoutputs(lifetime, -1);

  {
    const auto &outputs_last = replay->get_pixel_inoutputs(lifetime, 16);
    assert(outputs_root == outputs_last);
    assert(outputs_root->get_inputs().empty());
    assert(outputs_root->get_colorOutputs().size() == 1);
    assert(!outputs_root->get_depthOutput());
    assert(outputs_root->get_colorOutputs()[0]->get_width() == 1280);
    assert(outputs_root->get_colorOutputs()[0]->get_height() == 720);
  }
  {
    const auto &outputs_leaf = replay->get_pixel_inoutputs(lifetime, 13);
    assert(outputs_leaf->get_inputs().empty());
    assert(outputs_leaf->get_colorOutputs().size() == 1);
    assert(outputs_leaf->get_depthOutput());
    assert(outputs_leaf->get_colorOutputs()[0]->get_width() == 1280);
    assert(outputs_leaf->get_colorOutputs()[0]->get_height() == 720);
  }
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
    assert_actions_collection(lifetime, replay);

    const auto action = replay->get_rootActions()[2];
    assert_debug_vertex_step_by_step(lifetime, replay, action->get_eventId());
    assert_debug_vertex_with_breakpoints(lifetime, replay, action->get_eventId());
    assert_try_debug_vertex(lifetime, replay);

    assert_debug_pixel_step_by_step(lifetime, replay, action->get_eventId());
    assert_try_debug_pixel(lifetime, replay);

    assert_vertices_table(lifetime, replay);
    assert_texture_outputs(lifetime, replay);
    return 0;
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
