#pragma once

#include <atomic>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace XiangQi {

class EngineProcess {
public:
  struct RawLine {
    enum class Source : uint8_t {
      StdOut,
      StdErr,
      Sys,
    };
    Source      source = Source::Sys;
    std::string text;
  };

  EngineProcess() = default;
  ~EngineProcess();

  EngineProcess(const EngineProcess &)            = delete;
  EngineProcess &operator=(const EngineProcess &) = delete;

  bool start(const std::string &enginePath);
  void stop();
  bool isRunning() const;

  bool                 writeLine(const std::string &line);
  std::vector<RawLine> pollLines();

  const std::string &lastError() const { return lastError_; }

private:
  void pushLine(RawLine::Source source, std::string text);
  void readerLoop(int fd, RawLine::Source source);

  int pid_      = -1;
  int stdinFd_  = -1;
  int stdoutFd_ = -1;
  int stderrFd_ = -1;

  std::thread outThread_;
  std::thread errThread_;

  std::atomic<bool> running_{false};

  mutable std::mutex  qMu_;
  std::queue<RawLine> q_;
  std::string         lastError_;
};

} // namespace XiangQi
