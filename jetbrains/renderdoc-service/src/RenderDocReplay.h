#ifndef RENDERDOCCAPTURE_H
#define RENDERDOCCAPTURE_H

#include "RenderDocDebugSession.h"
#include "RenderDocModel/RdcCapture.Generated.h"

enum class ShaderStage : uint8_t;
struct IReplayController;

namespace jetbrains::renderdoc {
class RenderDocMeshPreviewService;
class RenderDocTexturePreviewService;
class RenderDocLineBreakpointsMapper;

class RenderDocReplay : public model::RdcCapture {
  std::unordered_map<int64_t, uint32_t> effective_event_ids;
  void calculate_effective_event_ids(const ActionDescription *parent);
  uint32_t get_effective_event_id(int64_t event_id) const;
public:
  std::shared_ptr<IReplayController> controller;
  std::shared_ptr<RenderDocLineBreakpointsMapper> mapper;
  std::shared_ptr<RenderDocTexturePreviewService> texture_previewer;
  std::shared_ptr<RenderDocMeshPreviewService> mesh_previewer;

  explicit RenderDocReplay(IReplayController *controller);

  [[nodiscard]] rd::Wrapper<model::RdcTextureOutputs> get_textureRGBBuffer(const rd::Lifetime &session_lifetime, int64_t event_id) const;
  [[nodiscard]] rd::Wrapper<model::RdcVertexStageInOutputs> get_vertices_inoutputs(const rd::Lifetime &session_lifetime, int64_t event_id) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> debug_vertex(const rd::Lifetime &session_lifetime, const model::RdcDebugVertexInput &input) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> debug_pixel(const rd::Lifetime &session_lifetime, const model::RdcDebugPixelInput &input) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> try_debug_vertex(const rd::Lifetime &session_lifetime, const model::RdcDebugVertexInput &input) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> try_debug_pixel(const rd::Lifetime &session_lifetime, const model::RdcDebugPixelInput &input) const;

  [[nodiscard]] rd::Wrapper<RenderDocDrawCallDebugSession> start_debug_vertex(const ActionDescription *action, DebugInput input) const;
  [[nodiscard]] rd::Wrapper<RenderDocDrawCallDebugSession> start_debug_pixel(const ActionDescription *action, DebugInput input) const;
};

} // namespace jetbrains::renderdoc

#endif // RENDERDOCCAPTURE_H
