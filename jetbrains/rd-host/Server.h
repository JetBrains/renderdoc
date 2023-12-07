#ifndef JETBRAINS_RENDERDOC_SERVER_H
#define JETBRAINS_RENDERDOC_SERVER_H

#include "RenderDocModel/RdcCapture.Generated.h"
#include "RenderDocModel/RenderDocModel.Generated.h"
#include "RenderDocServiceApi.h"

#include "PeerBase.h"

namespace jetbrains::renderdoc::rdhost
{

class Server final : public PeerBase<rd::SocketWire::Server>
{
  std::mutex terminationMutex;
  std::condition_variable terminationCondition;
  std::unique_ptr<RenderDocService> service;
  bool shouldTerminate = false;

  void set_up_model(model::RenderDocModel& model);

public:
  Server();

  int get_port() const { return wire->port; }
  int run() override;
  void request_termination();
};

}

#endif	  // JETBRAINS_RENDERDOC_SERVER_H
