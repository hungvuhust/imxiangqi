#include "EngineLog.hpp"

namespace XiangQi {

void EngineLog::push(EngineLogDir dir, std::string text) {
  std::lock_guard<std::mutex> lk(mu_);
  lines_.push_back(EngineLogEntry{nextSeq_++,
                                  std::chrono::system_clock::now(),
                                  dir,
                                  std::move(text)});
  trimLocked();
}

void EngineLog::clear() {
  std::lock_guard<std::mutex> lk(mu_);
  lines_.clear();
}

void EngineLog::setMaxLines(size_t maxLines) {
  std::lock_guard<std::mutex> lk(mu_);
  maxLines_ = maxLines > 0 ? maxLines : 1;
  trimLocked();
}

std::vector<EngineLogEntry> EngineLog::snapshot() const {
  std::lock_guard<std::mutex> lk(mu_);
  return {lines_.begin(), lines_.end()};
}

void EngineLog::trimLocked() {
  while (lines_.size() > maxLines_) lines_.pop_front();
}

} // namespace XiangQi
