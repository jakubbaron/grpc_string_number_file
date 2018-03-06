#include <iostream>
#include <string>

#include <grpc++/grpc++.h>

#include "challenge.grpc.pb.h"
#include <vector>

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
    std::string SendFile(const std::string& filename) {
      challenge::FileAck file_ack;
      ClientContext context;

      std::unique_ptr<::grpc::ClientWriter<challenge::FileChunk> > writer(
        stub_->ReceiveFile(&context, &file_ack));
      std::vector<std::string> fnames = {"first", "second"};
      for(int i = 0; i < 2; ++i) {
        challenge::FileChunk file_chunk;
        file_chunk.set_filename(fnames[i]);
        if(!writer->Write(file_chunk)) {
          std::cerr << "Broken stream!" << std::endl;
          break;
        }
      }
      writer->WritesDone();
      Status status = writer->Finish();
      if(status.ok()) {
        std::cout << "Finished writing to the stream" << std::endl;
      } else {
        std::cerr << "SendFile rpc failed." << std::endl;
      }
      return file_ack.filename();
    }

  private:
    std::unique_ptr<Handler::Stub> stub_;
};

class FileChunker {
  public:
    FileChunker(const std::string& filepath):
      filepath_(filepath)
    {}
  private:
    const std::string filepath_;
};

int main(int argc, char** argv) {
  ChallengeClient challenge(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string str("test string");
  std::string reply = challenge.SendString(str);
  std::cout << "SendString: Server said: " << reply << std::endl;

  int32_t num(14);
  reply = challenge.SendNumber(num);
  std::cout << "SendNumber: Server said: " << reply << std::endl;

  std::string filename("test filename");
  reply = challenge.SendFile(filename);
  std::cout << "SendFile: Server said: " << reply << std::endl;

  return 0;
}
