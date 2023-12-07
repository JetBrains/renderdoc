#ifndef RENDERDOCFILE_H
#define RENDERDOCFILE_H
#include "RenderDocModel/RdcCaptureFile.Generated.h"
#include "RenderDocReplay.h"

class ICaptureFile;
namespace jetbrains::renderdoc {

class RenderDocFile : public model::RdcCaptureFile {
public:
  typedef std::unique_ptr<ICaptureFile, std::function<void(ICaptureFile*)>> capture_file_ptr;
private:
  capture_file_ptr capture_file;
public:
  explicit RenderDocFile(capture_file_ptr capture_file);

  static rd::Wrapper<RenderDocFile> open(const std::wstring &file_path);

  [[nodiscard]] rd::Wrapper<RenderDocReplay> open_capture() const;

  RenderDocFile(RenderDocFile&& other) noexcept = delete;
};

}


#endif //RENDERDOCFILE_H
