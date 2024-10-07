#include "RenderDocReplay.h"

#include <string>
#include <api/replay/renderdoc_replay.h>

#include "util/StringUtils.h"

namespace jetbrains::renderdoc
{
namespace replay::helpers {

model::RdcActionFlags map_flags(const ActionFlags flags) {
  model::RdcActionFlags rdc_flags = {};
  if (flags & ActionFlags::Drawcall)
    rdc_flags |= model::RdcActionFlags::Drawcall;
  if (flags & ActionFlags::MeshDispatch)
    rdc_flags |= model::RdcActionFlags::MeshDispatch;
  return rdc_flags;
}

std::vector<rd::Wrapper<model::RdcAction>> get_actions_recursive(const rdcarray<ActionDescription> &descriptions, const SDFile &file) { // NOLINT(*-no-recursion)
  std::vector<rd::Wrapper<model::RdcAction>> actions;
  for (const auto &it : descriptions) {
    actions.emplace_back(rd::wrapper::make_wrapper<model::RdcAction>(it.eventId, it.actionId, StringUtils::Utf8ToWide(it.GetName(file)), map_flags(it.flags), get_actions_recursive(it.children, file)));
  }
  return actions;
}

std::vector<rd::Wrapper<model::RdcAction>> get_root_actions(IReplayController* controller) {
  return get_actions_recursive(controller->GetRootActions(), controller->GetStructuredFile());
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

const ActionDescription *find_action(const ActionDescription *begin, const std::function<bool(const ActionDescription &)> &predicate)
{
  const ActionDescription *action = begin;
  while(action != nullptr)
  {
    if(predicate(*action))
      break;

    if(!action->children.empty())
    {
      action = action->children.begin();
    }
    else
    {
      if(!action->next && action->parent)
        action = action->parent;

      action = action->next;
    }
  }
  return action;
}

}

RenderDocReplay::RenderDocReplay(IReplayController *controller) : RdcCapture{replay::helpers::get_graphics_api(controller), replay::helpers::get_root_actions(controller)}, controller(controller, [](IReplayController* ptr) { ptr->Shutdown(); }) {
  get_debugVertex().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return debug_vertex(lifetime, req);
  });
  get_debugPixel().set([this](const rd::Lifetime& lifetime, const auto& req) {
    return debug_pixel(lifetime, req);
  });
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug(const ShaderStage &stage, const rd::Lifetime &session_lifetime, const uint32_t event_id) const {
  controller->SetFrameEvent(event_id, true);

  const auto &pipeline = controller->GetPipelineState();
  const auto shader = pipeline.GetShaderReflection(stage);
  ShaderDebugTrace *trace;
  if (stage == ShaderStage::Vertex) {
    trace = controller->DebugVertex(0, 0, 0, IReplayController::NoPreference);
  } else {
    const DebugPixelInputs inputs;
    trace = controller->DebugPixel(400, 400, inputs);
  }
  auto&& session = rd::wrapper::make_wrapper<RenderDocDebugSession>(session_lifetime, controller, trace, &shader->debugInfo);
  session->step_into();
  return session;
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_vertex(const rd::Lifetime &session_lifetime, const uint32_t event_id) const {
  return debug(ShaderStage::Vertex, session_lifetime, event_id);
}

rd::Wrapper<RenderDocDebugSession> RenderDocReplay::debug_pixel(const rd::Lifetime &session_lifetime, const uint32_t event_id) const {
  return debug(ShaderStage::Pixel, session_lifetime, event_id);
}

}
