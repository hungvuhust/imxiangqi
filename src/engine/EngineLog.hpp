#pragma once

#include "EngineTypes.hpp"
#include <cstddef>
#include <deque>
#include <mutex>
#include <vector>

namespace XiangQi {

class EngineLog {
public:
  explicit EngineLog(size_t maxLines = 5000)
      : maxLines_(maxLines) {}

  void                        push(EngineLogDir dir, std::string text);
  void                        clear();
  void                        setMaxLines(size_t maxLines);
  std::vector<EngineLogEntry> snapshot() const;

private:
  void trimLocked();

  mutable std::mutex         mu_;
  std::deque<EngineLogEntry> lines_;
  uint64_t                   nextSeq_  = 1;
  size_t                     maxLines_ = 5000;
};

} // namespace XiangQi
