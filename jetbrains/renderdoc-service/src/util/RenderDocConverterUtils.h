#ifndef RENDERDOCUTILS_H
#define RENDERDOCUTILS_H

#include "RenderDocModel/RdcDebugTrace.Generated.h"
#include "RenderDocModel/RdcShaderReflection.Generated.h"
#include "RenderDocModel/RdcUsedDescriptor.Generated.h"
#include "RenderDocModel/RenderDocModel.Generated.h"

#include <api/replay/renderdoc_replay.h>
#include <set>

namespace jetbrains::renderdoc {

struct RenderDocConverterUtils {
  [[nodiscard]] static std::vector<rd::Wrapper<std::wstring>> wrapStringsSet(const std::set<std::wstring> &strings);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderVariable> convertShaderVariable(const ShaderVariable &var);
  [[nodiscard]] static rd::Wrapper<model::RdcDebugVariableReference> convertVariableReference(const DebugVariableReference &ref);
  [[nodiscard]] static rd::Wrapper<model::RdcSourceVariableMapping> convertSourceMapping(const SourceVariableMapping &mapping);
  [[nodiscard]] static rd::Wrapper<model::RdcDebugTrace> convertDebugTrace(const ShaderDebugTrace *trace);
  [[nodiscard]] static std::vector<rd::Wrapper<model::RdcUsedDescriptor>> convertResources(const rdcarray<UsedDescriptor> &resources);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderReflection> convertShaderReflection(const ShaderReflection *shader);
private:
  [[nodiscard]] static rd::Wrapper<model::RdcUsedDescriptor> convertDescriptor(const UsedDescriptor &r);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderConstantType> convertShaderConstantType(const ShaderConstantType &t);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderResource> convertResource(const ShaderResource &r);
  [[nodiscard]] static rd::Wrapper<model::RdcConstantBlock> convertConstantBlock(const ConstantBlock &b);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderSampler> convertSampler(const ShaderSampler &s);
  [[nodiscard]] static rd::Wrapper<model::RdcShaderConstant> convertShaderConstant(const ShaderConstant &c);
};
} // namespace jetbrains::renderdoc

#endif // RENDERDOCUTILS_H
