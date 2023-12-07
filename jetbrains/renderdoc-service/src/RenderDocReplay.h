#ifndef RENDERDOCCAPTURE_H
#define RENDERDOCCAPTURE_H
#include "RenderDocDebugSession.h"
#include "RenderDocModel/RdcCapture.Generated.h"

struct IReplayController;

namespace jetbrains::renderdoc {

class RenderDocReplay : public model::RdcCapture {
  std::shared_ptr<IReplayController> controller;

public:
  explicit RenderDocReplay(IReplayController *controller);

  [[nodiscard]] rd::Wrapper<RenderDocDebugSession> debug_vertex(const rd::Lifetime &session_lifetime, uint32_t event_id) const;
};

} // namespace jetbrains::renderdoc

#endif    // RENDERDOCCAPTURE_H
