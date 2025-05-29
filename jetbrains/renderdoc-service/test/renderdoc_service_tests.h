#ifndef RENDERDOC_SERVICE_TESTS_H
#define RENDERDOC_SERVICE_TESTS_H
#include "types/wrapper.h"

#include <string>

namespace rd {
class Lifetime;
}
namespace jetbrains::renderdoc {
class RenderDocReplay;
class RenderDocFile;
class RenderDocService;
}
struct AbstractTest {
  explicit AbstractTest(const std::wstring &capture_name) : capture_name(capture_name) {}
  virtual ~AbstractTest() = default;
  virtual void run();

protected:
  const std::wstring capture_name;
  virtual void test(const rd::Lifetime &lifetime, const rd::Wrapper<jetbrains::renderdoc::RenderDocFile> &file, const rd::Wrapper<jetbrains::renderdoc::RenderDocReplay> &replay) = 0;

private:
  static jetbrains::renderdoc::RenderDocService service;
};

#endif //RENDERDOC_SERVICE_TESTS_H
