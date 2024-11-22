#ifndef RENDERDOCDEBUGSESSION_H
#define RENDERDOCDEBUGSESSION_H
#include "RenderDocDrawCallDebugSession.h"
#include "RenderDocModel/RdcDebugSession.Generated.h"
#include "RenderDocModel/RdcSourceBreakpoint.Generated.h"

#include <memory>

enum class ShaderStage : uint8_t;
struct ShaderReflection;
struct IReplayController;
class ShaderDebugTrace;
struct ShaderDebugInfo;
struct LineColumnInfo;

namespace jetbrains::renderdoc
{
class RenderDocReplay;

class RenderDocDebugSessionData;

struct DebugInput {
  union {
    struct {
      uint32_t x;
      uint32_t y;
    } pixel;
    uint32_t vertex_id;
  };
};

class RenderDocDebugSession : public model::RdcDebugSession  {
  const DebugInput input;
  std::shared_ptr<RenderDocDebugSessionData> data;

public:
  RenderDocDebugSession(const rd::Lifetime& session_lifetime, const RenderDocReplay *replay, rd::Wrapper<RenderDocDrawCallDebugSession> draw_call_session, const ShaderStage &stage, DebugInput input, bool is_draw_call_debug);
  std::vector<rd::Wrapper<model::RdcSourceFile>> const & get_sourceFiles() const;
  void step_into() const;
  void step_over() const;
  void resume() const;
  void add_breakpoint(int32_t source_file_index, uint32_t line) const;
  void add_source_breakpoint(const rd::Wrapper<model::RdcSourceBreakpoint> &breakpoint) const;
  void remove_breakpoint(int32_t source_file_index, uint32_t line) const;
  void remove_source_breakpoint(const rd::Wrapper<model::RdcSourceBreakpoint> &breakpoint) const;
  void add_breakpoints_from_sources(const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const;

private:
  bool step_to_next_draw_call() const;
  void resume_to_next_not_null_stack(const std::function<rd::Wrapper<model::RdcDebugStack>()> &func) const;
  void step_to_next_not_null_stack(const std::function<rd::Wrapper<model::RdcDebugStack>()> &func, bool step_over= false) const;
};

}


#endif //RENDERDOCDEBUGSESSION_H
