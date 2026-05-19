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

  // Parse "option name Hash type spin default 16 min 1 max 4096"
  static std::optional<EngineOption> parseOption(const std::string &line);

  // Parse "id name Pikafish" -> name field; "id author ..." -> ignored
  static std::string parseIdName(const std::string &line);

  // Build "setoption name X value Y" string for an option
  static std::string buildSetOption(const EngineOption &opt);
};

} // namespace XiangQi
