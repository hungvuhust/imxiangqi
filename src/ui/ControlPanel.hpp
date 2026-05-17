#pragma once
#include "IPanel.hpp"

namespace XiangQi {

// -----------------------------------------------------------------------
//  ControlPanel  – New Game / Undo / Flip / Load FEN buttons
// -----------------------------------------------------------------------
class ControlPanel : public IPanel {
public:
  ControlPanel();
  void onRender(AppContext &ctx) override;

private:
  // FEN input buffer
  char fenBuf_[256] = {};
  bool fenError_    = false;
};

} // namespace XiangQi
