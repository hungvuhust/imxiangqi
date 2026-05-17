#pragma once

#include "IPanel.hpp"

namespace XiangQi {

class EngineLogPanel : public IPanel {
public:
  EngineLogPanel();
  void onRender(AppContext &ctx) override;

private:
  bool autoScroll_ = true;
  bool showIn_     = true;
  bool showOut_    = true;
  bool showErr_    = true;
  bool showSys_    = true;
};

} // namespace XiangQi
