#include "RenderDocReplay.h"

#include "RenderDocTexturePreviewService.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocActionHelpers.h"

#include <api/replay/renderdoc_replay.h>

namespace jetbrains::renderdoc
{
namespace replay::helpers {

std::vector<rd::Wrapper<model::RdcAction>> get_root_actions(IReplayController* controller) {
  return renderdoc::helpers::get_actions_recursive(controller->GetRootActions(), controller->GetStructuredFile());
}

model::RdcGraphicsApi get_graphics_api(IReplayController *controller) {
  switch (controller->GetAPIProperties().pipelineType) {
  case GraphicsAPI::D3D11:
    return model::RdcGraphicsApi::D3D11;
  case GraphicsAPI::D3D12:
    return model::RdcGraphicsApi::D3D12;
  case GraphicsAPI::OpenGL:
    return model::RdcGraphicsApi::OpenGL;
  case GraphicsAPI::Vulkan:
    return model::RdcGraphicsApi::Vulkan;
  default:
    return model::RdcGraphicsApi::Unknown;
  }
}

}

RenderDocReplay::RenderDocReplay(IReplayController *controller) : RdcCapture{replay::helpers::get_graphics_api(controller), replay::helpers::get_root_actions(controller)},
controller(controller, [](IReplayController* ptr) { ptr->Shutdown(); }), mapper(std::make_shared<RenderDocLineBreakpointsMapper>()), texture_previewer(std::make_shared<RenderDocTexturePreviewService>(controller)) {
  get_debugVertex().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return debug_vertex(lifetime, req);
  });
  get_debugPixel().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return debug_pixel(lifetime, req);
  });
  get_tryDebugVertex().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return try_debug_vertex(lifetime, req);
  });
  get_tryDebugPixel().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return try_debug_pixel(lifetime, req);
  });
  get_getTextureRGBBuffer().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return get_textureRGBBuffer(lifetime, req);
  });
}

[[nodiscard]] std::vector<rd::Wrapper<model::RdcWindowOutputData>> RenderDocReplay::get_textureRGBBuffer(const rd::Lifetime &session_lifetime, uint32_t event_id) const {
  const auto event = helpers::find_action(controller->GetRootActions().begin(), [event_id](const ActionDescription &a) {
    const auto next = helpers::get_next_action(&a);
    return a.eventId <= event_id && (next ? next->eventId > event_id : true);
  });
  if (!event)
    return {};
  controller->SetFrameEvent(event->eventId, true);
  const rdcarray<Descriptor> out_targets = controller->GetPipelineState().GetOutputTargets();
  return texture_previewer->get_buffers(event->eventId, out_targets);
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_vertex(const rd::Lifetime &session_lifetime, const uint32_t event_id) const  {
  constexpr DebugInput debug_input = {0};
  const auto action = helpers::find_action(controller->GetRootActions().begin(), [&event_id](const ActionDescription &a){ return a.eventId == event_id; });
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_vertex(action), ShaderStage::Vertex, debug_input, true);
  session->step_into();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_pixel(const rd::Lifetime &session_lifetime, const model::RdcDebugPixelInput &input) const  {
  const DebugInput debug_input = {input.get_x(), input.get_y()};
  const auto action = helpers::find_action(controller->GetRootActions().begin(), [event_id = input.get_eventId()](const ActionDescription &a){ return a.eventId == event_id; });
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_pixel(action, debug_input), ShaderStage::Pixel, debug_input, true);
  session->step_into();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::try_debug_vertex(const rd::Lifetime &session_lifetime, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const {
  constexpr DebugInput debug_input = {0};
  const ActionDescription *action = helpers::find_action(controller->GetRootActions().begin(), helpers::is_draw_call);
  auto&&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_vertex(action), ShaderStage::Vertex, debug_input, false);
  session->add_breakpoints_from_sources(breakpoints);
  session->resume();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::try_debug_pixel(const rd::Lifetime &session_lifetime, const model::RdcDebugPixelInput &input) const {
  const DebugInput debug_input = {input.get_x(), input.get_y()};
  const auto action = helpers::find_action(controller->GetRootActions().begin(), helpers::is_draw_call);
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_pixel(action, debug_input), ShaderStage::Pixel, debug_input, false);
  session->add_breakpoints_from_sources(input.get_breakpoints());
  session->resume();
  return session;
}

rd::Wrapper<RenderDocDrawCallDebugSession> RenderDocReplay::start_debug_vertex(const ActionDescription *action) const  {
  controller->SetFrameEvent(action->eventId, true);

  const auto &pipeline = controller->GetPipelineState();
  const auto shader = pipeline.GetShaderReflection(ShaderStage::Vertex);
  ShaderDebugTrace *trace = controller->DebugVertex(0, 0, 0, IReplayController::NoPreference);
  const auto &drawCallSession = rd::wrapper::make_wrapper<RenderDocDrawCallDebugSession>(action, controller, trace, &shader->debugInfo, shader);
  if (drawCallSession)
    mapper->register_sources_usages_in_draw_call(action->eventId, drawCallSession->get_sourceFiles());
  return drawCallSession;
}

rd::Wrapper<RenderDocDrawCallDebugSession> RenderDocReplay::start_debug_pixel(const ActionDescription *action, DebugInput input) const {
  controller->SetFrameEvent(action->eventId, true);

  const auto &pipeline = controller->GetPipelineState();
  const ShaderReflection * shader = pipeline.GetShaderReflection(ShaderStage::Pixel);
  const DebugPixelInputs inputs;
  ShaderDebugTrace *trace = controller->DebugPixel(input.pixel.x, input.pixel.y, inputs);
  if (trace == nullptr)
    return rd::Wrapper<RenderDocDrawCallDebugSession>(nullptr);
  const auto &drawCallSession = rd::wrapper::make_wrapper<RenderDocDrawCallDebugSession>(action, controller, trace, &shader->debugInfo, shader);
  if (drawCallSession)
    mapper->register_sources_usages_in_draw_call(action->eventId, drawCallSession->get_sourceFiles());
  return drawCallSession;
}

}
