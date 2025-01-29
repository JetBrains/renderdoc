#ifndef SERVICETESTUTILS_H
#define SERVICETESTUTILS_H

#include "lifetime/Lifetime.h"
#include "types/wrapper.h"
#include <cstdint>
#include <vector>

namespace jetbrains::renderdoc {
namespace model {
class RdcDebugStack;
}
class RenderDocDebugSession;

struct LineTracker {
  std::vector<int32_t> lines;

  LineTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session);
};

struct FrameTracker {
  std::vector<rd::Wrapper<model::RdcDebugStack>> frames;

  FrameTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session);
};
} // namespace jetbrains::renderdoc

#endif // SERVICETESTUTILS_H
