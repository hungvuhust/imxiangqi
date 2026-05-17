#pragma once
#include "IPanel.hpp"

namespace XiangQi {

// -----------------------------------------------------------------------
//  StatusPanel  – displays current turn, check warning, game result
// -----------------------------------------------------------------------
class StatusPanel : public IPanel {
public:
  StatusPanel();
  void onRender(AppContext &ctx) override;
};

} // namespace XiangQi
