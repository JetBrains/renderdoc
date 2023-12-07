#include <csignal>
#include <iostream>
#include <thread>
#include "Server.h"

int main() {
  static jetbrains::renderdoc::rdhost::Server server;
  
  auto handler = [](int) { server.request_termination(); };
  signal(SIGINT, handler);
  signal(SIGTERM, handler);

  std::thread parentProcessWatcher([] {
    while (getchar() != EOF) { } // wait for stdin termination
    server.request_termination();
  });
  parentProcessWatcher.detach();

  std::cout << "HOST_INTRODUCTION: PORT=" << server.get_port() << std::endl;
  server.run();
}
