#ifndef RENDERDOC_WINDOWS_TESTS_H
#define RENDERDOC_WINDOWS_TESTS_H
#include "../renderdoc_service_tests.h"

struct Test1 final : AbstractTest {
  explicit Test1() : AbstractTest(L"samples/windows/test.rdc") {}
  void test(const rd::Lifetime &lifetime, const rd::Wrapper<jetbrains::renderdoc::RenderDocFile> &file, const rd::Wrapper<jetbrains::renderdoc::RenderDocReplay> &replay) override;
};

#endif //RENDERDOC_WINDOWS_TESTS_H
