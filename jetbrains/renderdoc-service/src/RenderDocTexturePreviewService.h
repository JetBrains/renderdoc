#ifndef RENDERDOCTEXTUREPREVIEWSERVICE_H
#define RENDERDOCTEXTUREPREVIEWSERVICE_H

#include "types/wrapper.h"

#include <api/replay/renderdoc_replay.h>
#include <api/replay/resourceid.h>
#include <unordered_map>

namespace jetbrains::renderdoc {
namespace model {
class RdcTextureOutputs;
class RdcWindowOutputData;
}

class RenderDocTexturePreviewService {
  struct Dimensions {
    int32_t width;
    int32_t height;

    Dimensions() = default;
    Dimensions(int32_t width, int32_t height);
  };

  IReplayController *controller;
  std::unordered_map<uint32_t, std::vector<std::pair<Descriptor, Dimensions>>> color_outputs_cache;
  std::unordered_map<uint32_t, std::pair<Descriptor, Dimensions>> depth_outputs_cache;
  std::unordered_map<uint32_t, Dimensions> max_dimensions;

  static Descriptor create_descriptor(ResourceId id);

  void calculate_dimensions(const ActionDescription *action, uint32_t event_id);
  void calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const;
  rdcarray<Descriptor> get_output_targets(const ActionDescription *action) const;
  Descriptor get_depth_target(const ActionDescription *action) const;

public:
  explicit RenderDocTexturePreviewService(IReplayController *controller);
  rd::Wrapper<model::RdcTextureOutputs> get_outputs(const ActionDescription *action, uint32_t event_id);
};
}

#endif //RENDERDOCTEXTUREPREVIEWSERVICE_H
