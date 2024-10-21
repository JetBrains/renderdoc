#ifndef RENDERDOCDRAWCALLDEBUGSESSION_H
#define RENDERDOCDRAWCALLDEBUGSESSION_H
#include "RenderDocModel/RdcDebugSession.Generated.h"
#include "RenderDocModel/RdcDebugStack.Generated.h"
#include "RenderDocModel/RdcDrawCallDebugSession.Generated.h"

#include <memory>

class ActionDescription;
struct ShaderReflection;
struct IReplayController;
class ShaderDebugTrace;
struct ShaderDebugInfo;
struct LineColumnInfo;

namespace jetbrains::renderdoc
{
struct RenderDocBreakpoint;

namespace model {
class RdcSourceBreakpoint;
}

struct RenderDocDrawCallDebugSessionData;

class RenderDocDrawCallDebugSession : public model::RdcDrawCallDebugSession  {
  static const std::wregex FILE_INFO_REGEX;
  std::shared_ptr<RenderDocDrawCallDebugSessionData> data;
  static rd::Wrapper<model::RdcDebugStack> make_debug_stack(uint32_t step_index, LineColumnInfo const &line_column_info);
  static std::vector<rd::Wrapper<model::RdcSourceFile>> get_source_files(const ShaderDebugInfo *debug_info);
  static std::vector<rd::Wrapper<model::RdcResourceInfo>> get_resource(const std::shared_ptr<IReplayController> &controller);

public:
  RenderDocDrawCallDebugSession(const ActionDescription *action, const std::shared_ptr<IReplayController> &controller, ShaderDebugTrace *trace, const ShaderDebugInfo *debug_info, const ShaderReflection *reflection);
  rd::Wrapper<model::RdcDebugStack> step_into() const;
  rd::Wrapper<model::RdcDebugStack> step_over() const;
  rd::Wrapper<model::RdcDebugStack> resume() const;
  void add_breakpoint(uint32_t source_file_index, uint32_t line) const;
  void remove_breakpoint(uint32_t source_file_index, uint32_t line) const;
  std::vector<rd::Wrapper<model::RdcSourceVariableMapping>> get_source_variables() const;
  std::vector<rd::Wrapper<model::RdcShaderVariableChange>> get_variable_changes() const;
  const ActionDescription *get_action() const;
  void map_and_add_breakpoints_from_sources(const std::unordered_map<std::wstring, std::vector<model::RdcSourceBreakpoint>> &breakpoints) const;
};

}


#endif //RENDERDOCDRAWCALLDEBUGSESSION_H
