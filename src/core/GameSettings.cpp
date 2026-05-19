#include "GameSettings.hpp"
#include <sstream>

namespace XiangQi {

// -----------------------------------------------------------------------
//  serialize  – "key=value\n" format
//  Engine pool:
//    engineCount=N
//    engine[i].name=...  engine[i].path=...  etc.
//    redEngine=<index or -1>
//    blackEngine=<index or -1>
// -----------------------------------------------------------------------
std::string GameSettings::serialize() const {
  std::ostringstream ss;

  // Pool
  ss << "engineCount=" << pool.entries.size() << "\n";
  for (int i = 0; i < (int)pool.entries.size(); ++i) {
    const auto &e = pool.entries[i];
    ss << "engine[" << i << "].displayName=" << e.displayName << "\n";
    ss << "engine[" << i << "].path=" << e.settings.path << "\n";
    ss << "engine[" << i << "].depth=" << e.settings.depth << "\n";
    ss << "engine[" << i << "].timeMs=" << e.settings.timeMs << "\n";
    ss << "engine[" << i << "].ponder=" << (e.settings.ponder ? "1" : "0")
       << "\n";
    ss << "engine[" << i << "].multiPv=" << e.settings.multiPv << "\n";
  }
  ss << "redEngine=" << pool.redIdx << "\n";
  ss << "blackEngine=" << pool.blackIdx << "\n";

  // Board visual
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
//  deserialize
// -----------------------------------------------------------------------
GameSettings GameSettings::deserialize(const std::string &data) {
  GameSettings       s;
  std::istringstream ss(data);
  std::string        line;

  auto parseBool = [](const std::string &v) { return v == "1" || v == "true"; };
  auto parseColor = [](const std::string &v, Color4 &c) {
    float r = 0, g = 0, b = 0, a = 1;
    if (sscanf(v.c_str(), "%f,%f,%f,%f", &r, &g, &b, &a) == 4)
      c = {r, g, b, a};
  };

  // First pass: find engineCount and pre-allocate
  int engineCount = 0;
  {
    std::istringstream ss2(data);
    std::string        l2;
    while (std::getline(ss2, l2)) {
      auto eq = l2.find('=');
      if (eq == std::string::npos)
        continue;
      if (l2.substr(0, eq) == "engineCount")
        engineCount = std::stoi(l2.substr(eq + 1));
    }
  }
  // Pre-allocate entries (without starting engines)
  for (int i = 0; i < engineCount; ++i) s.pool.entries.emplace_back();

  // Second pass: fill in values
  while (std::getline(ss, line)) {
    auto eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    // Pool entries: key format "engine[i].field"
    if (key.rfind("engine[", 0) == 0) {
      auto close = key.find(']');
      if (close == std::string::npos)
        continue;
      int         idx   = std::stoi(key.substr(7, close - 7));
      std::string field = key.substr(close + 2); // skip "]."
      if (idx < 0 || idx >= (int)s.pool.entries.size())
        continue;
      auto &e = s.pool.entries[idx];
      if (field == "displayName")
        e.displayName = val;
      else if (field == "path")
        e.settings.path = val;
      else if (field == "depth")
        e.settings.depth = std::stoi(val);
      else if (field == "timeMs")
        e.settings.timeMs = std::stoi(val);
      else if (field == "ponder")
        e.settings.ponder = parseBool(val);
      else if (field == "multiPv")
        e.settings.multiPv = std::stoi(val);
      continue;
    }

    if (key == "redEngine")
      s.pool.redIdx = std::stoi(val);
    else if (key == "blackEngine")
      s.pool.blackIdx = std::stoi(val);
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

  // Clamp indices to valid range
  auto clamp = [&](int &ref) {
    if (ref < -1 || ref >= (int)s.pool.entries.size())
      ref = -1;
  };
  clamp(s.pool.redIdx);
  clamp(s.pool.blackIdx);

  return s;
}

} // namespace XiangQi
