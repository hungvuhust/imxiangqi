#pragma once
#include "../engine/EngineTypes.hpp"
#include "AppContext.hpp"
#include <string>

namespace XiangQi {

// -----------------------------------------------------------------------
//  SettingsDialog  – modal popup chia tab: General | Engine | Board | Colors
//
//  Dùng:
//    dialog.open();               // gọi từ menu bar
//    dialog.render(ctx);          // gọi mỗi frame
// -----------------------------------------------------------------------
class SettingsDialog {
public:
  void open();
  void render(AppContext &ctx);

private:
  bool pendingOpen_ = false;

  // Engine tab buffers (Red)
  char pathBufRed_[512]   = {};
  char nameBufRed_[128]   = {};
  // Engine tab buffers (Black)
  char pathBufBlack_[512] = {};
  char nameBufBlack_[128] = {};
  bool bufInit_           = false;

  // Reset confirm
  bool confirmReset_ = false;

  void syncBuffers(const GameSettings &s);
  bool browseEnginePath(EngineSettings &s, char *pathBuf, size_t pathBufSize);

  void tabGeneral(AppContext &ctx);
  void tabEngine(AppContext &ctx);
  void tabBoard(AppContext &ctx);
  void tabColors(AppContext &ctx);

  // Helper: render settings for one engine side
  void renderEngineSettings(const char       *label,
                            EngineSettings   &s,
                            EngineController &eng,
                            char             *pathBuf,
                            size_t            pathBufSize,
                            char             *nameBuf,
                            size_t            nameBufSize);
};

} // namespace XiangQi
