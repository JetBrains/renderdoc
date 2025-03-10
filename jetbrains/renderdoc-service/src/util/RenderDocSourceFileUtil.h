#ifndef RENDERDOCSOURCEFILEUTIL_H
#define RENDERDOCSOURCEFILEUTIL_H

#include <regex>

namespace jetbrains::renderdoc::helpers {
const inline std::wregex FILE_ENTRY_INFO_REGEX = std::wregex(LR"(//#line (\d+)(?:\s+"([^"]*)\")?)");
}
#endif //RENDERDOCSOURCEFILEUTIL_H
