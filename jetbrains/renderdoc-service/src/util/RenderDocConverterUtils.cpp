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

rd::Wrapper<model::RdcUsedDescriptor> RenderDocConverterUtils::convertDescriptor(const UsedDescriptor &r) {
  const rd::Wrapper<model::RdcDescriptorAccess> access = model::RdcDescriptorAccess(static_cast<model::RdcDescriptorType>(r.access.type), r.access.index, r.access.arrayElement);
  if (const auto resource = r.descriptor.resource; resource == ResourceId::Null())
    return {model::RdcUsedDescriptor(access, rd::nullopt) };
  return { model::RdcUsedDescriptor(access, *reinterpret_cast<const uint64_t*>(&r.descriptor.resource)) };
}

rd::Wrapper<model::RdcShaderConstant> RenderDocConverterUtils::convertShaderConstant(const ShaderConstant &c) {
  return { model::RdcShaderConstant(StringUtils::Utf8ToWide(c.name), c.byteOffset, c.bitFieldOffset, c.bitFieldSize, c.defaultValue, convertShaderConstantType(c.type), ArrayUtils::CopyToVector(c.type.members, convertShaderConstant)) };
}

rd::Wrapper<model::RdcShaderConstantType> RenderDocConverterUtils::convertShaderConstantType(const ShaderConstantType &t) {
  return { model::RdcShaderConstantType(StringUtils::Utf8ToWide(t.name), static_cast<model::RdcVariableFlags>(t.flags),
    t.pointerTypeID, t.elements, t.arrayByteStride, static_cast<model::RdcVarType>(t.baseType), t.rows, t.columns, t.matrixByteStride) };
}

rd::Wrapper<model::RdcShaderResource> RenderDocConverterUtils::convertResource(const ShaderResource &r) {
  return { model::RdcShaderResource(static_cast<model::RdcTextureType>(r.textureType), static_cast<model::RdcDescriptorType>(r.descriptorType), StringUtils::Utf8ToWide(r.name),
    convertShaderConstantType(r.variableType), ArrayUtils::CopyToVector(r.variableType.members, convertShaderConstant), r.fixedBindNumber, r.fixedBindSetOrSpace, r.bindArraySize,
    r.isTexture, r.hasSampler, r.isInputAttachment, r.isReadOnly) };
}

rd::Wrapper<model::RdcConstantBlock> RenderDocConverterUtils::convertConstantBlock(const ConstantBlock &b) {
  return { model::RdcConstantBlock(StringUtils::Utf8ToWide(b.name), ArrayUtils::CopyToVector(b.variables, convertShaderConstant), b.fixedBindNumber, b.fixedBindSetOrSpace, b.bindArraySize, b.byteSize, b.bufferBacked, b.inlineDataBytes, b.compileConstants) };
}

rd::Wrapper<model::RdcShaderSampler> RenderDocConverterUtils::convertSampler(const ShaderSampler &s) {
  return { model::RdcShaderSampler(StringUtils::Utf8ToWide(s.name), s.fixedBindNumber, s.fixedBindSetOrSpace, s.bindArraySize) };
}

std::vector<rd::Wrapper<model::RdcUsedDescriptor>> RenderDocConverterUtils::convertResources(const rdcarray<UsedDescriptor> &resources) {
  return ArrayUtils::CopyToVector(resources, convertDescriptor);
}

rd::Wrapper<model::RdcShaderReflection> RenderDocConverterUtils::convertShaderReflection(const ShaderReflection *shader) {
  if (shader == nullptr)
    throw std::invalid_argument("ShaderReflection should not be nullptr");
  return { model::RdcShaderReflection(
    ArrayUtils::CopyToVector(shader->constantBlocks, convertConstantBlock),
    ArrayUtils::CopyToVector(shader->samplers, convertSampler),
    ArrayUtils::CopyToVector(shader->readOnlyResources, convertResource),
    ArrayUtils::CopyToVector(shader->readWriteResources, convertResource)) };
}
}