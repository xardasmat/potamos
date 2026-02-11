#pragma once

#include <istream>
#include <vector>

namespace potamos {

class FileBuffer : public std::streambuf {
 public:
  FileBuffer(FILE* file, size_t bufferSize = 1024)
      : file_(file), buffer_(bufferSize) {
    setg(buffer_.data(), buffer_.data(), buffer_.data());
  }
  ~FileBuffer() { pclose(file_); }

 protected:
  virtual int_type underflow() override {
    // If the current pointer hasn't reached the end, just return current char
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    // Try to read a chunk of data from the FILE*
    size_t bytesRead = std::fread(buffer_.data(), 1, buffer_.size(), file_);

    if (bytesRead == 0) {
      return traits_type::eof();  // End of file reached
    }

    // Reset the pointers: [start, current, end]
    setg(buffer_.data(), buffer_.data(), buffer_.data() + bytesRead);

    return traits_type::to_int_type(static_cast<unsigned char>(*gptr()));
  }

 private:
  FILE* file_;
  std::vector<char> buffer_;
};

class iPipeStream : public std::istream {
 public:
  iPipeStream(const std::string& cmd)
      : buffer_(popen(cmd.c_str(), "r")), std::istream(&buffer_) {}
  iPipeStream(const iPipeStream&) = delete;
  iPipeStream(iPipeStream&&) = delete;
  ~iPipeStream() override {}

 private:
  FileBuffer buffer_;
};
}  // namespace potamos