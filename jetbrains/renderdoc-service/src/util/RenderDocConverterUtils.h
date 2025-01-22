#ifndef RENDERDOCUTILS_H
#define RENDERDOCUTILS_H

#include "RenderDocModel/RdcDebugTrace.Generated.h"
#include "RenderDocModel/RdcShaderReflection.Generated.h"
#include "RenderDocModel/RdcUsedDescriptor.Generated.h"
#include "RenderDocModel/RenderDocModel.Generated.h"

#include <api/replay/renderdoc_replay.h>

namespace jetbrains::renderdoc {

struct RenderDocConverterUtils {
  static rd::Wrapper<model::RdcShaderVariable> convertShaderVariable(const ShaderVariable &var);
  static rd::Wrapper<model::RdcDebugVariableReference> convertVariableReference(const DebugVariableReference &ref);
  static rd::Wrapper<model::RdcSourceVariableMapping> convertSourceMapping(const SourceVariableMapping &mapping);
  static rd::Wrapper<model::RdcDebugTrace> convertDebugTrace(const ShaderDebugTrace &trace);
  static std::vector<rd::Wrapper<model::RdcUsedDescriptor>> convertResources(const rdcarray<UsedDescriptor> &resources);
  static rd::Wrapper<model::RdcShaderReflection> convertShaderReflection(const ShaderReflection *shader);
private:
  static rd::Wrapper<model::RdcUsedDescriptor> convertDescriptor(const UsedDescriptor &r);
  static rd::Wrapper<model::RdcShaderConstantType> convertShaderConstantType(const ShaderConstantType &t);
  static rd::Wrapper<model::RdcShaderResource> convertResource(const ShaderResource &r);
  static rd::Wrapper<model::RdcConstantBlock> convertConstantBlock(const ConstantBlock &b);
  static rd::Wrapper<model::RdcShaderSampler> convertSampler(const ShaderSampler &s);
  static rd::Wrapper<model::RdcShaderConstant> convertShaderConstant(const ShaderConstant &c);
};
} // namespace jetbrains::renderdoc

#endif // RENDERDOCUTILS_H