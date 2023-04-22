#pragma once

#include <string>
#include <vector>

namespace tools::log {
class LogFile {
  public:
    LogFile(const std::string& name, size_t buffer_size = 1000000);
    ~LogFile();
    LogFile(const LogFile&) = delete;
    LogFile(LogFile&& other) = default;
    LogFile& operator=(const LogFile&) = delete;
    LogFile& operator=(LogFile&&) = delete;

    void Write(std::string message);
    void Flush();
    void Close() const;

  private:
    std::string file_name_;
    std::vector<char> buffer_;
    int32_t content_size_;
    int32_t file_descriptor_;
};
}  // namespace tools::log