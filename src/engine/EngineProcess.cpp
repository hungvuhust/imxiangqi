#include "EngineProcess.hpp"

#include <chrono>
#include <csignal>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace XiangQi {

namespace {

void closeFd(int &fd) {
  if (fd >= 0) {
    close(fd);
    fd = -1;
  }
}

bool writeAll(int fd, const char *buf, size_t len) {
  size_t off = 0;
  while (off < len) {
    ssize_t n = ::write(fd, buf + off, len - off);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    off += static_cast<size_t>(n);
  }
  return true;
}

} // namespace

EngineProcess::~EngineProcess() { stop(); }

bool EngineProcess::start(const std::string &enginePath) {
  stop();
  lastError_.clear();

  int inPipe[2]  = {-1, -1};
  int outPipe[2] = {-1, -1};
  int errPipe[2] = {-1, -1};

  if (pipe(inPipe) != 0 || pipe(outPipe) != 0 || pipe(errPipe) != 0) {
    lastError_ = std::string("pipe failed: ") + std::strerror(errno);
    closeFd(inPipe[0]);
    closeFd(inPipe[1]);
    closeFd(outPipe[0]);
    closeFd(outPipe[1]);
    closeFd(errPipe[0]);
    closeFd(errPipe[1]);
    return false;
  }

  pid_t child = fork();
  if (child < 0) {
    lastError_ = std::string("fork failed: ") + std::strerror(errno);
    closeFd(inPipe[0]);
    closeFd(inPipe[1]);
    closeFd(outPipe[0]);
    closeFd(outPipe[1]);
    closeFd(errPipe[0]);
    closeFd(errPipe[1]);
    return false;
  }

  if (child == 0) {
    dup2(inPipe[0], STDIN_FILENO);
    dup2(outPipe[1], STDOUT_FILENO);
    dup2(errPipe[1], STDERR_FILENO);

    closeFd(inPipe[0]);
    closeFd(inPipe[1]);
    closeFd(outPipe[0]);
    closeFd(outPipe[1]);
    closeFd(errPipe[0]);
    closeFd(errPipe[1]);

    execl(enginePath.c_str(), enginePath.c_str(), static_cast<char *>(nullptr));
    _exit(127);
  }

  pid_ = static_cast<int>(child);

  stdinFd_  = inPipe[1];
  stdoutFd_ = outPipe[0];
  stderrFd_ = errPipe[0];

  closeFd(inPipe[0]);
  closeFd(outPipe[1]);
  closeFd(errPipe[1]);

  running_.store(true);

  outThread_ = std::thread(&EngineProcess::readerLoop,
                           this,
                           stdoutFd_,
                           RawLine::Source::StdOut);
  errThread_ = std::thread(&EngineProcess::readerLoop,
                           this,
                           stderrFd_,
                           RawLine::Source::StdErr);

  pushLine(RawLine::Source::Sys, "process started");
  return true;
}

void EngineProcess::stop() {
  bool wasRunning = running_.exchange(false);

  if (wasRunning && stdinFd_ >= 0) {
    writeLine("quit");
  }

  if (pid_ > 0) {
    auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(1200);
    int status = 0;

    while (std::chrono::steady_clock::now() < deadline) {
      pid_t r = waitpid(pid_, &status, WNOHANG);
      if (r == pid_) {
        pid_ = -1;
        break;
      }
      if (r < 0 && errno == ECHILD) {
        pid_ = -1;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    if (pid_ > 0) {
      kill(pid_, SIGKILL);
      waitpid(pid_, &status, 0);
      pid_ = -1;
      pushLine(RawLine::Source::Sys, "process killed after timeout");
    }
  }

  closeFd(stdinFd_);
  closeFd(stdoutFd_);
  closeFd(stderrFd_);

  if (outThread_.joinable())
    outThread_.join();
  if (errThread_.joinable())
    errThread_.join();
}

bool EngineProcess::isRunning() const {
  if (!running_.load() || pid_ <= 0)
    return false;

  int   status = 0;
  pid_t r      = waitpid(pid_, &status, WNOHANG);
  return r == 0;
}

bool EngineProcess::writeLine(const std::string &line) {
  if (!running_.load() || stdinFd_ < 0)
    return false;

  std::string withNl = line;
  withNl.push_back('\n');

  if (!writeAll(stdinFd_, withNl.data(), withNl.size())) {
    lastError_ = std::string("write failed: ") + std::strerror(errno);
    return false;
  }
  return true;
}

std::vector<EngineProcess::RawLine> EngineProcess::pollLines() {
  std::vector<RawLine>        out;
  std::lock_guard<std::mutex> lk(qMu_);
  while (!q_.empty()) {
    out.push_back(std::move(q_.front()));
    q_.pop();
  }
  return out;
}

void EngineProcess::pushLine(RawLine::Source source, std::string text) {
  std::lock_guard<std::mutex> lk(qMu_);
  q_.push(RawLine{source, std::move(text)});
}

void EngineProcess::readerLoop(int fd, RawLine::Source source) {
  std::string buf;
  char        tmp[512];

  while (running_.load()) {
    ssize_t n = read(fd, tmp, sizeof(tmp));
    if (n > 0) {
      buf.append(tmp, tmp + n);

      size_t pos = 0;
      while (true) {
        size_t nl = buf.find('\n', pos);
        if (nl == std::string::npos)
          break;

        std::string line = buf.substr(pos, nl - pos);
        if (!line.empty() && line.back() == '\r')
          line.pop_back();
        if (!line.empty())
          pushLine(source, std::move(line));
        pos = nl + 1;
      }

      if (pos > 0)
        buf.erase(0, pos);
      continue;
    }

    if (n == 0)
      break;

    if (errno == EINTR)
      continue;

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    break;
  }

  if (!buf.empty())
    pushLine(source, std::move(buf));
}

} // namespace XiangQi
