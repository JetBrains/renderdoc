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
  std::unordered_map<uint32_t, Dimensions> thumbnails_dimension_cache;
  std::unordered_map<uint32_t, std::vector<std::size_t>> outputs_indices_cache;

  void calculate_dimensions(uint32_t event_id, const rdcarray<Descriptor> &outputs);
  int32_t get_width(uint32_t event_id) const;
  int32_t get_height(uint32_t event_id) const;

public:
  explicit RenderDocTexturePreviewService(IReplayController *controller);
  std::vector<rd::Wrapper<model::RdcWindowOutputData>> get_buffers(uint32_t event_id, const rdcarray<Descriptor> &outputs);
};
}

#endif //RENDERDOCTEXTUREPREVIEWSERVICE_H
