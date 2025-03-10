#ifndef RENDERDOCLINEBREAKPOINTSMAPPER_H
#define RENDERDOCLINEBREAKPOINTSMAPPER_H
#include "RenderDocModel/RdcCapture.Generated.h"

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>

struct IReplayController;
struct ShaderSourceFile;
enum class ShaderStage : uint8_t;

namespace jetbrains::renderdoc {
class RenderDocLineBreakpointsMapper {
  struct SourceFileUsageEntry {
    uint32_t source_start_line;
    uint32_t source_end_line;
    uint32_t entry_start_line;

    struct comparator {
      std::size_t operator() (const SourceFileUsageEntry& l, const SourceFileUsageEntry& r) const {
        return l.source_end_line < r.source_end_line;
      }
    };
  };

  using usages_by_path = std::unordered_map<std::wstring, std::set<SourceFileUsageEntry, SourceFileUsageEntry::comparator>>;
  using usages_per_source_file = std::vector<usages_by_path>;

  std::unordered_map<uint32_t, usages_per_source_file> sources_usages_in_draw_calls;
  static bool try_register_sources_usages_in_file(usages_by_path &usages, const rd::Wrapper<model::RdcSourceFile> &file);

public:
  explicit RenderDocLineBreakpointsMapper();
  void register_sources_usages_in_draw_call(uint32_t event_id, const std::vector<rd::Wrapper<model::RdcSourceFile>> &files);
  [[nodiscard]]
  std::vector<std::pair<uint32_t, uint32_t>> map_source_line(uint32_t event_id, const std::wstring &file_path, uint32_t source_line) const;
};
}

#endif //RENDERDOCLINEBREAKPOINTSMAPPER_H
