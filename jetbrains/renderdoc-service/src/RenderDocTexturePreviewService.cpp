#include "RenderDocTexturePreviewService.h"

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

void RenderDocTexturePreviewService::calculate_dimensions(const ActionDescription *action) {
  auto outputs = get_output_targets(action);
  outputs.push_back(get_depth_target(action));

  const auto textures = controller->GetTextures();
  constexpr auto flags = TextureCategory::ColorTarget | TextureCategory::DepthTarget;
  int32_t width = 0;
  int32_t height = 0;
  std::vector<Dimensions> dimensions;
  for (auto target : outputs) {
    const auto texture = std::find_if(textures.begin(), textures.end(), [id = target.resource](const TextureDescription &t) { return t.resourceId == id; });
    if (texture == textures.end() || !(texture->creationFlags & flags))
      continue;
    width = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->width, INT32_MAX)), width);
    height = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->height, INT32_MAX)), height);
    outputs_cache[action->eventId].emplace_back(target, Dimensions(width, height));
  }
  max_dimensions.try_emplace(action->eventId, width, height);
}

void RenderDocTexturePreviewService::calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const {
  copy = static_cast<bool>(action->flags & (ActionFlags::Copy | ActionFlags::Resolve | ActionFlags::Present));
  clear = static_cast<bool>(action->flags & ActionFlags::Clear);
  compute = static_cast<bool>(action->flags & ActionFlags::Dispatch) && controller->GetPipelineState().GetShader(ShaderStage::Compute) != ResourceId();
}

rdcarray<Descriptor> RenderDocTexturePreviewService::get_output_targets(const ActionDescription *action) const {
  bool copy, clear, compute;
  calculate_action_context(action, copy, clear, compute);
  if(copy || clear)
    return {create_descriptor(action->copyDestination)};

  if(compute)
    return {};

  const rdcarray<Descriptor> outputs = controller->GetPipelineState().GetOutputTargets();

  if(outputs.isEmpty() && action->flags & ActionFlags::Present) {
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
  else
    return controller->GetPipelineState().GetDepthTarget();
}

int32_t RenderDocTexturePreviewService::get_width(uint32_t event_id, std::size_t output_index) const {
  return outputs_cache.at(event_id).at(output_index).second.width;
}

int32_t RenderDocTexturePreviewService::get_height(uint32_t event_id, std::size_t output_index) const {
  return outputs_cache.at(event_id).at(output_index).second.height;
}

std::vector<rd::Wrapper<model::RdcWindowOutputData>> RenderDocTexturePreviewService::get_buffers(const ActionDescription *action) {

  const uint32_t event_id = action->eventId;
  if (outputs_cache.find(event_id) == outputs_cache.end()) {
    calculate_dimensions(action);
  }
  if (outputs_cache[event_id].empty())
    return {};

  const auto max_width = max_dimensions.at(event_id).width;
  const auto max_height = max_dimensions.at(event_id).height;

  const auto window_data = CreateHeadlessWindowingData(max_width, max_height);
  const auto &output = controller->CreateOutput(window_data, ReplayOutputType::Texture);

  std::vector<rd::Wrapper<model::RdcWindowOutputData>> result;
  result.reserve(outputs_cache[event_id].size());

  for (const auto &[desc, dim] : outputs_cache[event_id]) {
    const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, desc.resource, {}, CompType::Typeless));
    result.emplace_back(rd::wrapper::make_wrapper<model::RdcWindowOutputData>(dim.width, dim.height, buffer));
  }

  output->Shutdown();
  return result;
}
}
