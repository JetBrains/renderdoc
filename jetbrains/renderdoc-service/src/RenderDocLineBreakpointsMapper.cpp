#include "RenderDocLineBreakpointsMapper.h"

#include "RenderDocDrawCallDebugSession.h"
#include "util/RenderDocActionHelpers.h"
#include "util/RenderDocSourceFileUtil.h"
#include "util/StringUtils.h"

namespace jetbrains::renderdoc {

RenderDocLineBreakpointsMapper::RenderDocLineBreakpointsMapper(const std::vector<rd::Wrapper<model::RdcSourceFile>> &files) {
  auto &usages_cache = sources_usages;
  usages_cache.resize(files.size());
  for (uint32_t i = 0; i < files.size(); ++i) {
    try_register_sources_usages_in_file(usages_cache[i], files[i]);
  }
}

bool RenderDocLineBreakpointsMapper::try_register_sources_usages_in_file(usages_by_path &usages, const rd::Wrapper<model::RdcSourceFile> &file) {
  auto content = std::wistringstream(file->get_content());

  std::wstring prev_file_path;
  uint32_t prev_line_start = 0;
  uint32_t prev_source_line = 0;
  uint32_t j = 0;
  for (std::wstring line; std::getline(content, line); ++j) {
    if (std::wsmatch matches; std::regex_match(line, matches, helpers::FILE_ENTRY_INFO_REGEX) && matches.size() > 1) {
      if (!prev_file_path.empty() && prev_line_start != 0 && prev_source_line != 0) {
        SourceFileUsageEntry entry = {prev_source_line, prev_source_line + j - prev_line_start - 1, prev_line_start};
        usages[prev_file_path].insert(entry);
      }

      prev_line_start = j + 1;
      prev_source_line = static_cast<uint32_t>(std::stoul(matches[1]));
      if (matches.size() < 3 || !matches[2].matched)
        continue;

      prev_file_path = matches[2];
    } else if (j == 0)
      return false; // file doesn't contain file usages info in form //line <line> "<file_path>"
  }
  if (!prev_file_path.empty()) {
    SourceFileUsageEntry entry = {prev_source_line, prev_source_line + j - prev_line_start - 1, prev_line_start};
    usages[prev_file_path].insert(entry);
  }
  return true;
}

std::vector<std::pair<uint32_t, uint32_t>> RenderDocLineBreakpointsMapper::map_source_line(const std::wstring &file_path, uint32_t source_line) const {
  std::vector<std::pair<uint32_t, uint32_t>> res;
  for (uint32_t i = 0; i < sources_usages.size(); ++i) {
    const auto &usages_map = sources_usages.at(i);

    if (const auto &it = usages_map.find(file_path); it != usages_map.end()) {
      const auto &usages = it->second;
      const auto &usage_it = usages.lower_bound(SourceFileUsageEntry{ 0, source_line, 0 });
      if (usage_it != usages.end() && usage_it->source_start_line <= source_line) {
        res.emplace_back(i, usage_it->entry_start_line + source_line - usage_it->source_start_line + 1);
      }
    }
  }
  return res;
}
}