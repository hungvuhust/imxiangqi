#pragma once
#include "IPanel.hpp"

namespace XiangQi {

// -----------------------------------------------------------------------
//  MoveHistoryPanel  – scrollable move list in UCCI notation
// -----------------------------------------------------------------------
class MoveHistoryPanel : public IPanel {
public:
  MoveHistoryPanel();
  void onRender(AppContext &ctx) override;

private:
  bool autoScroll_    = true;
  int  selectedIndex_ = -1; // future: click to jump to position
};

} // namespace XiangQi
