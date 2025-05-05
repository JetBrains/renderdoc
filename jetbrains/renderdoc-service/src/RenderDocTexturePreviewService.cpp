#include "RenderDocTexturePreviewService.h"

#include "RenderDocCaptureContext.h"
#include "RenderDocModel/RdcPixelStageInOutputs.Generated.h"
#include "RenderDocModel/RdcWindowOutputData.Generated.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocActionHelpers.h"

namespace jetbrains::renderdoc {

RenderDocTexturePreviewService::Dimensions::Dimensions(int32_t width, int32_t height) : width(width), height(height) {}

RenderDocTexturePreviewService::RenderDocTexturePreviewService(IReplayController *controller, const std::shared_ptr<RenderDocCaptureContext> &capture_context) :controller(controller), capture_context(capture_context) {
}

Descriptor RenderDocTexturePreviewService::create_descriptor(ResourceId id, Subresource sub = Subresource()) {
  Descriptor descriptor;
  descriptor.type = DescriptorType::ReadWriteImage;
  descriptor.resource = id;
  descriptor.firstMip = sub.mip;
  descriptor.firstSlice = sub.slice;
  return descriptor;
}

std::vector<std::pair<Descriptor, RenderDocTexturePreviewService::Dimensions>> RenderDocTexturePreviewService::calculate_dimensions(
    const rdcarray<TextureDescription> &textures,
    const rdcarray<Descriptor> &targets,
    const TextureCategory category,
    Dimensions &max_dim)
{
  auto &width = max_dim.width;
  auto &height = max_dim.height;

  std::vector<std::pair<Descriptor, Dimensions>> descriptors;
  for (auto target : targets) {
    const auto texture = std::find_if(textures.begin(), textures.end(), [id = target.resource](const TextureDescription &t) { return t.resourceId == id; });
    if (texture == textures.end() || !(texture->creationFlags & category))
      continue;
    const Dimensions dims(static_cast<int32_t>(texture->width), static_cast<int32_t>(texture->height));
    width = std::max(dims.width, width);
    height = std::max(dims.height, height);
    descriptors.emplace_back(target, dims);
  }
  return descriptors;
}

void RenderDocTexturePreviewService::calculate_dimensions(const ActionDescription *action, uint32_t event_id) {
  const auto textures = controller->GetTextures();
  Dimensions max_dim(0, 0);

  const auto input_targets = calculate_dimensions(textures, get_input_targets(action), TextureCategory::ShaderRead, max_dim);
  inputs_cache[event_id].insert(inputs_cache[event_id].end(), input_targets.begin(), input_targets.end());

  const auto color_output_targets = calculate_dimensions(textures, get_output_targets(action), TextureCategory::ColorTarget, max_dim);
  color_outputs_cache[event_id].insert(color_outputs_cache[event_id].end(), color_output_targets.begin(), color_output_targets.end());

  if (const auto depth_output_targets = calculate_dimensions(textures, {get_depth_target(action)}, TextureCategory::DepthTarget, max_dim); !depth_output_targets.empty())
    depth_outputs_cache.try_emplace(event_id, depth_output_targets[0]);

  max_dimensions[event_id] = max_dim;
}

void RenderDocTexturePreviewService::calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const {
  copy = action && action->flags & (ActionFlags::Copy | ActionFlags::Resolve | ActionFlags::Present);
  clear = action && action->flags & ActionFlags::Clear;
  compute = action && action->flags & ActionFlags::Dispatch && controller->GetPipelineState().GetShader(ShaderStage::Compute) != ResourceId();
}

void RenderDocTexturePreviewService::collect_read_only_resources(const ActionDescription *action, ShaderStage stage, rdcarray<Descriptor> &descriptors) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);
  if (action && clear)
    return;

  if (action && copy && stage == ShaderStage::Pixel) {
    descriptors.push_back(create_descriptor(action->copySource, action->copySourceSubresource));
    return;
  }

  if (compute && stage != ShaderStage::Pixel && stage != ShaderStage::Compute)
    return;


  const auto &inputs = controller->GetPipelineState().GetReadOnlyResources(compute ? ShaderStage::Compute : stage, true);
  for (const auto &input : inputs) {
    descriptors.emplace_back(input.descriptor);
  }
}

