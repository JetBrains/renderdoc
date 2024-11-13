#ifndef RENDERDOCCAPTURE_H
#define RENDERDOCCAPTURE_H
#include "RenderDocDebugSession.h"
#include "RenderDocModel/RdcCapture.Generated.h"

enum class ShaderStage : uint8_t;
struct IReplayController;

namespace jetbrains::renderdoc {
class RenderDocTexturePreviewService;
class RenderDocLineBreakpointsMapper;

class RenderDocReplay : public model::RdcCapture {
public:
  std::shared_ptr<IReplayController> controller;
  std::shared_ptr<RenderDocLineBreakpointsMapper> mapper;
  std::shared_ptr<RenderDocTexturePreviewService> texture_previewer;

  explicit RenderDocReplay(IReplayController *controller);

  [[nodiscard]] std::vector<rd::Wrapper<model::RdcWindowOutputData>> get_textureRGBBuffer(const rd::Lifetime &session_lifetime, uint32_t event_id) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> debug_vertex(const rd::Lifetime &session_lifetime, uint32_t event_id) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> debug_pixel(const rd::Lifetime &session_lifetime, uint32_t event_id) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> try_debug_vertex(const rd::Lifetime &session_lifetime, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const;
  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> try_debug_pixel(const rd::Lifetime &session_lifetime, const std::vector<rd::Wrapper<model::RdcSourceBreakpoint>> &breakpoints) const;

  [[nodiscard]] rd::Wrapper<RenderDocDrawCallDebugSession> start_debug_vertex(const ActionDescription * action) const;
  [[nodiscard]] rd::Wrapper<RenderDocDrawCallDebugSession> start_debug_pixel(const ActionDescription * action) const;
};

} // namespace jetbrains::renderdoc

#endif    // RENDERDOCCAPTURE_H
