#include <iostream>
#include <string>

#include <grpc++/grpc++.h>

#include "challenge.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using challenge::StringRequest;
using challenge::StringReply;
using challenge::NumberRequest;
using challenge::NumberReply;
using challenge::Handler;

class ChallengeClient {
  public:
    ChallengeClient(std::shared_ptr<Channel> channel):
      stub_(Handler::NewStub(channel)) {}

    std::string SendString(const std::string& str) {
      StringRequest request;
      request.set_str(str);

      StringReply reply;

      ClientContext context;
      Status status = stub_->SayString(&context, request, &reply);

      if(status.ok()) {
        return reply.message();
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
      }
    }
    std::string SendNumber(const int32_t& number) {
      NumberRequest request;
      request.set_num(number);

      NumberReply reply;
      ClientContext context;
      Status status = stub_->SayNumber(&context, request, &reply);

      if(status.ok()) {
        return "Success!";
      } else {
        std::cout << status.error_code() << ": " << status.error_message()
                  << std::endl;
        return "RPC failed";
      }
    }

  private:
    std::unique_ptr<Handler::Stub> stub_;
};

int main(int argc, char** argv) {
  ChallengeClient challenge(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string str("test string");
  std::string reply = challenge.SendString(str);
  std::cout << "Server said: " << reply << std::endl;

  int32_t num(14);
  reply = challenge.SendNumber(num);
  std::cout << "Server said: " << reply << std::endl;

  return 0;
}
