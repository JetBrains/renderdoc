#include "RenderDocTexturePreviewService.h"

#include "RenderDocModel/RdcTextureOutputs.Generated.h"
#include "RenderDocModel/RdcWindowOutputData.Generated.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocActionHelpers.h"

namespace jetbrains::renderdoc {

RenderDocTexturePreviewService::Dimensions::Dimensions(int32_t width, int32_t height) : width(width), height(height) {}

RenderDocTexturePreviewService::RenderDocTexturePreviewService(IReplayController *controller) : controller(controller) {
}

Descriptor RenderDocTexturePreviewService::create_descriptor(ResourceId id) {
  Descriptor descriptor;
  descriptor.type = DescriptorType::ReadWriteImage;
  descriptor.resource = id;
  return descriptor;
}

void RenderDocTexturePreviewService::calculate_dimensions(const ActionDescription *action, uint32_t event_id) {
  auto outputs = get_output_targets(action);
  outputs.push_back(get_depth_target(action));

  const auto textures = controller->GetTextures();
  int32_t width = 0;
  int32_t height = 0;
  std::vector<Dimensions> dimensions;
  for (auto target : outputs) {
    const auto texture = std::find_if(textures.begin(), textures.end(), [id = target.resource](const TextureDescription &t) { return t.resourceId == id; });
    if (texture == textures.end() || !(texture->creationFlags & (TextureCategory::ColorTarget | TextureCategory::DepthTarget)))
      continue;
    width = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->width, INT32_MAX)), width);
    height = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->height, INT32_MAX)), height);
    if (texture->creationFlags & TextureCategory::ColorTarget)
      color_outputs_cache[event_id].emplace_back(target, Dimensions(width, height));
    else
      depth_outputs_cache.try_emplace(event_id, target, Dimensions(width, height));
  }
  max_dimensions.try_emplace(event_id, width, height);
}

void RenderDocTexturePreviewService::calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const {
  copy = action && action->flags & (ActionFlags::Copy | ActionFlags::Resolve | ActionFlags::Present);
  clear = action && action->flags & ActionFlags::Clear;
  compute = action && action->flags & ActionFlags::Dispatch && controller->GetPipelineState().GetShader(ShaderStage::Compute) != ResourceId();
}

rdcarray<Descriptor> RenderDocTexturePreviewService::get_output_targets(const ActionDescription *action) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);
  if(action && (copy || clear))
    return {create_descriptor(action->copyDestination)};

  if(compute)
    return {};

  const rdcarray<Descriptor> outputs = controller->GetPipelineState().GetOutputTargets();

  if(action && outputs.isEmpty() && action->flags & ActionFlags::Present) {
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

rd::Wrapper<model::RdcTextureOutputs> RenderDocTexturePreviewService::get_outputs(const ActionDescription *action, uint32_t event_id) {
  if (max_dimensions.find(event_id) == max_dimensions.end()) {
    calculate_dimensions(action, event_id);
  }
  if (max_dimensions[event_id].width == 0 && max_dimensions[event_id].height == 0)
    return rd::Wrapper<model::RdcTextureOutputs>(nullptr);

  const auto max_width = max_dimensions.at(event_id).width;
  const auto max_height = max_dimensions.at(event_id).height;

  const auto window_data = CreateHeadlessWindowingData(max_width, max_height);
  const auto &output = controller->CreateOutput(window_data, ReplayOutputType::Texture);

  std::vector<rd::Wrapper<model::RdcWindowOutputData>> color_outs;
  if (const auto &outputs_it = color_outputs_cache.find(event_id); outputs_it != color_outputs_cache.end()) {
    color_outs.reserve(color_outputs_cache[event_id].size());

    for (const auto &[desc, dim] : color_outputs_cache[event_id]) {
      const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
      color_outs.emplace_back(rd::wrapper::make_wrapper<model::RdcWindowOutputData>(dim.width, dim.height, buffer));
    }
  }

  rd::Wrapper<model::RdcWindowOutputData> depth_out(nullptr);
  if (const auto &depth_it = depth_outputs_cache.find(event_id); depth_it != depth_outputs_cache.end()) {
    const auto &[desc, dim] = depth_it->second;
    const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
    depth_out = rd::wrapper::make_wrapper<model::RdcWindowOutputData>(dim.width, dim.height, buffer);
  }

  output->Shutdown();
  return rd::wrapper::make_wrapper<model::RdcTextureOutputs>(color_outs, depth_out);
}
}
