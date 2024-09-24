#include "RenderDocConverterUtils.h"

#include "ArrayUtils.h"
#include "StringUtils.h"

namespace jetbrains::renderdoc {
rd::Wrapper<model::RdcShaderVariable> RenderDocConverterUtils::convertShaderVariable(const ShaderVariable &var) {
  model::RdcShaderValue value(ArrayUtils::CopyToVector(var.value.f32v),
    ArrayUtils::CopyToVector(var.value.s32v), ArrayUtils::CopyToVector(var.value.u32v),
    ArrayUtils::CopyToVector(var.value.f64v), ArrayUtils::CopyToVector<rdhalf, uint16_t>(var.value.f16v),
    ArrayUtils::CopyToVector(var.value.u64v), ArrayUtils::CopyToVector(var.value.s64v),
    ArrayUtils::CopyToVector(var.value.u16v), ArrayUtils::CopyToVector(var.value.s16v),
    ArrayUtils::CopyToVector(var.value.u8v), ArrayUtils::CopyToVector<signed char, uint8_t>(var.value.s8v));
  return model::RdcShaderVariable(StringUtils::Utf8ToWide(var.name), var.rows, var.columns, static_cast<model::RdcVarType>(var.type), static_cast<model::RdcVariableFlags>(var.flags), value, ArrayUtils::CopyToVector(var.members, convertShaderVariable));
}

rd::Wrapper<model::RdcDebugVariableReference> RenderDocConverterUtils::convertVariableReference(const DebugVariableReference &ref) {
  return model::RdcDebugVariableReference(StringUtils::Utf8ToWide(ref.name), static_cast<model::RdcDebugVariableType>(ref.type), ref.component);
}

rd::Wrapper<model::RdcSourceVariableMapping> RenderDocConverterUtils::convertSourceMapping(const SourceVariableMapping &mapping) {
  return model::RdcSourceVariableMapping(StringUtils::Utf8ToWide(mapping.name), static_cast<model::RdcVarType>(mapping.type),
      mapping.rows, mapping.columns, mapping.offset, mapping.signatureIndex, ArrayUtils::CopyToVector(mapping.variables, convertVariableReference));
}

rd::Wrapper<model::RdcDebugTrace> RenderDocConverterUtils::convertDebugTrace(const ShaderDebugTrace &trace) {
  return model::RdcDebugTrace(static_cast<model::RdcShaderStage>(trace.stage), ArrayUtils::CopyToVector(trace.inputs, convertShaderVariable),
    ArrayUtils::CopyToVector(trace.constantBlocks, convertShaderVariable), ArrayUtils::CopyToVector(trace.readOnlyResources, convertShaderVariable),
    ArrayUtils::CopyToVector(trace.readWriteResources, convertShaderVariable), ArrayUtils::CopyToVector(trace.samplers, convertShaderVariable), ArrayUtils::CopyToVector(trace.sourceVars, convertSourceMapping));
}

rd::Wrapper<model::RdcShaderVariableChange> RenderDocConverterUtils::convertVariableChange(const ShaderVariableChange &change) {
  return model::RdcShaderVariableChange(convertShaderVariable(change.before), convertShaderVariable(change.after));
}
}