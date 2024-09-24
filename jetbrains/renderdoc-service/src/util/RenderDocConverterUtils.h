#ifndef RENDERDOCUTILS_H
#define RENDERDOCUTILS_H

#include "RenderDocModel/RdcDebugTrace.Generated.h"
#include "RenderDocModel/RdcShaderVariableChange.Generated.h"

#include <api/replay/renderdoc_replay.h>

namespace jetbrains::renderdoc {
struct RenderDocConverterUtils {
  static rd::Wrapper<model::RdcShaderVariable> convertShaderVariable(const ShaderVariable &var);
  static rd::Wrapper<model::RdcDebugVariableReference> convertVariableReference(const DebugVariableReference &ref);
  static rd::Wrapper<model::RdcSourceVariableMapping> convertSourceMapping(const SourceVariableMapping &mapping);
  static rd::Wrapper<model::RdcDebugTrace> convertDebugTrace(const ShaderDebugTrace &trace);
  static rd::Wrapper<model::RdcShaderVariableChange> convertVariableChange(const ShaderVariableChange &change);
};
} // namespace jetbrains::renderdoc

#endif // RENDERDOCUTILS_H