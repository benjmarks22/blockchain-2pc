#ifndef SRC_SERVER_GREETER_SERVER_H_

#define SRC_SERVER_GREETER_SERVER_H_

#include "grpcpp/server_context.h"
#include "src/proto/greeter.grpc.pb.h"

namespace server {

class GreeterServer : public greeter::Greeter::Service {
 public:
  grpc::Status Greet(grpc::ServerContext *context,
                     const greeter::GreetRequest *request,
                     greeter::GreetResponse *response) override;
};

}  // namespace server

#endif  // SRC_SERVER_GREETER_SERVER_H_