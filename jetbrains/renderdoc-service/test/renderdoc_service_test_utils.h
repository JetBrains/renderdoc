#ifndef SERVICETESTUTILS_H
#define SERVICETESTUTILS_H

#include "lifetime/Lifetime.h"
#include "types/wrapper.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace jetbrains::renderdoc {
namespace model {
class RdcDebugStack;
class RdcDrawCallDebugSession;
}
class RenderDocDebugSession;

struct LineTracker {
  std::vector<int32_t> lines;

  LineTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session);
};

struct FrameTracker {
  std::vector<rd::Wrapper<model::RdcDebugStack>> frames;
  std::vector<rd::Wrapper<model::RdcDrawCallDebugSession>> draw_call_sessions;
  std::vector<std::pair<std::size_t, int64_t>> draw_call_id_changes;

  FrameTracker(const rd::Lifetime &lifetime, rd::Wrapper<RenderDocDebugSession> debug_session);
};
} // namespace jetbrains::renderdoc

#endif // SERVICETESTUTILS_H
