#pragma once

#include "IPanel.hpp"

namespace XiangQi {

class SettingsPanel : public IPanel {
public:
  SettingsPanel();
  void onRender(AppContext &ctx) override;

private:
  bool confirmReset_ = false;

  void renderVisualSection(GameSettings &s);
  void renderHighlightSection(GameSettings &s);
};

} // namespace XiangQi
