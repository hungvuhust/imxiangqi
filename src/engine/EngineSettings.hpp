#pragma once

#include <string>

namespace XiangQi {

/**
 * @brief Engine configuration (equivalent to parts of ChessProgramState in
 * xboard)
 */
struct EngineSettings {
  std::string path    = "";    // path to engine binary
  std::string name    = "";    // display name
  int         depth   = 15;    // search depth
  int         timeMs  = 3000;  // time per move (ms)
  bool        ponder  = false; // pondering on opponent's turn
  int         multiPv = 1;     // number of PV lines to show (MultiPV)

  void reset() { *this = EngineSettings{}; }
};

} // namespace XiangQi
