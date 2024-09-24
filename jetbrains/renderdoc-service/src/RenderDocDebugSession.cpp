#include "RenderDocDebugSession.h"

#include "RenderDocModel/RdcDebugStack.Generated.h"
#include "types/wrapper.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocConverterUtils.h"
#include "util/StringUtils.h"
#include <api/replay/renderdoc_replay.h>

#include <cstddef>
#include <stack>
#include <utility>

namespace jetbrains::renderdoc {

struct RenderDocBreakpoint {
  uint32_t source_file;
  uint32_t line;

  RenderDocBreakpoint(const uint32_t source_file, const uint32_t line) : source_file(source_file), line(line) {}
  explicit RenderDocBreakpoint(const LineColumnInfo &line_column_info) : source_file(line_column_info.fileIndex), line(line_column_info.lineStart) {}

  friend std::size_t hash_value(const RenderDocBreakpoint &obj) {
    std::size_t seed = 0x54B49F34;
    seed ^= (seed << 6) + (seed >> 2) + 0x45EE8067 + static_cast<std::size_t>(obj.source_file);
    seed ^= (seed << 6) + (seed >> 2) + 0x6356C6E9 + static_cast<std::size_t>(obj.line);
    return seed;
  }
  friend bool operator==(const RenderDocBreakpoint &lhs, const RenderDocBreakpoint &rhs) { return lhs.source_file == rhs.source_file && lhs.line == rhs.line; }
  friend bool operator!=(const RenderDocBreakpoint &lhs, const RenderDocBreakpoint &rhs) { return !(lhs == rhs); }

  struct hash {
    std::size_t operator()(const RenderDocBreakpoint& breakpoint) const noexcept
    {
      return hash_value(breakpoint);
    }
  };
};

struct RenderDocDebugSessionData {
  ShaderDebugTrace *trace;
  std::shared_ptr<IReplayController> controller;
  rdcarray<ShaderDebugState> states;
  const ShaderDebugInfo *debug_info;
  ShaderDebugState *current_state = states.begin();
  InstructionSourceInfo current_instruction;
  std::stack<std::pair<size_t, InstructionSourceInfo>> calltrace;
  std::unordered_set<RenderDocBreakpoint, RenderDocBreakpoint::hash> breakpoints;
  size_t current_callstack_size;

  RenderDocDebugSessionData(ShaderDebugTrace *trace, const std::shared_ptr<IReplayController> &controller, const ShaderDebugInfo *debug_info)
      : trace(trace), controller(controller), debug_info(debug_info), current_callstack_size(0) {
    current_instruction.lineInfo.fileIndex = -1;
  }

  [[nodiscard]] bool do_step() {
    if (current_state == states.end()) {
      states = controller->ContinueDebug(trace->debugger);
      current_state = states.begin();
    } else {
      ++current_state;
    }

    if (current_state == states.end()) {
      calltrace = {};
      current_callstack_size = 0;
      return false;
    }

    const auto callstack_size = current_state->callstack.size();
    if (current_callstack_size < callstack_size) {
      calltrace.emplace(callstack_size, current_instruction);
      current_callstack_size = callstack_size;
    } else if (current_callstack_size > callstack_size) {
      while (!calltrace.empty() && calltrace.top().first > callstack_size) {
        calltrace.pop();
      }
      current_callstack_size = callstack_size;
    }
    current_instruction = get_instruction(current_state->nextInstruction);
    return true;
  }

  [[nodiscard]] bool do_source_step() {
    auto call_line_info = !calltrace.empty() ? calltrace.top().second.lineInfo : LineColumnInfo();
    auto last_line_info = current_instruction.lineInfo;
    while (do_step()) {
      const auto &line_info = current_instruction.lineInfo;
      if (!line_info.SourceEqual(last_line_info)) {
        if (line_info.SourceEqual(call_line_info)) {
          last_line_info = call_line_info;
          call_line_info = !calltrace.empty() ? calltrace.top().second.lineInfo : LineColumnInfo();
          continue;
        }
        return line_info.fileIndex >= 0;
      }
    }
    return false;
  }

  [[nodiscard]] InstructionSourceInfo get_instruction(const uint32_t instruction) const {
    InstructionSourceInfo search;
    search.instruction = instruction;
    const auto it = std::lower_bound(trace->instInfo.begin(), trace->instInfo.end(), search);
    if (it == trace->instInfo.end()) {
      throw std::runtime_error(StringUtils::BuildString("Couldn't find instruction info for ", search.instruction));
    }

    return *it;
  }

