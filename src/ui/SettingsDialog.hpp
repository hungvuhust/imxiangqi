#pragma once
#include "../engine/EnginePool.hpp"
#include "AppContext.hpp"
#include <string>
#include <vector>

namespace XiangQi {

// -----------------------------------------------------------------------
//  SettingsDialog  – modal popup: General | Engine | Board | Colors
// -----------------------------------------------------------------------
class SettingsDialog {
public:
  void open();
  void render(AppContext &ctx);

private:
  bool pendingOpen_   = false;
  bool confirmReset_  = false;
  int  deleteConfirm_ = -1; // engine index pending delete confirm

  // Per-engine text buffers (resized to match pool)
  struct EngBufs {
    char path[512] = {};
    char name[128] = {};
  };
  std::vector<EngBufs> bufs_;
  bool                 bufsInit_ = false;

  void syncBuffers(const EnginePool &pool);

  void tabGeneral(AppContext &ctx);
  void tabEngine(AppContext &ctx);
  void tabBoard(AppContext &ctx);
  void tabColors(AppContext &ctx);

  void renderOneEngine(int idx, AppContext &ctx);
};

} // namespace XiangQi
