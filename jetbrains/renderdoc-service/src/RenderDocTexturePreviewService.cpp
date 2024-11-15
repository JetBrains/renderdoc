#include "RenderDocTexturePreviewService.h"

#include "RenderDocModel/RdcWindowOutputData.Generated.h"
#include "util/ArrayUtils.h"
#include "util/RenderDocActionHelpers.h"

namespace jetbrains::renderdoc {

RenderDocTexturePreviewService::Dimensions::Dimensions(int32_t width, int32_t height) : width(width), height(height) {}

RenderDocTexturePreviewService::RenderDocTexturePreviewService(IReplayController *controller) : controller(controller) {
}

void RenderDocTexturePreviewService::calculate_dimensions(uint32_t event_id, const rdcarray<Descriptor> &outputs) {
  const auto textures = controller->GetTextures();
  constexpr auto flags = TextureCategory::ColorTarget | TextureCategory::DepthTarget;
  int32_t width = 0;
  int32_t height = 0;
  std::vector<Dimensions> dimensions;
  for (std::size_t i = 0; i < outputs.size(); ++i) {
    const auto &target = outputs[i];
    const auto texture = std::find_if(textures.begin(), textures.end(), [id = target.resource](const TextureDescription &t) { return t.resourceId == id; });
    if (texture == textures.end() || !(texture->creationFlags & flags))
      continue;
    width = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->width, INT32_MAX)), width);
    height = std::max(static_cast<int32_t>(std::min<uint32_t>(texture->height, INT32_MAX)), height);
    outputs_dimensions_cache[event_id].try_emplace(i, width, height);
  }
  max_dimensions.try_emplace(event_id, width, height);
}

int32_t RenderDocTexturePreviewService::get_width(uint32_t event_id, std::size_t output_index) const {
  return outputs_dimensions_cache.at(event_id).at(output_index).width;
}

int32_t RenderDocTexturePreviewService::get_height(uint32_t event_id, std::size_t output_index) const {
  return outputs_dimensions_cache.at(event_id).at(output_index).height;
}

std::vector<rd::Wrapper<model::RdcWindowOutputData>> RenderDocTexturePreviewService::get_buffers(uint32_t event_id, const rdcarray<Descriptor> &outputs) {
  if (outputs_dimensions_cache.find(event_id) == outputs_dimensions_cache.end()) {
    calculate_dimensions(event_id, outputs);
  }
  if (outputs_dimensions_cache[event_id].empty())
    return {};

  const auto max_width = max_dimensions.at(event_id).width;
  const auto max_height = max_dimensions.at(event_id).height;

  const auto window_data = CreateHeadlessWindowingData(max_width, max_height);
  const auto &output = controller->CreateOutput(window_data, ReplayOutputType::Texture);

  std::vector<rd::Wrapper<model::RdcWindowOutputData>> result;
  result.reserve(outputs_dimensions_cache[event_id].size());

  for (const auto &[i, dim] : outputs_dimensions_cache[event_id]) {
    const std::vector<uint8_t> buffer = ArrayUtils::CopyToVector(output->DrawThumbnail(dim.width, dim.height, outputs[i].resource, {}, CompType::Typeless));
    result.emplace_back(rd::wrapper::make_wrapper<model::RdcWindowOutputData>(dim.width, dim.height, buffer));
  }

  output->Shutdown();
  return result;
}
}