  ~RenderDocDebugSessionData() { controller->FreeTrace(trace); }
};

std::vector<rd::Wrapper<model::RdcSourceFile>> RenderDocDebugSession::get_source_files(const ShaderDebugInfo *debug_info) {
  std::vector<rd::Wrapper<model::RdcSourceFile>> source_files;
  const auto source_files_count = debug_info->files.count();
  for (auto index = 0; index < source_files_count; ++index) {
    const auto &[filename, contents] = debug_info->files[index];
    source_files.emplace_back(model::RdcSourceFile(StringUtils::Utf8ToWide(filename), StringUtils::Utf8ToWide(contents)));
  }
  return source_files;
}

RenderDocDebugSession::RenderDocDebugSession(const rd::Lifetime& session_lifetime, const std::shared_ptr<IReplayController> &controller, ShaderDebugTrace *trace, const ShaderDebugInfo *debug_info)
    :  RdcDebugSession(RenderDocConverterUtils::convertDebugTrace(*trace), get_source_files(debug_info)), data(std::make_shared<RenderDocDebugSessionData>(trace, controller, debug_info)) {
  get_stepInto().advise(session_lifetime, [this] { step_into(); });
  get_stepOver().advise(session_lifetime,[this] { step_over(); });
  get_addBreakpoint().advise(session_lifetime, [this](const auto& req) { add_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_removeBreakpoint().advise(session_lifetime, [this](const auto &req) { remove_breakpoint(req.get_sourceFileIndex(), req.get_line()); });
  get_resume().advise(session_lifetime, [this] { resume(); });
  get_getStageVariableInfo().set([this](const auto &) { return get_stage_variable_info(); });
}

void RenderDocDebugSession::step_into() const {
  if (!data->do_source_step()) {
    get_currentStack().set(rd::Wrapper<model::RdcDebugStack>(nullptr));
    return;
  }

  get_currentStack().set(make_debug_stack(data->current_state->stepIndex, data->current_instruction.lineInfo));
}

rd::Wrapper<model::RdcDebugStack> RenderDocDebugSession::make_debug_stack(const uint32_t step_index, LineColumnInfo const &line_column_info) {
  return rd::wrapper::make_wrapper<model::RdcDebugStack>(step_index, line_column_info.fileIndex, line_column_info.lineStart, line_column_info.lineEnd, line_column_info.colStart, line_column_info.colEnd);
}

void RenderDocDebugSession::step_over() const {
  const auto stack_size = data->current_callstack_size;
  while (data->do_source_step()) {
    if (data->current_state->callstack.size() <= stack_size) {
      get_currentStack().set(make_debug_stack(data->current_state->stepIndex, data->current_instruction.lineInfo));
      return;
    }
  }
  get_currentStack().set(rd::Wrapper<model::RdcDebugStack>(nullptr));
}

void RenderDocDebugSession::resume() const {
  const auto& breakpoints = data->breakpoints;
  if (!data->breakpoints.empty()) {
    const auto& end = breakpoints.end();
    while (data->do_source_step()) {
      auto const& line_info = data->current_instruction.lineInfo;
      if (breakpoints.find(RenderDocBreakpoint(line_info)) != end) {
        get_currentStack().set(make_debug_stack(data->current_state->stepIndex, line_info));
        return;
      }
    }
  }

  get_currentStack().set(rd::Wrapper<model::RdcDebugStack>(nullptr));
}

rd::Wrapper<model::RdcStageVariableInfo> RenderDocDebugSession::get_stage_variable_info() const {
  auto sourceVars = ArrayUtils::CopyToVector(data->current_instruction.sourceVars, RenderDocConverterUtils::convertSourceMapping);
  auto changes = ArrayUtils::CopyToVector(data->current_state->changes, RenderDocConverterUtils::convertVariableChange);
  return model::RdcStageVariableInfo(sourceVars, changes);
}

void RenderDocDebugSession::add_breakpoint(uint32_t source_file_index, uint32_t line) const {
  data->breakpoints.emplace(source_file_index, line);
}

void RenderDocDebugSession::remove_breakpoint(uint32_t source_file_index, uint32_t line) const {
  const auto& it = data->breakpoints.find(RenderDocBreakpoint(source_file_index, line));
  if (it != data->breakpoints.end())
    data->breakpoints.erase(it);
}

} // namespace jetbrains::renderdoc
