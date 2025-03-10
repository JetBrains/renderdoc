#ifndef SERVICETESTUTILS_H
#define SERVICETESTUTILS_H

#include "RenderDocModel/RdcDebugPixelInput.Generated.h"
#include "RenderDocModel/RdcDebugVertexInput.Generated.h"
#include "types/wrapper.h"
#include <cstdint>
#include <optional>
#include <set>
#include <vector>

namespace jetbrains::renderdoc {
namespace model {
class RdcSourceFilesInAction;
class RdcAction;
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


template <typename T>
std::vector<T> unwrap_range(const std::vector<rd::Wrapper<T>> &range) {
  std::vector<T> result;
  result.reserve(range.size());
  for (const auto &item : range) {
    result.push_back(*item);
  }
  return result;
}

uint32_t get_current_draw_call_id(const rd::Wrapper<RenderDocDebugSession> &debug_session);
std::map<uint32_t, model::RdcSourceFilesInAction> get_file_usages_in_events(const rd::Wrapper<model::RdcAction>& action);
std::map<uint32_t, model::RdcSourceFilesInAction> get_file_usages_in_events(const rd::Wrapper<RenderDocReplay> &replay);
void assert_source_file_usages(const model::RdcSourceFilesInAction &left, const std::set<std::wstring> &expected_entrypoints,  const std::set<std::wstring> &expected_others);
void assert_session_finishes_immediately(const rd::Lifetime &lifetime, const rd::Wrapper<RenderDocReplay> &replay, const RdcDebugInput &input, bool debug_single_call);
void assert_float_2d_vectors_are_equal(const std::vector<std::vector<float>> &actual, const std::vector<std::vector<float>> &expected);
} // namespace jetbrains::renderdoc

#endif // SERVICETESTUTILS_H
