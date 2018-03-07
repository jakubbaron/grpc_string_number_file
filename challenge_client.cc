#include <iostream>
#include <string>

#include <grpc++/grpc++.h>

#include "challenge.grpc.pb.h"
#include <vector>

#include <fstream>
#include <cstddef>

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
      //TODO: replace with reading actual chunks of the file
      //TODO: not hardcode the file

      const std::string fname = "test.img";
      const size_t CHUNK_SIZE = 1024 * 1024; // 1MB
      std::ifstream in_file;
      in_file.open(fname, std::ios::in|std::ios::binary|std::ios::ate);
      if(!in_file.is_open()) {
        std::cerr << "Can't open file[" << fname << "]" << std::endl;
        return "File[" + fname + "] reading failed";
      }
      in_file.seekg(0, std::ios::end);
      std::ifstream::pos_type file_size = in_file.tellg(); //tells the size of the file
      std::cout << "Size of the file: " << file_size << std::endl;
      const uint32_t number_of_chunks = static_cast<uint32_t>(file_size/CHUNK_SIZE) 
        + static_cast<uint32_t>(file_size%CHUNK_SIZE);
      std::cout << "We will create: " << number_of_chunks << " chunks" << std::endl;
      in_file.seekg(0, std::ios::beg);

      char buffer[CHUNK_SIZE];
      uint32_t chunk_number = 0;
      size_t read_size = CHUNK_SIZE;
      std::ifstream::pos_type current_pos = in_file.tellg();
      while(current_pos < file_size) {
        std::cout << "Current position: " << current_pos << std::endl;
        std::cout << "Chunk number: " << chunk_number << std::endl;
        if(file_size - current_pos >= CHUNK_SIZE) {
          read_size = CHUNK_SIZE;
        } else {
          read_size = file_size - current_pos;
        }
        std::cout << "Reading now: " << read_size << std::endl;
        if(!in_file.read(buffer, read_size)) {
          std::cerr << "Failed to read[" << read_size 
                    << "] from file[" << fname 
                    << "] at[" << current_pos << "]" 
                    << std::endl;
          break;
        }
        challenge::FileChunk file_chunk; 
        //TODO: create a pointer and just change relevant data in the object
        file_chunk.set_filename(fname);
        file_chunk.set_data(buffer, read_size);
        file_chunk.set_chunknumber(chunk_number++);
        file_chunk.set_islastchunk(chunk_number == number_of_chunks);
        file_chunk.set_sizeinbytes(read_size);

        //As the last step, write the chunk to the stream
        if(!writer->Write(file_chunk)) {
          std::cerr << "Broken stream!" << std::endl;
          break;
        }
        current_pos = in_file.tellg();
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
