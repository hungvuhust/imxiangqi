#include "EngineProtocol.hpp"

#include <sstream>

namespace XiangQi {

bool EngineProtocolParser::isUcciOk(const std::string &line) {
  return line.find("ucciok") != std::string::npos;
}

bool EngineProtocolParser::isUciOk(const std::string &line) {
  return line.find("uciok") != std::string::npos;
}

bool EngineProtocolParser::isReadyOk(const std::string &line) {
  return line.find("readyok") != std::string::npos;
}

std::optional<EngineSuggestion>
EngineProtocolParser::parseBestMove(const std::string &line,
                                    const EngineInfo  &lastInfo) {
  if (line.rfind("bestmove", 0) != 0)
    return std::nullopt;

  std::istringstream ss(line);
  std::string        token;
  std::string        move;
  ss >> token >> move;
  if (move.empty() || move == "(none)")
    return std::nullopt;

  EngineSuggestion out;
  out.moveUcci = move;
  out.info     = lastInfo;
  return out;
}

bool EngineProtocolParser::parseInfo(const std::string &line, EngineInfo &out) {
  if (line.rfind("info", 0) != 0)
    return false;

  std::istringstream ss(line);
  std::string        token;
  ss >> token; // info

  bool        inPv = false;
  std::string pvAccum;

  while (ss >> token) {
    if (inPv) {
      if (!pvAccum.empty())
        pvAccum += ' ';
      pvAccum += token;
      continue;
    }

    if (token == "depth") {
      int d = -1;
      if (ss >> d)
        out.depth = d;
      continue;
    }

    if (token == "score") {
      std::string kind;
      if (!(ss >> kind))
        continue;
      if (kind == "cp") {
        int cp = 0;
        if (ss >> cp) {
          out.hasScoreCp = true;
          out.scoreCp    = cp;
        }
      } else if (kind == "mate") {
        int mate = 0;
        if (ss >> mate) {
          out.hasMate = true;
          out.mate    = mate;
        }
      }
      continue;
    }

    if (token == "pv") {
      inPv = true;
      continue;
    }
  }

  if (!pvAccum.empty())
    out.pv = std::move(pvAccum);

  return true;
}

} // namespace XiangQi
