#pragma once

#include "../engine/EngineController.hpp"
#include "IPanel.hpp"

namespace XiangQi {

class EngineLogPanel : public IPanel {
public:
  EngineLogPanel();
  void onRender(AppContext &ctx) override;

private:
  bool              autoScroll_ = true;
  bool              showIn_     = true;
  bool              showOut_    = true;
  bool              showErr_    = true;
  bool              showSys_    = true;
  EngineController *activeLog_  = nullptr; // tab đang xem
};

} // namespace XiangQi
