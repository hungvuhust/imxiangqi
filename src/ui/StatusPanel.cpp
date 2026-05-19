#include "StatusPanel.hpp"
#include "../core/Board.hpp"
#include "../core/MoveNotation.hpp"
#include "../engine/EnginePool.hpp"
#include <imgui.h>
#include <sstream>
#include <vector>

namespace XiangQi {

StatusPanel::StatusPanel()
    : IPanel("Status") {}

void StatusPanel::onRender(AppContext &ctx) {
  if (!open_)
    return;

  if (!ImGui::Begin(title_.c_str(), &open_)) {
    ImGui::End();
    return;
  }

  const GameState &gs = ctx.gameState;

  // --- Game result --------------------------------------------------------
  switch (gs.status()) {
  case GameStatus::RedWins:
    ImGui::TextColored({1.f, 0.35f, 0.1f, 1.f}, "Red (South) wins!");
    break;
  case GameStatus::BlackWins:
    ImGui::TextColored({0.5f, 0.65f, 1.f, 1.f}, "Black (North) wins!");
    break;
  case GameStatus::Draw:
    ImGui::TextColored({0.8f, 0.8f, 0.2f, 1.f}, "Draw");
    break;
  case GameStatus::Playing: {
    // Side to move indicator
    bool   isRed = (gs.sideToMove() == PieceColor::Red);
    ImVec4 col =
        isRed ? ImVec4{1.f, 0.4f, 0.2f, 1.f} : ImVec4{0.55f, 0.7f, 1.f, 1.f};
    ImGui::TextColored(col, "%s to move", isRed ? "Red" : "Black");

    if (gs.board().isInCheck())
      ImGui::TextColored({1.f, 0.15f, 0.1f, 1.f}, "  CHECK!");
    break;
  }
  }

  ImGui::Separator();

  // --- Move counter -------------------------------------------------------
  int full = gs.board().fullMoveNum();
  int half = gs.board().halfMoveClock();
  ImGui::Text("Move: %d  |  Half-clock: %d", full, half);

  // --- Material count (quick display) ------------------------------------
  ImGui::Separator();
  ImGui::Text("Pieces on board:");

  int redCount = 0, blackCount = 0;
  for (int r = 0; r < Board::ROWS; ++r)
    for (int c = 0; c < Board::COLS; ++c) {
      const Piece &p = gs.board().at(r, c);
      if (!p.empty()) {
        if (p.color() == PieceColor::Red)
          ++redCount;
        else
          ++blackCount;
      }
    }
  ImGui::TextColored({1.f, 0.45f, 0.2f, 1.f}, "  Red:   %d", redCount);
  ImGui::TextColored({0.55f, 0.7f, 1.f, 1.f}, "  Black: %d", blackCount);

  // --- Engine analysis (PV) -----------------------------------------------
  EnginePool                  &pool = ctx.settings.pool;
  EngineController            *engR = pool.redEngine();
  EngineController            *engB = pool.blackEngine();
  static const AnalyzeSnapshot kEmpty{};
  const AnalyzeSnapshot       &snapR = engR ? engR->analyzeSnapshot() : kEmpty;
  const AnalyzeSnapshot       &snapB = engB ? engB->analyzeSnapshot() : kEmpty;
  const AnalyzeSnapshot       &snap  = snapR.pvLines.empty() ? snapB : snapR;
  bool                         isAnalyzing =
      (engR && engR->isAnalyzing()) || (engB && engB->isAnalyzing());
  if (isAnalyzing || !snap.pvLines.empty()) {
    ImGui::Separator();
    if (snap.depth >= 0)
      ImGui::Text("Độ sâu: %d", snap.depth);

    // Rebuild board tại trạng thái hiện tại
    Board pvBoard;
    for (const auto &m : ctx.gameState.history()) pvBoard.applyMove(m);

    bool useFont = ctx.fontVN != nullptr;

    for (const auto &pv : snap.pvLines) {
      ImGui::PushID(pv.multipv);
      ImGui::Separator();

      // Header: "PV 1" + score
      if (snap.pvLines.size() > 1)
        ImGui::Text("PV %d", pv.multipv);

      if (pv.hasMate) {
        if (snap.pvLines.size() > 1)
          ImGui::SameLine();
        ImGui::TextColored({1.f, 0.6f, 0.2f, 1.f},
                           "Chiếu hết trong %d",
                           pv.mate);
      } else if (pv.hasScoreCp) {
        float  s        = pv.scoreCp / 100.0f;
        ImVec4 scoreCol = s >= 0 ? ImVec4{0.4f, 1.f, 0.5f, 1.f}
                                 : ImVec4{1.f, 0.4f, 0.4f, 1.f};
        if (snap.pvLines.size() > 1)
          ImGui::SameLine();
        ImGui::TextColored(scoreCol, "%+.2f", s);
      }

      // Parse tối đa 6 nước đầu, hiển thị notation VN
      if (!pv.pv.empty()) {
        std::istringstream       ss(pv.pv);
        std::string              tok;
        Board                    tmpBoard = pvBoard;
        std::vector<std::string> labels;
        labels.reserve(6);
        while (labels.size() < 6 && ss >> tok) {
          if (tok.size() < 4)
            break;
          Square from = Square::fromString(tok.substr(0, 2));
          Square to   = Square::fromString(tok.substr(2, 2));
          if (!from.valid() || !to.valid())
            break;
          Piece moved    = tmpBoard.at(from);
          Piece captured = tmpBoard.at(to);
          Move  mv{from, to, moved, captured};
          labels.push_back(moveToVN(mv, tmpBoard));
          tmpBoard.applyMove(mv);
        }

        // 2 hàng × 3 cột:
        //   hàng trên = nước 1,3,5 (index 0,2,4) — bên đang đi trước
        //   hàng dưới = nước 2,4,6 (index 1,3,5) — bên đối thủ
        ImVec4 colRow0 = snap.redToMove ? ImVec4{1.f, 0.55f, 0.3f, 1.f} // đỏ
                                        : ImVec4{0.6f, 0.8f, 1.f, 1.f}; // đen
        ImVec4 colRow1 = snap.redToMove ? ImVec4{0.6f, 0.8f, 1.f, 1.f}  // đen
                                        : ImVec4{1.f, 0.55f, 0.3f, 1.f}; // đỏ

        if (useFont)
          ImGui::PushFont(ctx.fontVN);
        if (ImGui::BeginTable("##pv", 3, ImGuiTableFlags_None)) {
          ImGui::TableNextRow();
          for (int col = 0; col < 3; ++col) {
            ImGui::TableSetColumnIndex(col);
            int i = col * 2; // index 0,2,4
            if (i < static_cast<int>(labels.size()))
              ImGui::TextColored(colRow0, "%s", labels[i].c_str());
          }
          ImGui::TableNextRow();
          for (int col = 0; col < 3; ++col) {
            ImGui::TableSetColumnIndex(col);
            int i = col * 2 + 1; // index 1,3,5
            if (i < static_cast<int>(labels.size()))
              ImGui::TextColored(colRow1, "%s", labels[i].c_str());
          }
          ImGui::EndTable();
        }
        if (useFont)
          ImGui::PopFont();
      }

      ImGui::PopID();
    }
  }

  ImGui::End();
}

} // namespace XiangQi
