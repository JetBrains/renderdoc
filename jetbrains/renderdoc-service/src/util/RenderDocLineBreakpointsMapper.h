#ifndef RENDERDOCLINEBREAKPOINTSMAPPER_H
#define RENDERDOCLINEBREAKPOINTSMAPPER_H
#include "RenderDocModel/RdcCapture.Generated.h"

#include <cstdint>
#include <string>
#include <unordered_map>

namespace jetbrains::renderdoc {
class RenderDocLineBreakpointsMapper {
  static const std::wregex FILE_INFO_REGEX;

  struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2>& p) const {
      return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
    }
  };

  // event_id to {file_idx[...]}
  std::unordered_map<uint32_t, std::vector<uint32_t>> file_indices;
  // {event_id, file_idx} to (file_path to (end_pos to {start_pos, decompiled_start_pos}))
  std::unordered_map<std::pair<uint32_t, uint32_t>, std::unordered_map<std::wstring, std::map<uint32_t, std::pair<uint32_t, uint32_t>>>, pair_hash> sources_usages_in_draw_calls;

public:
  std::vector<std::pair<uint32_t, uint32_t>> map_source_line(uint32_t event_id, const std::wstring &file_path, uint32_t source_line) const;
  void register_sources_usages_in_draw_call(uint32_t event_id, const std::vector<rd::Wrapper<model::RdcSourceFile>> &files);
};
}

#endif //RENDERDOCLINEBREAKPOINTSMAPPER_H
