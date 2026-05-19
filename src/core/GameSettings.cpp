#include "GameSettings.hpp"
#include <sstream>

namespace XiangQi {

// -----------------------------------------------------------------------
//  serialize  – produce a simple "key=value\n" string
// -----------------------------------------------------------------------
std::string GameSettings::serialize() const {
  std::ostringstream ss;

  ss << "redPlayer=" << (redPlayer == PlayerMode::Engine ? "engine" : "human")
     << "\n";
  ss << "blackPlayer="
     << (blackPlayer == PlayerMode::Engine ? "engine" : "human") << "\n";
  ss << "engineRedPath=" << engineRed.path << "\n";
  ss << "engineRedName=" << engineRed.name << "\n";
  ss << "engineRedDepth=" << engineRed.depth << "\n";
  ss << "engineRedTimeMs=" << engineRed.timeMs << "\n";
  ss << "engineRedPonder=" << (engineRed.ponder ? "1" : "0") << "\n";
  ss << "engineRedMultiPv=" << engineRed.multiPv << "\n";
  ss << "engineBlackPath=" << engineBlack.path << "\n";
  ss << "engineBlackName=" << engineBlack.name << "\n";
  ss << "engineBlackDepth=" << engineBlack.depth << "\n";
  ss << "engineBlackTimeMs=" << engineBlack.timeMs << "\n";
  ss << "engineBlackPonder=" << (engineBlack.ponder ? "1" : "0") << "\n";
  ss << "engineBlackMultiPv=" << engineBlack.multiPv << "\n";
  ss << "theme=" << (theme == BoardTheme::Simple ? "simple" : "classic")
     << "\n";
  ss << "pieceScale=" << pieceScale << "\n";
  ss << "flipBoard=" << (flipBoard ? "1" : "0") << "\n";
  ss << "showCoords=" << (showCoords ? "1" : "0") << "\n";
  ss << "animateMoves=" << (animateMoves ? "1" : "0") << "\n";
  ss << "animSpeedMs=" << animSpeedMs << "\n";

  auto writeColor = [&](const char *key, const Color4 &c) {
    ss << key << "=" << c.r << "," << c.g << "," << c.b << "," << c.a << "\n";
  };
  writeColor("colSelected", colSelected);
  writeColor("colLegal", colLegal);
  writeColor("colLastMove", colLastMove);
  writeColor("colCheck", colCheck);

  return ss.str();
}

// -----------------------------------------------------------------------
//  deserialize  – parse "key=value\n" string back into GameSettings
// -----------------------------------------------------------------------
GameSettings GameSettings::deserialize(const std::string &data) {
  GameSettings       s;
  std::istringstream ss(data);
  std::string        line;

  auto parseBool = [](const std::string &v) { return v == "1" || v == "true"; };
  auto parseColor = [](const std::string &v, Color4 &c) {
    // format: "r,g,b,a"
    float r = 0, g = 0, b = 0, a = 1;
    if (sscanf(v.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a) == 4)
      c = {r, g, b, a};
  };

  while (std::getline(ss, line)) {
    auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    if (key == "redPlayer")
      s.redPlayer = (val == "engine") ? PlayerMode::Engine : PlayerMode::Human;
    else if (key == "blackPlayer")
      s.blackPlayer =
          (val == "engine") ? PlayerMode::Engine : PlayerMode::Human;
    // Red engine settings (new keys; legacy "engineXxx" also maps to Red)
    else if (key == "engineRedPath" || key == "enginePath")
      s.engineRed.path = val;
    else if (key == "engineRedName" || key == "engineName")
      s.engineRed.name = val;
    else if (key == "engineRedDepth" || key == "engineDepth")
      s.engineRed.depth = std::stoi(val);
    else if (key == "engineRedTimeMs" || key == "engineTimeMs")
      s.engineRed.timeMs = std::stoi(val);
    else if (key == "engineRedPonder" || key == "enginePonder")
      s.engineRed.ponder = parseBool(val);
    else if (key == "engineRedMultiPv" || key == "engineMultiPv")
      s.engineRed.multiPv = std::stoi(val);
    // Black engine settings
    else if (key == "engineBlackPath")
      s.engineBlack.path = val;
    else if (key == "engineBlackName")
      s.engineBlack.name = val;
    else if (key == "engineBlackDepth")
      s.engineBlack.depth = std::stoi(val);
    else if (key == "engineBlackTimeMs")
      s.engineBlack.timeMs = std::stoi(val);
    else if (key == "engineBlackPonder")
      s.engineBlack.ponder = parseBool(val);
    else if (key == "engineBlackMultiPv")
      s.engineBlack.multiPv = std::stoi(val);
    else if (key == "theme")
      s.theme = (val == "simple") ? BoardTheme::Simple : BoardTheme::Classic;
    else if (key == "pieceScale")
      s.pieceScale = std::stof(val);
    else if (key == "flipBoard")
      s.flipBoard = parseBool(val);
    else if (key == "showCoords")
      s.showCoords = parseBool(val);
    else if (key == "animateMoves")
      s.animateMoves = parseBool(val);
    else if (key == "animSpeedMs")
      s.animSpeedMs = std::stof(val);
    else if (key == "colSelected")
      parseColor(val, s.colSelected);
    else if (key == "colLegal")
      parseColor(val, s.colLegal);
    else if (key == "colLastMove")
      parseColor(val, s.colLastMove);
    else if (key == "colCheck")
      parseColor(val, s.colCheck);
  }
  return s;
}

} // namespace XiangQi
