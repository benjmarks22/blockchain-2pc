#ifndef SRC_CLIENT_GREETER_CLIENT_H_

#define SRC_CLIENT_GREETER_CLIENT_H_

#include <memory>

#include "grpcpp/channel.h"
#include "grpcpp/client_context.h"

#include "src/proto/greeter.grpc.pb.h"

namespace client {

class Client {
 public:
  Client(std::shared_ptr<grpc::Channel> channel)
      : stub_(greeter::Greeter::NewStub(channel)) {}

  void Greet();
  private:
  std::unique_ptr<greeter::Greeter::Stub> stub_;
};

} // namespace client

#endif  // SRC_CLIENT_GREETER_CLIENT_H_