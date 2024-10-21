#include "RenderDocDebugSession.h"

#include "RenderDocReplay.h"
#include "types/wrapper.h"
#include "util/RenderDocActionHelpers.h"
#include "util/RenderDocConverterUtils.h"
#include <api/replay/renderdoc_replay.h>

#include <fstream>

namespace jetbrains::renderdoc {

class RenderDocDebugSessionData {
public:
  const bool is_draw_call_debug;
  const ShaderStage stage;
  const RenderDocReplay * replay;
  rd::Wrapper<RenderDocDrawCallDebugSession> draw_call_session;
  std::unordered_map<std::wstring, std::vector<model::RdcSourceBreakpoint>> source_breakpoints;

  RenderDocDebugSessionData(bool is_draw_call, const RenderDocReplay * replay, rd::Wrapper<RenderDocDrawCallDebugSession> &&draw_call_session, const ShaderStage &stage)
  : is_draw_call_debug(is_draw_call), stage(stage), replay(replay), draw_call_session(std::move(draw_call_session)) {}
};

RenderDocDebugSession::RenderDocDebugSession(const rd::Lifetime& session_lifetime, const RenderDocReplay *replay, rd::Wrapper<RenderDocDrawCallDebugSession> draw_call_session, const ShaderStage &stage, bool is_draw_call_debug)
:  data(std::make_shared<RenderDocDebugSessionData>(is_draw_call_debug, replay, std::move(draw_call_session), stage)) {
  get_stepInto().advise(session_lifetime, [this] { step_into(); });
  get_stepOver().advise(session_lifetime,[this] { step_over(); });
  get_addBreakpoint().advise(session_lifetime, [this](const auto& req) { add_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_removeBreakpoint().advise(session_lifetime, [this](const auto &req) { remove_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_resume().advise(session_lifetime, [this] { resume(); });
}
std::vector<rd::Wrapper<model::RdcSourceFile>> const & RenderDocDebugSession::get_sourceFiles() const {
  return data->draw_call_session->get_sourceFiles();
}

void RenderDocDebugSession::step_into() const {
  navigate_to_next_not_null_stack([&] { return data->draw_call_session->step_into(); });
}

void RenderDocDebugSession::step_over() const {
  navigate_to_next_not_null_stack([&] { return data->draw_call_session->step_over(); });
}

void RenderDocDebugSession::resume() const {
  navigate_to_next_not_null_stack([&] { return data->draw_call_session->resume(); });
}

void RenderDocDebugSession::add_breakpoint(uint32_t source_file_index, uint32_t line) const {
  data->draw_call_session->add_breakpoint(source_file_index, line);
}

void RenderDocDebugSession::remove_breakpoint(uint32_t source_file_index, uint32_t line) const {
  data->draw_call_session->remove_breakpoint(source_file_index, line);
}

bool RenderDocDebugSession::step_to_next_draw_call() const {
  if (!data->draw_call_session)
    return false;
  const ActionDescription * action = data->draw_call_session->get_action();

  while ((action = helpers::find_action(helpers::get_next_action(action), helpers::is_draw_call))) {
    if (const auto next_session = data->stage == ShaderStage::Vertex ? data->replay->start_debug_vertex(action) : data->replay->start_debug_pixel(action)) {
      data->draw_call_session = next_session;
      data->draw_call_session->map_and_add_breakpoints_from_sources(data->source_breakpoints);
      return true;
    }
  }

  return false;
}

void RenderDocDebugSession::navigate_to_next_not_null_stack(const std::function<rd::Wrapper<model::RdcDebugStack>()> &func) const {
  rd::Wrapper<model::RdcDebugStack> stack;
  bool call_changed = false;

  while (!((stack = func()))) {
    if (data->is_draw_call_debug || !((call_changed = step_to_next_draw_call()))) {
      get_drawCallSession().set(data->draw_call_session);
      get_stageInfo().set(rd::Wrapper<model::RdcStageInfo>(nullptr));
      get_currentStack().set(rd::Wrapper<model::RdcDebugStack>(nullptr));
      return;
    }
  }

  get_drawCallSession().set(data->draw_call_session);
  get_stageInfo().set(rd::wrapper::make_wrapper<model::RdcStageInfo>(data->draw_call_session->get_source_variables(), data->draw_call_session->get_variable_changes(), call_changed));
  get_currentStack().set(stack);
}

void RenderDocDebugSession::add_breakpoints_from_sources(const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const {
  for (const auto& breakpoint : breakpoints) {
    data->source_breakpoints[breakpoint->get_sourceFilePath()].push_back(*breakpoint);
  }
  if (data->draw_call_session)
    data->draw_call_session->map_and_add_breakpoints_from_sources(data->source_breakpoints);
}

} // namespace jetbrains::renderdoc
