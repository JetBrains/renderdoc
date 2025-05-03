#ifndef RENDERDOCTEXTUREPREVIEWSERVICE_H
#define RENDERDOCTEXTUREPREVIEWSERVICE_H

#include "types/wrapper.h"

#include <api/replay/renderdoc_replay.h>
#include <api/replay/resourceid.h>
#include <unordered_map>

namespace jetbrains::renderdoc {
namespace model {
class RdcPixelStageInOutputs;
class RdcWindowOutputData;
}
class RenderDocCaptureContext;

class RenderDocTexturePreviewService {
  struct Dimensions {
    int32_t width;
    int32_t height;

    Dimensions() = default;
    Dimensions(int32_t width, int32_t height);
  };

  IReplayController *controller;
  std::shared_ptr<RenderDocCaptureContext> capture_context;
  std::unordered_map<uint32_t, std::vector<std::pair<Descriptor, Dimensions>>> inputs_cache;
  std::unordered_map<uint32_t, std::vector<std::pair<Descriptor, Dimensions>>> color_outputs_cache;
  std::unordered_map<uint32_t, std::pair<Descriptor, Dimensions>> depth_outputs_cache;
  std::unordered_map<uint32_t, Dimensions> max_dimensions;

  static Descriptor create_descriptor(ResourceId id, Subresource sub);
  static std::vector<std::pair<Descriptor, Dimensions>> calculate_dimensions(const rdcarray<TextureDescription> &textures, const rdcarray<Descriptor> &targets, const TextureCategory category, Dimensions &max_dim);
  void calculate_dimensions(const ActionDescription *action, uint32_t event_id);
  void calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const;
  void collect_read_only_resources(const ActionDescription *action, ShaderStage stage, rdcarray<Descriptor> &descriptors) const;
  std::wstring get_texture_name(const ActionDescription *action, const Descriptor &desc) const;
  rdcarray<Descriptor> get_input_targets(const ActionDescription *action) const;
  rdcarray<Descriptor> get_output_targets(const ActionDescription *action) const;
  Descriptor get_depth_target(const ActionDescription *action) const;

public:
  explicit RenderDocTexturePreviewService(IReplayController *controller, const std::shared_ptr<RenderDocCaptureContext> &capture_context);
  rd::Wrapper<model::RdcPixelStageInOutputs> get_inoutputs(const ActionDescription *action, uint32_t event_id);
};
}

#endif //RENDERDOCTEXTUREPREVIEWSERVICE_H
