#include "EngineProtocol.hpp"

#include <algorithm>
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

    if (token == "multipv") {
      int n = 1;
      if (ss >> n)
        out.multipv = n;
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

// ---------------------------------------------------------------------------
// parseOption
// Handles: "option name X type check default true/false"
//          "option name X type spin default N min M max K"
//          "option name X type combo default V var A var B ..."
//          "option name X type button"
//          "option name X type string default S"
// ---------------------------------------------------------------------------
std::optional<EngineOption>
EngineProtocolParser::parseOption(const std::string &line) {
  // Must start with "option"
  if (line.rfind("option", 0) != 0)
    return std::nullopt;

  EngineOption opt;

  // Extract "name <N>" — name extends until "type" keyword
  auto namePos = line.find(" name ");
  auto typePos = line.find(" type ");
  if (namePos == std::string::npos || typePos == std::string::npos)
    return std::nullopt;
  if (typePos <= namePos)
    return std::nullopt;

  opt.name = line.substr(namePos + 6, typePos - namePos - 6);

  // Parse remainder after "type"
  std::istringstream ss(line.substr(typePos + 6));
  std::string        typeStr;
  ss >> typeStr;

  if (typeStr == "check") {
    opt.type = EngineOptionType::Check;
    std::string tok;
    while (ss >> tok) {
      if (tok == "default") {
        std::string val;
        if (ss >> val)
          opt.checkVal = (val == "true" || val == "1");
      }
    }
  } else if (typeStr == "spin") {
    opt.type = EngineOptionType::Spin;
    std::string tok;
    while (ss >> tok) {
      if (tok == "default") { ss >> opt.spinVal; }
      else if (tok == "min") { ss >> opt.spinMin; }
      else if (tok == "max") { ss >> opt.spinMax; }
    }
  } else if (typeStr == "combo") {
    opt.type = EngineOptionType::Combo;
    std::string tok;
    bool inDefault = false, inVar = false;
    std::string varAccum;
    while (ss >> tok) {
      if (tok == "default") { inDefault = true; inVar = false; continue; }
      if (tok == "var")     { inVar = true; inDefault = false;
                              if (!varAccum.empty()) { opt.comboVars.push_back(varAccum); varAccum.clear(); }
                              continue; }
      if (inDefault) { opt.strVal = tok; inDefault = false; continue; }
      if (inVar) {
        if (!varAccum.empty()) varAccum += ' ';
        varAccum += tok;
      }
    }
    if (!varAccum.empty()) opt.comboVars.push_back(varAccum);
  } else if (typeStr == "button") {
    opt.type = EngineOptionType::Button;
  } else {
    // string (and unknown types)
    opt.type = EngineOptionType::String;
    std::string tok;
    bool inDefault = false;
    while (ss >> tok) {
      if (tok == "default") { inDefault = true; opt.strVal.clear(); continue; }
      if (inDefault) {
        if (!opt.strVal.empty()) opt.strVal += ' ';
        opt.strVal += tok;
      }
    }
  }

  return opt;
}

// ---------------------------------------------------------------------------
// parseIdName  – returns engine name from "id name <X>", else empty string
// ---------------------------------------------------------------------------
std::string EngineProtocolParser::parseIdName(const std::string &line) {
  const std::string prefix = "id name ";
  if (line.rfind(prefix, 0) == 0)
    return line.substr(prefix.size());
  return {};
}

// ---------------------------------------------------------------------------
// buildSetOption
// ---------------------------------------------------------------------------
std::string EngineProtocolParser::buildSetOption(const EngineOption &opt) {
  std::string cmd = "setoption name " + opt.name;
  switch (opt.type) {
  case EngineOptionType::Check:
    cmd += " value ";
    cmd += opt.checkVal ? "true" : "false";
    break;
  case EngineOptionType::Spin:
    cmd += " value " + std::to_string(opt.spinVal);
    break;
  case EngineOptionType::Combo:
  case EngineOptionType::String:
    if (!opt.strVal.empty())
      cmd += " value " + opt.strVal;
    break;
  case EngineOptionType::Button:
    // no value for button
    break;
  }
  return cmd;
}

} // namespace XiangQi
