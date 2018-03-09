#ifndef FILE_HELPERS_H_
#define FILE_HELPERS_H_

#include <string>
#include <fstream>
#include <iostream>
#include <cstddef>
#include <sys/stat.h>

#include "challenge.grpc.pb.h"

const size_t CHUNK_SIZE = 1024 * 1024;

inline size_t get_file_size(const std::string& filename) {
  std::ifstream in_file(filename, std::ios::ate | std::ios::binary);
  return in_file.tellg();
}

inline bool file_exists(const std::string& filename) {
  struct stat buffer;
  return (stat(filename.c_str(), &buffer) == 0);
}

inline bool try_removing_existing(const std::string& filename) {
  if(!::file_exists(filename))
    return true;

  //TODO: need some config/flag in message whether to overwrite the existing file or not
  if(std::remove(filename.c_str()) != 0) {
    std::cerr << "The file[" << filename << "] already exists. Can't overwrite" << std::endl;
    return false;
  } else {
    std::cout << "Removed existing file[" << filename << "]." << std::endl;
  }

  return true;
}

//TODO: actually use control sum in FileChunk/FileAck messages..

class FileChunker {
  public:
    FileChunker(const std::string& filename):
      filename_(filename),
      valid_file_(false),
      number_of_chunks_(0),
      chunk_number_(0),
      current_pos_(0)
    {
      in_file_.open(filename_, std::ios::in|std::ios::binary);
      valid_file_ = in_file_.is_open();
      if(!valid_file_) {
        std::cerr << "Can't open file[" << filename_ << "]" << std::endl;
        return;
      }
      buffer_ = new char[CHUNK_SIZE];

      in_file_.seekg(0, std::ios::beg);

      file_size_ = get_file_size(filename_);

      std::cout << "Size of[" << filename_ << "]: " << file_size_ << std::endl;

      number_of_chunks_ = static_cast<uint32_t>(file_size_/CHUNK_SIZE)
        + static_cast<uint32_t>(static_cast<bool>(file_size_%CHUNK_SIZE));
      std::cout << "We will create: " << number_of_chunks_ << " chunks" << std::endl;

      in_file_.seekg(0, std::ios::beg);
      current_pos_ = in_file_.tellg();
    }

    ~FileChunker() {
      delete[] buffer_;
      if(valid_file_)
        in_file_.close();
    }

    void reset_to_beginning_of_file() {
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
      size_t read_size(CHUNK_SIZE);
      if(file_size_ - current_pos_ < CHUNK_SIZE) {
        read_size = file_size_ - current_pos_;
      }
      //std::cout << "Current position: " << current_pos_ << std::endl;
      //std::cout << "Chunk number: " << chunk_number_ << std::endl;
      //std::cout << "Reading now: " << read_size << std::endl;
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
      if(chunk_number_ == number_of_chunks_) {
        std::cout << "Generated last chunk of file[" << filename_ << "]" << std::endl;
      }

      return true;
    }

  private:
    const std::string filename_;

    bool valid_file_;
    uint32_t number_of_chunks_;
    char* buffer_;//[CHUNK_SIZE];
    uint32_t chunk_number_;
    std::ifstream::pos_type current_pos_;
    size_t file_size_;

    std::ifstream in_file_;
};

class FileReceiver {
  public:
    FileReceiver(const std::string& filename):
      filename_(filename),
      out_file_(filename_, std::ios::out | std::ios::binary | std::ios::app) {
      if(!out_file_.is_open()) {
        std::cerr << "Can't open file[" << filename_ << "]" << std::endl;
        return;
      } else {
        std::cout << "Writing to a file[" << filename_ << "]" << std::endl;
      }
    }
    bool add_chunk(const challenge::FileChunk& file_chunk) {
      if(!out_file_.is_open())
        return false;
      std::string buffer(file_chunk.data());
      if(buffer.size() != file_chunk.sizeinbytes()) {
        std::cerr << "Received data size[" << buffer.size()
                  << "] differs from FileChunk.sizeinbytes[" << file_chunk.sizeinbytes()
                  << "]" << std::endl;
        return false;
      }
      out_file_.write(buffer.c_str(), file_chunk.sizeinbytes());

      if(file_chunk.islastchunk()) {
        std::cout << "Received last chunk for file[" << filename_
                  << "]. Size[" << this->get_file_size() << "]" << std::endl;
        out_file_.close();
      }
      return true;
    }

    ~FileReceiver() {
      if(out_file_.is_open())
        out_file_.close();
    }

    size_t get_file_size() const {
      return ::get_file_size(filename_);
    }

  private:
    const std::string filename_;
    std::ofstream out_file_;
};

#endif
