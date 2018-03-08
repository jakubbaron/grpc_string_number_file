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

const size_t CHUNK_SIZE = 1024 * 1024;

size_t get_file_size(std::ifstream& in_file) {
  if(!in_file.is_open())
    return 0;

  std::ifstream::pos_type current_pos = in_file.tellg();
  std::cout << "Cursor at the beginning: " << current_pos << std::endl;
  in_file.seekg(0, std::ios::end);
  std::ifstream::pos_type file_size = in_file.tellg(); //tells the size of the file
  in_file.seekg(current_pos, std::ios::cur);
  std::cout << "Moved cursor to: " << current_pos << std::endl;

  return file_size;
}

class FileChunker {
  public:
    FileChunker(const std::string& filename):
      filename_(filename),
      valid_file_(false),
      number_of_chunks_(0),
      chunk_number_(0),
      current_pos_(0)
    {
      in_file_.open(filename_, std::ios::in|std::ios::binary|std::ios::ate);
      valid_file_ = in_file_.is_open();
      if(!valid_file_) {
        std::cerr << "Can't open file[" << filename_ << "]" << std::endl;
        return;
      }

      in_file_.seekg(0, std::ios::beg);

      file_size_ = get_file_size(in_file_);

      std::cout << "Size of the file: " << file_size_ << std::endl;

      number_of_chunks_ = static_cast<uint32_t>(file_size_/CHUNK_SIZE) 
        + static_cast<uint32_t>(file_size_%CHUNK_SIZE);
      std::cout << "We will create: " << number_of_chunks_ << " chunks" << std::endl;

      in_file_.seekg(0, std::ios::beg);
      current_pos_ = in_file_.tellg();
    }
    void reset_to_begin_of_file() {
      number_of_chunks_ = 0;
      chunk_number_ = 0;
      in_file_.seekg(0, std::ios::beg);
      current_pos_ = 0;
    }

    bool get_next_chunk(challenge::FileChunk& file_chunk) {
      if(!valid_file_)
        return false;
      if(current_pos_ >= file_size_)
        return false;

      std::cout << "Current position: " << current_pos_ << std::endl;
      std::cout << "Chunk number: " << chunk_number_ << std::endl;
      size_t read_size(CHUNK_SIZE);
      if(file_size_ - current_pos_ < CHUNK_SIZE) {
        read_size = file_size_ - current_pos_;
      }
      std::cout << "Reading now: " << read_size << std::endl;
      if(!in_file_.read(buffer_, read_size)) {
        std::cerr << "Failed to read[" << read_size 
                  << "] from file[" << filename_
                  << "] at[" << current_pos_ << "]" 
                  << std::endl;
        return false;
      }
      file_chunk.set_filename(filename_);
      file_chunk.set_data(buffer_, read_size);
      file_chunk.set_chunknumber(chunk_number_++);
      file_chunk.set_islastchunk(chunk_number_ == number_of_chunks_);
      file_chunk.set_sizeinbytes(read_size);
      current_pos_ = in_file_.tellg();

      return true;
    }

  private:
    const std::string filename_;

    bool valid_file_;
    uint32_t number_of_chunks_;
    char buffer_[CHUNK_SIZE];
    uint32_t chunk_number_;
    std::ifstream::pos_type current_pos_;  
    size_t file_size_;

    std::ifstream in_file_;
};


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
        std::cerr << "SendFile rpc failed." << std::endl;
      }

      return file_ack.filename();
    }

  private:
    std::unique_ptr<Handler::Stub> stub_;
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

  std::string filename("test.img");
  reply = challenge.SendFile(filename);
  std::cout << "SendFile: Server said: " << reply << std::endl;

  return 0;
}
