#pragma once

#include "IPanel.hpp"

namespace XiangQi {

class EngineSettingsPanel : public IPanel {
public:
  EngineSettingsPanel();

  void onRender(AppContext &ctx) override;

private:
  void syncBuffers(const EngineSettings &s);
  bool browseEnginePath(EngineSettings &s);

  char pathBuf_[512] = "";
  char nameBuf_[128] = "";
  bool bufInit_      = false;
};

} // namespace XiangQi
