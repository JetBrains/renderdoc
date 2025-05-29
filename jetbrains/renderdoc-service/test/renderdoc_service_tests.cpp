#include "renderdoc_service_tests.h"

#include "lifetime/LifetimeDefinition.h"
#include "RenderDocServiceApi.h"

jetbrains::renderdoc::RenderDocService AbstractTest::service = jetbrains::renderdoc::RenderDocService();

void AbstractTest::run() {
  const rd::LifetimeDefinition test_lifetime_def;
  const auto lifetime = test_lifetime_def.lifetime;
  try {
    const auto file = service.open_capture_file(capture_name);
    const auto replay = file->open_capture();

    test(lifetime, file, replay);
  } catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
  }
}
