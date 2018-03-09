#include "FileHelpers.h"

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
        return std::to_string(reply.message());
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

      FileChunker file_chunker(filename);
      challenge::FileChunk file_chunk;
      while(file_chunker.get_next_chunk(file_chunk)) {
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
        std::cerr << "SendFile rpc failed." << status.error_message() <<  std::endl;
      }

      return file_ack.filename();
    }

    //TODO: take second parameter where to store received file
    std::string RequestFile(const std::string& filename, const std::string& rcv_filename) {
      challenge::FileRequest file_request;
      file_request.set_filename(filename);

      ClientContext context;
      std::unique_ptr<::grpc::ClientReader<challenge::FileChunk> > reader(
          stub_->RequestFile(&context, file_request));

      ::challenge::FileChunk file_chunk;
      if(!reader->Read(&file_chunk)) {
        Status status = reader->Finish();
        return status.error_message();
      }

      if(!::try_removing_existing(rcv_filename)) {
        std::cerr << "Couldn't remove existing file[" << rcv_filename << "]";
        return "Requesting file failed. File[" + rcv_filename +"] exists and cannot be removed";
      }

      FileReceiver file_receiver(rcv_filename);
      file_receiver.add_chunk(file_chunk);
      while(reader->Read(&file_chunk)) {
        if(!file_receiver.add_chunk(file_chunk)) {
          std::cerr << "Can't append FileChunk for file[" << rcv_filename << "]" << std::endl;
          return "INTERNAL ERROR: can't append FileChunks to[" + rcv_filename + "]";
        }
      }
      return "Finished receiving[" + filename + "]";
    }

  private:
    std::unique_ptr<Handler::Stub> stub_;
};

int main(int argc, char** argv) {
  ChallengeClient challenge(grpc::CreateChannel(
        "localhost:50051", grpc::InsecureChannelCredentials()));
  std::string str("test string");
  std::string reply = challenge.SendString(str);
  std::cout << "SendString: " << reply << std::endl;

  int32_t num(14);
  reply = challenge.SendNumber(num);
  std::cout << "SendNumber: " << reply << std::endl;

  std::string filename("test.img");
  reply = challenge.SendFile(filename);
  std::cout << "SendFile: " << reply << std::endl;

  filename = "lorem.img";
  std::string rcv_filename("received_" + filename);
  reply = challenge.RequestFile(filename, rcv_filename);
  std::cout << "RequestFile: " << reply << std::endl;

  filename = "lorem_ipsum.img";
  rcv_filename = "received_" + filename;
  reply = challenge.RequestFile(filename, rcv_filename);
  std::cout << "RequestFile: " << reply << std::endl;

  return 0;
}
