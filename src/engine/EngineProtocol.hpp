#pragma once

#include "EngineTypes.hpp"
#include <optional>
#include <string>

namespace XiangQi {

class EngineProtocolParser {
public:
  static bool isUcciOk(const std::string &line);
  static bool isUciOk(const std::string &line);
  static bool isReadyOk(const std::string &line);

  static std::optional<EngineSuggestion>
  parseBestMove(const std::string &line, const EngineInfo &lastInfo);

  static bool parseInfo(const std::string &line, EngineInfo &out);
};

} // namespace XiangQi
