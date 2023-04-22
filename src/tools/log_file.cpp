#include <log_file.hpp>

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <iostream>

using namespace tools;
using namespace log;

using std::string;
using std::vector;

LogFile::LogFile(const string& name, size_t buffer_size)
    : file_name_(name),
      buffer_(vector<char>(buffer_size, '#')),
      content_size_(0),
      file_descriptor_(0) {
    file_descriptor_ = open(file_name_.c_str(), O_RDWR | O_CREAT, 0664);
    if (file_descriptor_ < 0) {
        std::cerr << "Couldn't create or open file: " << file_name_
                  << " : In LogFile Init\n";
        assert(false);
    }
}

LogFile::~LogFile() {
    Flush();
    Close();
}

void LogFile::Close() const { close(file_descriptor_); }

void LogFile::Flush() {
    int32_t written_bytes = 0;
    do {
        written_bytes += write(file_descriptor_, buffer_.data() + written_bytes,
                               content_size_ - written_bytes);
    } while (written_bytes < content_size_);
    content_size_ = 0;
}

void LogFile::Write(std::string message) {
    // if message.size() > buffer.size() - RIP
    if (content_size_ + message.size() >= buffer_.size()) {
        Flush();
    }
    std::copy(message.begin(), message.end(), buffer_.begin() + content_size_);
    content_size_ += message.size();
}