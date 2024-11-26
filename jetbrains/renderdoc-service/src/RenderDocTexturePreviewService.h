#ifndef RENDERDOCTEXTUREPREVIEWSERVICE_H
#define RENDERDOCTEXTUREPREVIEWSERVICE_H

#include "types/wrapper.h"

#include <api/replay/renderdoc_replay.h>
#include <api/replay/resourceid.h>
#include <unordered_map>

namespace jetbrains::renderdoc {
namespace model {
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
  std::unordered_map<uint32_t, std::vector<std::pair<Descriptor, Dimensions>>> outputs_cache;
  std::unordered_map<uint32_t, Dimensions> max_dimensions;

  static Descriptor create_descriptor(ResourceId id);

  void calculate_dimensions(const ActionDescription *action);
  void calculate_action_context(const ActionDescription *action, bool &copy, bool &clear, bool &compute) const;
  rdcarray<Descriptor> get_output_targets(const ActionDescription *action) const;
  Descriptor get_depth_target(const ActionDescription *action) const;
  int32_t get_width(uint32_t event_id, std::size_t output_index) const;
  int32_t get_height(uint32_t event_id, std::size_t output_index) const;

public:
  explicit RenderDocTexturePreviewService(IReplayController *controller);
  std::vector<rd::Wrapper<model::RdcWindowOutputData>> get_buffers(const ActionDescription *action);
};
}

#endif //RENDERDOCTEXTUREPREVIEWSERVICE_H
