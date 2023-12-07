#ifndef RENDERDOCSERVICE_H
#define RENDERDOCSERVICE_H
#include "RenderDocFile.h"
#include "types/wrapper.h"

#include <string>

class ActionDescription;

namespace jetbrains::renderdoc {
class RenderDocService {
public:
  RenderDocService();

  ~RenderDocService();
  rd::Wrapper<RenderDocFile> open_capture_file(const std::wstring &capture_path);
};
} // namespace jetbrains::renderdoc

#endif    // RENDERDOCSERVICE_H
