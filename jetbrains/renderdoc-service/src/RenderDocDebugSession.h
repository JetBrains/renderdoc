#ifndef RENDERDOCDEBUGSESSION_H
#define RENDERDOCDEBUGSESSION_H
#include "RenderDocModel/RdcDebugSession.Generated.h"
#include "RenderDocModel/RdcDebugStack.Generated.h"

#include <memory>

struct IReplayController;
class ShaderDebugTrace;
struct ShaderDebugInfo;
struct LineColumnInfo;

namespace jetbrains::renderdoc
{

struct RenderDocDebugSessionData;

class RenderDocDebugSession : public model::RdcDebugSession  {
  std::shared_ptr<RenderDocDebugSessionData> data;
  static rd::Wrapper<model::RdcDebugStack> make_debug_stack(const uint32_t step_index, LineColumnInfo const &line_column_info);
  static std::vector<rd::Wrapper<model::RdcSourceFile>> get_source_files(const ShaderDebugInfo *debug_info);
  static std::vector<rd::Wrapper<model::RdcResourceInfo>> get_resource(const std::shared_ptr<IReplayController> &controller);

public:
  RenderDocDebugSession(const rd::Lifetime &session_lifetime, const std::shared_ptr<IReplayController> &controller, ShaderDebugTrace *trace, const ShaderDebugInfo *debug_info);
  void step_into() const;
  void step_over() const;
  void resume() const;
  void add_breakpoint(uint32_t source_file_index, uint32_t line) const;
  void remove_breakpoint(uint32_t source_file_index, uint32_t line) const;
  rd::Wrapper<model::RdcStageVariableInfo> get_stage_variable_info() const;
  std::wstring get_resource_name(const model::RdcResourceInfo& info) const;
};

}


#endif //RENDERDOCDEBUGSESSION_H
