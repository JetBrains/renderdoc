#ifndef RENDERDOCCAPTURECONTEXT_H
#define RENDERDOCCAPTURECONTEXT_H

#include <api/replay/renderdoc_replay.h>
#include <api/replay/resourceid.h>
#include <string>
#include <map>

namespace jetbrains::renderdoc {
class RenderDocCaptureContext {
  std::map<ResourceId, BufferDescription> buffers;
  std::map<ResourceId, ResourceDescription> resources;

public:
  explicit RenderDocCaptureContext(IReplayController *controller);

  [[nodiscard]] const BufferDescription* try_get_buffer(ResourceId id) const;
  [[nodiscard]] const ResourceDescription* try_get_resource(ResourceId id) const;
  [[nodiscard]] std::wstring get_resource_name(ResourceId id) const;
  [[nodiscard]] bool has_auto_generated_name(ResourceId id) const;
};
}

#endif // RENDERDOCCAPTURECONTEXT_H