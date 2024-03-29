#include "FileHelpers.h"

#include <iostream>
#include <string>

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
  public:
    ChallengeServiceImpl(const std::string& storage_dir = ""):
      Handler::Service(),
      storage_dir_(storage_dir)
    {}
    Status SayString(ServerContext* context, const StringRequest* request,
        StringReply* reply) override {
      static const std::string prefix("Received ");
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

      if(!reader->Read(&file_chunk))
        return Status(grpc::INTERNAL, "Can't read FileChunk from the stream");

      const std::string filename = storage_dir_ + file_chunk.filename();
      if(!::try_removing_existing(filename)) {
        return Status(grpc::ALREADY_EXISTS, "File[" + filename + "] already exists and can't overwrite");
      }

      FileReceiver file_receiver(filename);
      file_receiver.add_chunk(file_chunk);
      while(reader->Read(&file_chunk)) {
        if(!file_receiver.add_chunk(file_chunk)) {
          std::cerr << "Can't append FileChunk for file[" << filename << "]" << std::endl;
          response->set_filename(filename);
          response->set_sizeinbytes(file_receiver.get_file_size());
          response->set_error("INTERNAL ERROR: can't append FileChunks");
          return Status(::grpc::INTERNAL,
                        "Can't append FileChunk to file[" + filename + "]");
        }
      }

      response->set_filename(filename);
      response->set_sizeinbytes(file_receiver.get_file_size());
      response->set_error("");

      return Status::OK;
    }

    Status RequestFile(::grpc::ServerContext* context,
             const ::challenge::FileRequest* request,
             ::grpc::ServerWriter< ::challenge::FileChunk>* writer) override {
      const std::string filename = storage_dir_ + request->filename();
      if(!::file_exists(filename)) {
        return Status(::grpc::NOT_FOUND,
                      "File[" + request->filename() + "] not found at the server");
      }
      FileChunker file_chunker(filename);
      challenge::FileChunk file_chunk;
      while(file_chunker.get_next_chunk(file_chunk)) {
        if(!writer->Write(file_chunk)) {
          std::cerr << "Broken stream!" << std::endl;
          return Status(::grpc::INTERNAL, "Broken stream!");
        }
      }

      return Status::OK;
    }

  private:
    const std::string storage_dir_;
};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  static const std::string storage_dir("server_storage/");
  ChallengeServiceImpl service(storage_dir);
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
