#include "RenderDocDebugSession.h"

#include "RenderDocReplay.h"
#include "types/wrapper.h"
#include "util/RenderDocActionHelpers.h"
#include "util/RenderDocConverterUtils.h"

#include <api/replay/renderdoc_replay.h>
#include <optional>

namespace jetbrains::renderdoc {

class RenderDocDebugSessionData {
public:
  const bool is_draw_call_debug;
  const ShaderStage stage;
  const RenderDocReplay * replay;
  const ActionDescription * current_action;
  bool was_inside_draw_call = true;
  rd::Wrapper<RenderDocDrawCallDebugSession> draw_call_session;
  std::unordered_set<model::RdcSourceBreakpoint, RdcSourceBreakpointHash> source_breakpoints;

  RenderDocDebugSessionData(bool is_draw_call, const RenderDocReplay * replay, rd::Wrapper<RenderDocDrawCallDebugSession> &&draw_call_session, const ShaderStage &stage)
  : is_draw_call_debug(is_draw_call), stage(stage), replay(replay), current_action(draw_call_session ? draw_call_session->get_action() : nullptr), draw_call_session(draw_call_session), source_breakpoints({}) {}
};

RenderDocDebugSession::RenderDocDebugSession(const rd::Lifetime& session_lifetime, const RenderDocReplay *replay, rd::Wrapper<RenderDocDrawCallDebugSession> draw_call_session, const ShaderStage &stage, DebugInput input, bool is_draw_call_debug)
:  input(input), data(std::make_shared<RenderDocDebugSessionData>(is_draw_call_debug, replay, std::move(draw_call_session), stage)) {
  get_stepInto().advise(session_lifetime, [this] { step_into(); });
  get_stepOver().advise(session_lifetime,[this] { step_over(); });
  get_addLineBreakpoint().advise(session_lifetime, [this](const auto& req) { add_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_addSourceBreakpoint().advise(session_lifetime, [this](const auto& req) { add_source_breakpoint(req); });
  get_removeLineBreakpoint().advise(session_lifetime, [this](const auto &req) { remove_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_removeSourceBreakpoint().advise(session_lifetime, [this](const auto &req) { remove_source_breakpoint(req); });
  get_resume().advise(session_lifetime, [this] { resume(); });
}
std::vector<rd::Wrapper<model::RdcSourceFile>> RenderDocDebugSession::get_sourceFiles() const {
  if (!data->draw_call_session)
    return {};
  return data->draw_call_session->get_sourceFiles();
}

void RenderDocDebugSession::step_into() const {
  step_to_next_not_null_stack([&] {
    if (!data->draw_call_session)
      return rd::Wrapper<model::RdcDebugStack>(nullptr);
    return data->draw_call_session->step_into();
  });
}

void RenderDocDebugSession::step_over() const {
  step_to_next_not_null_stack([&] {
    if (!data->draw_call_session)
      return rd::Wrapper<model::RdcDebugStack>(nullptr);
    return data->draw_call_session->step_over();
  }, true);
}

void RenderDocDebugSession::resume() const {
  resume_to_next_not_null_stack();
}

void RenderDocDebugSession::add_breakpoint(int32_t source_file_index, uint32_t line) const {
  if (data->draw_call_session)
    data->draw_call_session->add_breakpoint(source_file_index, line);
}

void RenderDocDebugSession::add_source_breakpoint(const rd::Wrapper<model::RdcSourceBreakpoint> &breakpoint) const {
  data->source_breakpoints.insert(*breakpoint);
  if (data->draw_call_session) {
    for (const auto &bp : data->draw_call_session->map_breakpoints_from_sources(data->replay->mapper.get(), { *breakpoint })) {
      add_breakpoint(bp.get_sourceFileIndex(), bp.get_line());
    }
  }
}

void RenderDocDebugSession::remove_breakpoint(int32_t source_file_index, uint32_t line) const {
  if (data->draw_call_session)
    data->draw_call_session->remove_breakpoint(source_file_index, line);
}

void RenderDocDebugSession::remove_source_breakpoint(const rd::Wrapper<model::RdcSourceBreakpoint> &breakpoint) const {
  data->source_breakpoints.erase(*breakpoint);
  if (data->draw_call_session) {
    for (const auto &bp : data->draw_call_session->map_breakpoints_from_sources(data->replay->mapper.get(), { *breakpoint })) {
      remove_breakpoint(bp.get_sourceFileIndex(), bp.get_line());
    }
  }
}

bool RenderDocDebugSession::step_to_next_draw_call() const {
  data->current_action = helpers::find_action(helpers::get_next_action(data->current_action), helpers::is_draw_call);

  if (!data->current_action) {
    data->draw_call_session = rd::Wrapper<RenderDocDrawCallDebugSession>(nullptr);
    return false;
  }

  data->draw_call_session = data->stage == ShaderStage::Vertex ? data->replay->start_debug_vertex(data->current_action, input) : data->replay->start_debug_pixel(data->current_action, input);
  if (data->draw_call_session) {
    for (const auto &bp : data->draw_call_session->map_breakpoints_from_sources(data->replay->mapper.get(), data->source_breakpoints)) {
      add_breakpoint(bp.get_sourceFileIndex(), bp.get_line());
    }
  } else {
    data->was_inside_draw_call = true;
  }

  return true;
}

void RenderDocDebugSession::resume_to_next_not_null_stack() const {
  int64_t previous_id = -1;
  if (get_sessionState().has_value() && get_sessionState().get())
    previous_id = get_sessionState().get()->get_currentStack().get_drawCallId();
  rd::Wrapper<model::RdcDebugStack> stack;
  while (!data->draw_call_session || !((stack = data->draw_call_session->resume()))) {
    if (data->is_draw_call_debug || !step_to_next_draw_call()) {
      get_sessionState().set(rd::Wrapper<model::RdcSessionState>(nullptr));
      return;
    }
  }

  data->was_inside_draw_call = true;
  const auto& draw_call = previous_id != stack->get_drawCallId() ?
    static_cast<const rd::Wrapper<model::RdcDrawCallDebugSession>&>(data->draw_call_session) : rd::Wrapper<model::RdcDrawCallDebugSession>(nullptr);
  get_sessionState().set(rd::wrapper::make_wrapper<model::RdcSessionState>(
    stack, rd::wrapper::make_wrapper<model::RdcStageInfo>(
        data->draw_call_session->get_source_variables(), data->draw_call_session->get_updated_variables()
      ), draw_call));
}

void RenderDocDebugSession::step_to_next_not_null_stack(const std::function<rd::Wrapper<model::RdcDebugStack>()> &func, bool step_over) const {
  const auto &previous_state = get_sessionState().has_value() ? get_sessionState().get() : rd::Wrapper<model::RdcSessionState>(nullptr);
  const int64_t previous_id = previous_state ? previous_state->get_currentStack().get_drawCallId() : -1;

  rd::Wrapper<model::RdcDebugStack> stack;
  const bool go_to_next = step_over && previous_state != nullptr && previous_state->get_currentStack().get_stepIndex() == -1 || !data->draw_call_session;
  if (go_to_next || !((stack = func()))) {
    if (data->is_draw_call_debug || !data->was_inside_draw_call && !step_to_next_draw_call()) {
      get_sessionState().set(rd::Wrapper<model::RdcSessionState>(nullptr));
      return;
    }

    data->was_inside_draw_call = false;
    const auto& draw_call = previous_id != data->current_action->eventId ?
      static_cast<const rd::Wrapper<model::RdcDrawCallDebugSession>&>(data->draw_call_session) : rd::Wrapper<model::RdcDrawCallDebugSession>(nullptr);
    get_sessionState().set(rd::wrapper::make_wrapper<model::RdcSessionState>(
      rd::wrapper::make_wrapper<model::RdcDebugStack>(data->current_action->eventId, -1, -1, 0, 0, 0, 0),
      rd::Wrapper<model::RdcStageInfo>(nullptr), draw_call));
    return;
  }

  data->was_inside_draw_call = true;
  const auto& draw_call = previous_id != stack->get_drawCallId() ?
    static_cast<const rd::Wrapper<model::RdcDrawCallDebugSession>&>(data->draw_call_session) : rd::Wrapper<model::RdcDrawCallDebugSession>(nullptr);
  get_sessionState().set(rd::wrapper::make_wrapper<model::RdcSessionState>(
      stack, rd::wrapper::make_wrapper<model::RdcStageInfo>(
          data->draw_call_session->get_source_variables(), data->draw_call_session->get_updated_variables()
        ), draw_call));
}

void RenderDocDebugSession::add_breakpoints_from_sources(const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const {
  for (const auto& breakpoint : breakpoints) {
    data->source_breakpoints.insert(*breakpoint);
  }
  if (data->draw_call_session) {
    for (const auto &bp : data->draw_call_session->map_breakpoints_from_sources(data->replay->mapper.get(), data->source_breakpoints)) {
      add_breakpoint(bp.get_sourceFileIndex(), bp.get_line());
    }
  }
}

} // namespace jetbrains::renderdoc
