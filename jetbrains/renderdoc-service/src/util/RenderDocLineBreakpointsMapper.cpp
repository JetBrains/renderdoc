#include "RenderDocLineBreakpointsMapper.h"

#include <api/replay/apidefs.h>

namespace jetbrains::renderdoc {

const std::wregex RenderDocLineBreakpointsMapper::FILE_INFO_REGEX = std::wregex(LR"(//#line (\d+)(?:\s+"([^"]*)\")?)");

void RenderDocLineBreakpointsMapper::register_sources_usages_in_draw_call(uint32_t event_id, const std::vector<rd::Wrapper<model::RdcSourceFile>> &files) {
  file_indices[event_id] = {};
  for (uint32_t i = 0; i < files.size(); ++i) {
    const auto& file = files[i];
    file_indices[event_id].push_back(i);
    auto content = std::wistringstream(file->get_content());

    std::wstring prev_file_path;
    uint32_t prev_line_start = 0;
    uint32_t prev_source_line = 0;
    uint32_t j = 0;
    for (std::wstring line; std::getline(content, line); ++j) {
      if (std::wsmatch matches; std::regex_match(line, matches, FILE_INFO_REGEX) && matches.size() > 1) {
        if (!prev_file_path.empty() && prev_line_start != 0 && prev_source_line != 0) {
          sources_usages_in_draw_calls[{event_id, i}][prev_file_path][prev_source_line + j - prev_line_start] = {prev_source_line, prev_line_start};
        }

        prev_line_start = j + 1;
        prev_source_line = std::stoul(matches[1]);
        if (matches.size() < 3 || !matches[2].matched)
          continue;

        prev_file_path = matches[2];
      }
    }
    if (!prev_file_path.empty()) {
      sources_usages_in_draw_calls[{event_id, i}][prev_file_path][prev_source_line + j - prev_line_start] = {prev_source_line, prev_line_start};
    }
  }
}

std::vector<std::pair<uint32_t, uint32_t>> RenderDocLineBreakpointsMapper::map_source_line(uint32_t event_id, const std::wstring &file_path, uint32_t source_line) const {
  std::vector<std::pair<uint32_t, uint32_t>> res;
  for (uint32_t i : file_indices.at(event_id)) {
    const auto &usages_map = sources_usages_in_draw_calls.at(std::pair(event_id, i));

    if (const auto &it = usages_map.find(file_path); it != usages_map.end()) {
      const auto &usages = it->second;
      const auto &usage_it = usages.lower_bound(source_line);
      if (usage_it != usages.end() && usage_it->second.first <= source_line) {
        res.emplace_back(i, usage_it->second.second + source_line - usage_it->second.first + 1);
      }
    }
  }
  return res;
}
}