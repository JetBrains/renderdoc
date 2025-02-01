#ifndef SERVICETESTUTILS_H
#define SERVICETESTUTILS_H

#include "RenderDocModel/RdcDebugPixelInput.Generated.h"
#include "RenderDocModel/RdcDebugVertexInput.Generated.h"
#include "types/wrapper.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace jetbrains::renderdoc {
namespace model {
class RdcDebugStack;
class RdcDrawCallDebugSession;
} // namespace model
class RenderDocDebugSession;
class RenderDocReplay;

struct RdcDebugInput {
  union {
    model::RdcDebugVertexInput vertex;
    model::RdcDebugPixelInput pixel;
  };

  explicit RdcDebugInput(model::RdcDebugVertexInput &&vertex);
  explicit RdcDebugInput(model::RdcDebugPixelInput &&pixel);
  ~RdcDebugInput() {}

  enum { Vertex, Pixel } type;
};

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

void assert_session_finishes_immediately(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const RdcDebugInput &input, bool debug_single_call);
void assert_float_2d_vectors_are_equal(std::vector<std::vector<float>> actual, std::vector<std::vector<float>> expected);
} // namespace jetbrains::renderdoc

#endif // SERVICETESTUTILS_H
