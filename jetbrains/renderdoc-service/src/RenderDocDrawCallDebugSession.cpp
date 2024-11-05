#include "RenderDocDrawCallDebugSession.h"

#include "RenderDocModel/RdcDebugStack.Generated.h"
#include "types/wrapper.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocConverterUtils.h"
#include "util/RenderDocLineBreakpointsMapper.h"
#include "util/StringUtils.h"
#include <api/replay/renderdoc_replay.h>
#include <stack>

namespace jetbrains::renderdoc {

std::size_t RdcSourceBreakpointHash::operator()(const model::RdcSourceBreakpoint &bp) const noexcept {
  return bp.hashCode();
}

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

struct RenderDocDrawCallDebugSessionData {
  const ActionDescription *action;
  ShaderDebugTrace *trace;
  std::shared_ptr<IReplayController> controller;
  rdcarray<ShaderDebugState> states;
  const ShaderDebugInfo *debug_info;
  ShaderDebugState *current_state = states.begin();
  InstructionSourceInfo current_instruction;
  std::stack<std::pair<size_t, InstructionSourceInfo>> calltrace;
  std::unordered_set<RenderDocBreakpoint, RenderDocBreakpoint::hash> breakpoints;
  std::unordered_map<model::RdcSourceBreakpoint, std::vector<model::RdcLineBreakpoint>, RdcSourceBreakpointHash> breakpoints_mapping;
  size_t current_callstack_size;

