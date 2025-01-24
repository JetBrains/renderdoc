#include "RenderDocReplay.h"

#include "RenderDocMeshPreviewService.h"
#include "RenderDocTexturePreviewService.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocActionHelpers.h"
#include "util/StringUtils.h"

#include <api/replay/renderdoc_replay.h>

namespace jetbrains::renderdoc {
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


void RenderDocReplay::calculate_effective_event_ids(const ActionDescription *parent) { // NOLINT(*-no-recursion)
  const auto &descriptions = parent ? parent->children : controller->GetRootActions();
  const int64_t parent_event_id = parent ? static_cast<int64_t>(parent->eventId) : -1;
  if (descriptions.empty()) return;

  auto &effective_id = effective_event_ids[parent_event_id];
  const auto &last_child = descriptions.back();
  effective_id = last_child.eventId;
  if (last_child.flags & ActionFlags::PopMarker)
    --effective_id;

  std::vector<rd::Wrapper<model::RdcAction>> actions;
  for (const auto &it : descriptions) {
    calculate_effective_event_ids(&it);

    if (it.eventId == effective_id)
      effective_id = get_effective_event_id(it.eventId);
  }
}

uint32_t RenderDocReplay::get_effective_event_id(int64_t event_id) const {
  if (const auto it = effective_event_ids.find(event_id); it != effective_event_ids.end())
    return it->second;
  return event_id;
}

RenderDocReplay::RenderDocReplay(IReplayController *controller) : RdcCapture{replay::helpers::get_graphics_api(controller), replay::helpers::get_root_actions(controller)},
controller(controller, [](IReplayController* ptr) { ptr->Shutdown(); }), mapper(std::make_shared<RenderDocLineBreakpointsMapper>()),
texture_previewer(std::make_shared<RenderDocTexturePreviewService>(controller)), mesh_previewer(std::make_shared<RenderDocMeshPreviewService>(controller)) {
  calculate_effective_event_ids(nullptr);

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
  get_getVertexStageInOutputs().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return get_vertices_inoutputs(lifetime, req);
  });
}

[[nodiscard]] rd::Wrapper<model::RdcTextureOutputs> RenderDocReplay::get_textureRGBBuffer(const rd::Lifetime &session_lifetime, int64_t event_id) const {
  const auto eid = get_effective_event_id(event_id);
  const auto event = helpers::get_action(controller->GetRootActions(), eid);
  controller->SetFrameEvent(eid, true);
  return texture_previewer->get_outputs(event, eid);
}

rd::Wrapper<model::RdcVertexStageInOutputs> RenderDocReplay::get_vertices_inoutputs(const rd::Lifetime &session_lifetime, int64_t event_id) const {
  const auto eid = get_effective_event_id(event_id);
  const auto event = helpers::get_action(controller->GetRootActions(), eid);
  if (!event)
    return rd::Wrapper<model::RdcVertexStageInOutputs>(nullptr);
  controller->SetFrameEvent(eid, true);
  return mesh_previewer->get_vertices(event);
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_vertex(const rd::Lifetime &session_lifetime, const model::RdcDebugVertexInput &input) const {
  const DebugInput debug_input = {input.get_vertex()};
  const auto action = helpers::find_action(controller->GetRootActions().begin(), [id = input.get_eventId()](const ActionDescription &a) { return a.eventId == id; });
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_vertex(action, debug_input), ShaderStage::Vertex, debug_input, true);
  session->step_into();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_pixel(const rd::Lifetime &session_lifetime, const model::RdcDebugPixelInput &input) const {
  const DebugInput debug_input = {input.get_x(), input.get_y()};
  const auto action = helpers::find_action(controller->GetRootActions().begin(), [event_id = input.get_eventId()](const ActionDescription &a) { return a.eventId == event_id; });
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_pixel(action, debug_input), ShaderStage::Pixel, debug_input, true);
  session->step_into();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::try_debug_vertex(const rd::Lifetime &session_lifetime, const model::RdcDebugVertexInput &input) const {
  const DebugInput debug_input = {input.get_vertex()};
  const ActionDescription *action = helpers::find_action(controller->GetRootActions().begin(), helpers::is_draw_call);
  auto &&session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, this, start_debug_vertex(action, debug_input), ShaderStage::Vertex, debug_input, false);
  session->add_breakpoints_from_sources(input.get_breakpoints());
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

rd::Wrapper<RenderDocDrawCallDebugSession> RenderDocReplay::start_debug_vertex(const ActionDescription *action, DebugInput input) const {
  controller->SetFrameEvent(action->eventId, true);

  const auto &pipeline = controller->GetPipelineState();
  const auto shader = pipeline.GetShaderReflection(ShaderStage::Vertex);
  ShaderDebugTrace *trace = controller->DebugVertex(input.vertex_id, 0, 0, IReplayController::NoPreference);
  const auto &drawCallSession = rd::wrapper::make_wrapper<RenderDocDrawCallDebugSession>(action, controller, trace, &shader->debugInfo, shader);
  if (drawCallSession)
    mapper->register_sources_usages_in_draw_call(action->eventId, drawCallSession->get_sourceFiles());
  return drawCallSession;
}

rd::Wrapper<RenderDocDrawCallDebugSession> RenderDocReplay::start_debug_pixel(const ActionDescription *action, DebugInput input) const {
  controller->SetFrameEvent(action->eventId, true);

  const auto &pipeline = controller->GetPipelineState();
  const ShaderReflection *shader = pipeline.GetShaderReflection(ShaderStage::Pixel);
  const DebugPixelInputs inputs;
  ShaderDebugTrace *trace = controller->DebugPixel(input.pixel.x, input.pixel.y, inputs);
  if (trace == nullptr)
    return rd::Wrapper<RenderDocDrawCallDebugSession>(nullptr);
  const auto &drawCallSession = rd::wrapper::make_wrapper<RenderDocDrawCallDebugSession>(action, controller, trace, &shader->debugInfo, shader);
  if (drawCallSession)
    mapper->register_sources_usages_in_draw_call(action->eventId, drawCallSession->get_sourceFiles());
  return drawCallSession;
}

} // namespace jetbrains::renderdoc
