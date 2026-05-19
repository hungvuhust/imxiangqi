#include "MoveHistoryPanel.hpp"
#include "../core/Board.hpp"
#include "../core/MoveNotation.hpp"
#include <imgui.h>
#include <string>
#include <vector>

namespace XiangQi {

MoveHistoryPanel::MoveHistoryPanel()
    : IPanel("Move History") {}

void MoveHistoryPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  const auto &hist = ctx.gameState.history();

  // --- Toolbar ------------------------------------------------------------
  ImGui::Checkbox("Auto-scroll", &autoScroll_);
  ImGui::SameLine();
  ImGui::TextDisabled("(%zu moves)", hist.size());

  ImGui::Separator();

  // Pre-compute VN notation: replay from start to get boardBefore each move
  std::vector<std::string> labels;
  labels.reserve(hist.size());
  {
    Board tmp; // starts at initial position
    for (const Move &m : hist) {
      labels.push_back(moveToVN(m, tmp));
      tmp.applyMove(m);
    }
  }

  // --- Move table ---------------------------------------------------------
  if (ImGui::BeginTable("##moves",
                        3,
                        ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_BordersInnerV)) {
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 28.f);
    ImGui::TableSetupColumn("Đỏ", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Đen", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableHeadersRow();

    int rows = static_cast<int>((hist.size() + 1) / 2);
    for (int row = 0; row < rows; ++row) {
      ImGui::TableNextRow();

      // Column 0: move number
      ImGui::TableSetColumnIndex(0);
      ImGui::TextDisabled("%d", row + 1);

      // Column 1: Red's move (index = row*2)
      ImGui::TableSetColumnIndex(1);
      int redIdx = row * 2;
      if (redIdx < static_cast<int>(hist.size())) {
        const std::string &nm  = labels[redIdx];
        bool               sel = (selectedIndex_ == redIdx);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{1.f, 0.45f, 0.2f, 1.f});
        if (ImGui::Selectable(nm.c_str(),
                              sel,
                              ImGuiSelectableFlags_SpanAllColumns))
          selectedIndex_ = redIdx;
        ImGui::PopStyleColor();
      }

      // Column 2: Black's move (index = row*2 + 1)
      ImGui::TableSetColumnIndex(2);
      int blkIdx = row * 2 + 1;
      if (blkIdx < static_cast<int>(hist.size())) {
        const std::string &nm  = labels[blkIdx];
        bool               sel = (selectedIndex_ == blkIdx);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{0.55f, 0.7f, 1.f, 1.f});
        if (ImGui::Selectable(nm.c_str(),
                              sel,
                              ImGuiSelectableFlags_SpanAllColumns))
          selectedIndex_ = blkIdx;
        ImGui::PopStyleColor();
      }
    }

    if (autoScroll_ && !hist.empty())
      ImGui::SetScrollHereY(1.0f);

    ImGui::EndTable();
  }

  ImGui::End();
}

} // namespace XiangQi
