#ifndef JETBRAINS_RENDERDOC_PEER_BASE_H
#define JETBRAINS_RENDERDOC_PEER_BASE_H

#include "lifetime/LifetimeDefinition.h"
#include "protocol/Protocol.h"
#include "scheduler/SimpleScheduler.h"
#include "wire/SocketWire.h"

namespace jetbrains
{
namespace renderdoc
{
template <class TWire>
class PeerBase
{
protected:
  using printer_t = std::vector<std::string>;

  rd::SimpleScheduler scheduler{};

  rd::LifetimeDefinition definition{false};
  rd::Lifetime model_lifetime = definition.lifetime;

  rd::LifetimeDefinition socket_definition{false};
  rd::Lifetime socket_lifetime = definition.lifetime;

  std::shared_ptr<TWire> wire;
  std::unique_ptr<rd::IProtocol> protocol;

public:
  PeerBase() = default;

  virtual ~PeerBase() = default;

protected:
  virtual int run() = 0;

  void terminate()
  {
    socket_definition.terminate();
    definition.terminate();
  }
};
}	 // namespace renderdoc
}	 // namespace jetbrains

#endif    //   JETBRAINS_RENDERDOC_PEER_BASE_H
