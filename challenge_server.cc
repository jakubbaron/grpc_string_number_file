#include <iostream>
#include <string>
#include <fstream>

#include <grpc++/grpc++.h>
#include "challenge.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using challenge::StringRequest;
using challenge::StringReply;
using challenge::NumberRequest;
using challenge::NumberReply;
using challenge::Handler;

class ChallengeServiceImpl final: public Handler::Service {
  Status SayString(ServerContext* context, const StringRequest* request,
      StringReply* reply) override {
    std::string prefix("Received ");
    reply->set_message(prefix + request->str());
    return Status::OK;
  }
  Status SayNumber(ServerContext* context, const NumberRequest* request,
      NumberReply* reply) override {
    int32_t received = request->num();
    reply->set_message(received * 2);
    return Status::OK;
  }
  Status ReceiveFile(::grpc::ServerContext* context,
      ::grpc::ServerReader< ::challenge::FileChunk>* reader,
      ::challenge::FileAck* response) override { 
    ::challenge::FileChunk file_chunk;
    size_t len = 0;
    if(!reader->Read(&file_chunk))
      return Status::OK;

    std::string filename = "s_" + file_chunk.filename();
    std::ofstream received_file;
    received_file.open(filename.c_str(), std::ios::out | std::ios::binary | std::ios::app);
    if(!received_file.is_open()) {
      std::cerr << "Can't open file[" << filename << "]" << std::endl;
      return Status::OK;
    }
    std::string buffer(file_chunk.data());
    received_file.write(buffer.c_str(), file_chunk.sizeinbytes());
    while(reader->Read(&file_chunk))
    {
      std::cout << "Received FileChunk: " << file_chunk.filename() << std::endl;
      std::cout << "Received Chunk: " << file_chunk.chunknumber() << std::endl;
      std::cout << "This is " << (file_chunk.islastchunk() ? "" : "NOT ") << "last file chunk" << std::endl;
      len += file_chunk.sizeinbytes();
      std::cout << "Read: " << len << " bytes" << std::endl;

      buffer.clear();
      buffer = file_chunk.data();
      received_file.write(buffer.c_str(), file_chunk.sizeinbytes());
    }

    received_file.close();

    response->set_filename(file_chunk.filename());
    return Status::OK;
  }
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  ChallengeServiceImpl service;
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on: " << server_address << std::endl;

  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}
