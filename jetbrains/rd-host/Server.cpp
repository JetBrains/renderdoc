#include "Server.h"

#include "wire/SocketWire.h"

#include "RenderDocModel/RenderDocModel.Generated.h"

/**
 * \brief RenderDoc Host
 */
namespace jetbrains::renderdoc::rdhost
{

Server::Server() : service(std::make_unique<RenderDocService>())
{
  wire = std::make_shared<rd::SocketWire::Server>(socket_lifetime, &scheduler, 0, "TestServer");
  protocol = std::make_unique<rd::Protocol>(rd::Identities::SERVER, &scheduler, wire, socket_lifetime);
}

int Server::run()
{
  model::RenderDocModel model;
  scheduler.queue([this, &model] { set_up_model(model); });

  {
    std::unique_lock lock(terminationMutex);
    while (!shouldTerminate)
      terminationCondition.wait(lock);
  }

  terminate();

  return 0;
}

void Server::request_termination()
{
  std::unique_lock lock(terminationMutex);
  shouldTerminate = true;
  terminationCondition.notify_one();
}

void Server::set_up_model(model::RenderDocModel& model)
{
  model.connect(model_lifetime, protocol.get());
  model.get_openCaptureFile().set([this](const auto &req) {
    return this->service->open_capture_file(req);
  });
}

}
