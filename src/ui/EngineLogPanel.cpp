#include "EngineLogPanel.hpp"
#include "../engine/EnginePool.hpp"

#include <imgui.h>
#include <string>

namespace XiangQi {

EngineLogPanel::EngineLogPanel()
    : IPanel("Engine Log") {}

void EngineLogPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  // Chọn engine để xem log – dùng tất cả engine trong pool
  EnginePool &pool = ctx.settings.pool;
  if (ImGui::BeginTabBar("##eng-log-tabs")) {
    for (int i = 0; i < (int)pool.entries.size(); ++i) {
      auto       &e     = pool.entries[i];
      std::string label = e.displayName.empty()
                              ? ("Engine " + std::to_string(i))
                              : e.displayName;
      // Đánh dấu phía được giao
      if (pool.redIdx == i)
        label += " [R]";
      if (pool.blackIdx == i)
        label += " [B]";
      if (ImGui::BeginTabItem(label.c_str())) {
        activeLog_ = e.ctrl.get();
        ImGui::EndTabItem();
      }
    }
    ImGui::EndTabBar();
  }
  // Fallback: nếu chưa chọn hoặc pool rỗng
  if (!activeLog_ && !pool.entries.empty())
    activeLog_ = pool.entries[0].ctrl.get();

  if (!activeLog_) {
    ImGui::TextDisabled("No engines configured.");
    ImGui::End();
    return;
  }

  if (ImGui::Button("Clear"))
    activeLog_->log().clear();
  ImGui::SameLine();
  ImGui::Checkbox("Auto-scroll", &autoScroll_);

  ImGui::Separator();
  ImGui::Checkbox("IN", &showIn_);
  ImGui::SameLine();
  ImGui::Checkbox("OUT", &showOut_);
  ImGui::SameLine();
  ImGui::Checkbox("ERR", &showErr_);
  ImGui::SameLine();
  ImGui::Checkbox("SYS", &showSys_);

  ImGui::Separator();

  auto lines = activeLog_->log().snapshot();

  ImGui::BeginChild("##engine-log-scroll",
                    ImVec2(0, 0),
                    true,
                    ImGuiWindowFlags_HorizontalScrollbar);
  ImGuiListClipper clipper;
  clipper.Begin(static_cast<int>(lines.size()));

  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
      const auto &e = lines[static_cast<size_t>(i)];

      bool show = (e.dir == EngineLogDir::In && showIn_) ||
                  (e.dir == EngineLogDir::Out && showOut_) ||
                  (e.dir == EngineLogDir::Err && showErr_) ||
                  (e.dir == EngineLogDir::Sys && showSys_);
      if (!show)
        continue;

      ImVec4 col = {0.85f, 0.85f, 0.85f, 1.f};
      if (e.dir == EngineLogDir::Err)
        col = {1.f, 0.45f, 0.45f, 1.f};
      else if (e.dir == EngineLogDir::In)
        col = {0.55f, 0.85f, 1.f, 1.f};
      else if (e.dir == EngineLogDir::Sys)
        col = {0.7f, 1.f, 0.7f, 1.f};

      ImGui::TextColored(col, "[%s] %s", toString(e.dir), e.text.c_str());
    }
  }

  if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 2.0f)
    ImGui::SetScrollHereY(1.0f);

  ImGui::EndChild();
  ImGui::End();
}

} // namespace XiangQi