rdcarray<Descriptor> RenderDocTexturePreviewService::get_input_targets(const ActionDescription *action) const {
  static constexpr ShaderStage stages[6] = { ShaderStage::Vertex, ShaderStage::Hull, ShaderStage::Domain, ShaderStage::Geometry, ShaderStage::Pixel, ShaderStage::Compute };
  rdcarray<Descriptor> descriptors;
  for (const auto stage : stages) {
    collect_read_only_resources(action, stage, descriptors);
  }
  return descriptors;
}

rdcarray<Descriptor> RenderDocTexturePreviewService::get_output_targets(const ActionDescription *action) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);
  if (action && (copy || clear))
    return {create_descriptor(action->copyDestination)};

  if (compute)
    return {};

  const rdcarray<Descriptor> outputs = controller->GetPipelineState().GetOutputTargets();

  if (action && outputs.isEmpty() && action->flags & ActionFlags::Present) {
    if(action->copyDestination != ResourceId())
      return {create_descriptor(action->copyDestination)};

    for(const TextureDescription &tex : controller->GetTextures())
    {
      if(tex.creationFlags & TextureCategory::SwapBuffer)
        return {create_descriptor(tex.resourceId)};
    }
  }

  return outputs;
}

Descriptor RenderDocTexturePreviewService::get_depth_target(const ActionDescription *action) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);

  if(copy || clear || compute)
    return {};
  return controller->GetPipelineState().GetDepthTarget();
}

std::wstring RenderDocTexturePreviewService::get_texture_name(const ActionDescription *action, const Descriptor &desc) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);

  const std::wstring bind_name = (copy || clear) ? L"Destination" : L"";
  if (desc.resource == ResourceId::Null())
    return L"";

  std::wstringstream name_stream(bind_name);

  if (!capture_context->has_auto_generated_name(desc.resource)) {
    if (name_stream.tellp() != 0) {
      name_stream << " = ";
    }
    name_stream << capture_context->get_resource_name(desc.resource);
  }

  if (name_stream.tellp() == 0) {
    name_stream << capture_context->get_resource_name(desc.resource);
  }

  return name_stream.str();
}

rd::Wrapper<model::RdcPixelStageInOutputs> RenderDocTexturePreviewService::get_inoutputs(const ActionDescription *action, uint32_t event_id) {
  if (max_dimensions.find(event_id) == max_dimensions.end()) {
    calculate_dimensions(action, event_id);
  }
  if (max_dimensions[event_id].width == 0 && max_dimensions[event_id].height == 0)
    return rd::Wrapper<model::RdcPixelStageInOutputs>(nullptr);

  const auto max_width = max_dimensions.at(event_id).width;
  const auto max_height = max_dimensions.at(event_id).height;

  const auto window_data = CreateHeadlessWindowingData(max_width, max_height);
  const auto &output = controller->CreateOutput(window_data, ReplayOutputType::Texture);

  std::vector<rd::Wrapper<model::RdcWindowOutputData>> inputs;
  if (const auto &inputs_it = inputs_cache.find(event_id); inputs_it != inputs_cache.end()) {
    const auto &event_inputs = inputs_it->second;
    inputs.reserve(event_inputs.size());

    for (const auto &[desc, dim] : event_inputs) {
      const auto &name = get_texture_name(action, desc);
      const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
      inputs.emplace_back(rd::wrapper::make_wrapper<model::RdcWindowOutputData>(name, dim.width, dim.height, buffer));
    }
  }

  std::vector<rd::Wrapper<model::RdcWindowOutputData>> color_outs;
  if (const auto &outputs_it = color_outputs_cache.find(event_id); outputs_it != color_outputs_cache.end()) {
    const auto &event_outputs = outputs_it->second;
    color_outs.reserve(event_outputs.size());

    for (const auto &[desc, dim] : event_outputs) {
      const auto &name = get_texture_name(action, desc);
      const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
      color_outs.emplace_back(rd::wrapper::make_wrapper<model::RdcWindowOutputData>(name, dim.width, dim.height, buffer));
    }
  }

  rd::Wrapper<model::RdcWindowOutputData> depth_out(nullptr);
  if (const auto &depth_it = depth_outputs_cache.find(event_id); depth_it != depth_outputs_cache.end()) {
    const auto &[desc, dim] = depth_it->second;
    const auto &name = get_texture_name(action, desc);
    const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
    depth_out = rd::wrapper::make_wrapper<model::RdcWindowOutputData>(name, dim.width, dim.height, buffer);
  }

  output->Shutdown();
  return rd::wrapper::make_wrapper<model::RdcPixelStageInOutputs>(inputs, color_outs, depth_out);
}
}