  RenderDocDrawCallDebugSessionData(const ActionDescription *action, ShaderDebugTrace *trace, const std::shared_ptr<IReplayController> &controller, const ShaderDebugInfo *debug_info)
      : action(action), trace(trace), controller(controller), debug_info(debug_info), breakpoints_mapping({}), current_callstack_size(0) {
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

  ~RenderDocDrawCallDebugSessionData() { controller->FreeTrace(trace); }
};

std::vector<rd::Wrapper<model::RdcSourceFile>> RenderDocDrawCallDebugSession::get_source_files(const ShaderDebugInfo *debug_info) {
  std::vector<rd::Wrapper<model::RdcSourceFile>> source_files;
  const auto source_files_count = debug_info->files.count();
  for (auto index = 0; index < source_files_count; ++index) {
    const auto &[filename, contents] = debug_info->files[index];
    source_files.emplace_back(model::RdcSourceFile(StringUtils::Utf8ToWide(filename), StringUtils::Utf8ToWide(contents)));
  }
  return source_files;
}

std::vector<rd::Wrapper<model::RdcResourceInfo>> RenderDocDrawCallDebugSession::get_resource(const std::shared_ptr<IReplayController> &controller) {
  auto resources = controller->GetResources();
  std::vector<rd::Wrapper<model::RdcResourceInfo>> result(resources.size());
  size_t i = 0;
  for (const auto &r: resources) {
    result[i++] = model::RdcResourceInfo(*reinterpret_cast<const uint64_t *>(&r.resourceId), StringUtils::Utf8ToWide(r.name));
  }
  return result;
}

RenderDocDrawCallDebugSession::RenderDocDrawCallDebugSession(const ActionDescription* action, const std::shared_ptr<IReplayController> &controller, ShaderDebugTrace *trace, const ShaderDebugInfo *debug_info, const ShaderReflection *reflection)
    :  RdcDrawCallDebugSession(RenderDocConverterUtils::convertDebugTrace(*trace), get_source_files(debug_info), get_resource(controller), RenderDocConverterUtils::convertResources(controller->GetPipelineState().GetReadOnlyResources(trace->stage)),
      RenderDocConverterUtils::convertResources(controller->GetPipelineState().GetReadWriteResources(trace->stage)), RenderDocConverterUtils::convertResources(controller->GetPipelineState().GetSamplers(trace->stage)),
      RenderDocConverterUtils::convertShaderReflection(reflection)), data(std::make_shared<RenderDocDrawCallDebugSessionData>(action, trace, controller, debug_info)) {
}

rd::Wrapper<model::RdcDebugStack> RenderDocDrawCallDebugSession::step_into() const {
  if (!data->do_source_step())
    return rd::Wrapper<model::RdcDebugStack>(nullptr);

  return make_debug_stack(data->current_state->stepIndex, data->current_instruction.lineInfo);
}

rd::Wrapper<model::RdcDebugStack> RenderDocDrawCallDebugSession::make_debug_stack(const uint32_t step_index, LineColumnInfo const &line_column_info) const {
  return rd::wrapper::make_wrapper<model::RdcDebugStack>(get_action()->eventId, step_index, line_column_info.fileIndex, line_column_info.lineStart, line_column_info.lineEnd, line_column_info.colStart, line_column_info.colEnd);
}

rd::Wrapper<model::RdcDebugStack> RenderDocDrawCallDebugSession::step_over() const {
  const auto stack_size = data->current_callstack_size;
  while (data->do_source_step()) {
    if (data->current_state->callstack.size() <= stack_size) {
      return make_debug_stack(data->current_state->stepIndex, data->current_instruction.lineInfo);
    }
  }
  return rd::Wrapper<model::RdcDebugStack>(nullptr);
}

rd::Wrapper<model::RdcDebugStack> RenderDocDrawCallDebugSession::resume() const {
  const auto& breakpoints = data->breakpoints;
  if (!data->breakpoints.empty()) {
    const auto& end = breakpoints.end();
    while (data->do_source_step()) {
      auto const& line_info = data->current_instruction.lineInfo;
      if (breakpoints.find(RenderDocBreakpoint(line_info)) != end) {
        return make_debug_stack(data->current_state->stepIndex, line_info);
      }
    }
  }

  return rd::Wrapper<model::RdcDebugStack>(nullptr);
}

std::vector<rd::Wrapper<model::RdcSourceVariableMapping>> RenderDocDrawCallDebugSession::get_source_variables() const {
  return ArrayUtils::CopyToVector(data->current_instruction.sourceVars, RenderDocConverterUtils::convertSourceMapping);
}


std::vector<rd::Wrapper<model::RdcShaderVariableChange>> RenderDocDrawCallDebugSession::get_variable_changes() const {
  return ArrayUtils::CopyToVector(data->current_state->changes, RenderDocConverterUtils::convertVariableChange);
}

const ActionDescription *RenderDocDrawCallDebugSession::get_action() const { return data->action; }

void RenderDocDrawCallDebugSession::add_breakpoint(uint32_t source_file_index, uint32_t line) const {
  data->breakpoints.emplace(source_file_index, line);
}

void RenderDocDrawCallDebugSession::remove_breakpoint(uint32_t source_file_index, uint32_t line) const {
  const auto& it = data->breakpoints.find(RenderDocBreakpoint(source_file_index, line));
  if (it != data->breakpoints.end())
    data->breakpoints.erase(it);
}

std::vector<model::RdcLineBreakpoint> RenderDocDrawCallDebugSession::map_breakpoints_from_sources(const RenderDocLineBreakpointsMapper *mapper, const std::unordered_set<model::RdcSourceBreakpoint, RdcSourceBreakpointHash> &breakpoints) const {
  std::vector<model::RdcLineBreakpoint> res;
  if (sourceFiles_.empty())
    return res;

  for (const auto &breakpoint : breakpoints) {
    if (const auto &it = data->breakpoints_mapping.find(breakpoint); it != data->breakpoints_mapping.end()) {
      for (const auto &bp: it->second) {
        res.push_back(bp);
      }
    } else {
      for (const auto &[idx, line] : mapper->map_source_line(data->action->eventId, breakpoint.get_sourceFilePath(), breakpoint.get_line())) {
        data->breakpoints_mapping[breakpoint].emplace_back(idx, line);
        res.emplace_back(idx, line);
      }
    }
  }

  return res;
}

} // namespace jetbrains::renderdoc
