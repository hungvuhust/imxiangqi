#include "GameState.hpp"

namespace XiangQi {

// =======================================================================
//  Construction
// =======================================================================
GameState::GameState() { newGame(); }

GameState::GameState(const std::string &fen) {
  board_.loadFen(fen);
  status_ = GameStatus::Playing;
}

void GameState::newGame() {
  board_.reset();
  status_ = GameStatus::Playing;
  selection_.clear();
}

void GameState::loadFen(const std::string &fen) {
  board_.loadFen(fen);
  status_ = GameStatus::Playing;
  selection_.clear();
  updateStatus();
}

// =======================================================================
//  Status
// =======================================================================
void GameState::updateStatus() {
  if (board_.isCheckmate()) {
    // The side that cannot move has lost
    status_ = (board_.sideToMove() == PieceColor::Red) ? GameStatus::BlackWins
                                                       : GameStatus::RedWins;
    if (onStatusChanged_)
      onStatusChanged_(status_);
  } else if (board_.isStalemate()) {
    // Xiangqi: stalemate = the stalemated side loses
    status_ = (board_.sideToMove() == PieceColor::Red) ? GameStatus::BlackWins
                                                       : GameStatus::RedWins;
    if (onStatusChanged_)
      onStatusChanged_(status_);
  }
}

// =======================================================================
//  isLegalDest
// =======================================================================
bool GameState::isLegalDest(Square sq) const {
  for (const auto &m : selection_.legalDests)
    if (m.to() == sq)
      return true;
  return false;
}

// =======================================================================
//  onSquareClicked  – main interaction entry point
// =======================================================================
bool GameState::onSquareClicked(Square sq) {
  if (!isPlaying())
    return false;

  const Piece &clicked = board_.at(sq);

  // --- Phase: nothing selected yet ---
  if (selection_.phase == SelectionPhase::None) {
    // Must click on a friendly piece
    if (clicked.empty() || clicked.color() != board_.sideToMove())
      return false;

    selection_.phase      = SelectionPhase::PieceSelected;
    selection_.square     = sq;
    selection_.legalDests = board_.legalMovesFor(sq);
    return false; // no move made, just selection updated
  }

  // --- Phase: a piece is already selected ---
  // Click on the same square → deselect
  if (sq == selection_.square) {
    selection_.clear();
    return false;
  }

  // Click on another friendly piece → switch selection
  if (!clicked.empty() && clicked.color() == board_.sideToMove()) {
    selection_.phase      = SelectionPhase::PieceSelected;
    selection_.square     = sq;
    selection_.legalDests = board_.legalMovesFor(sq);
    return false;
  }

  // Otherwise: try to move
  return tryMoveToSquare(sq);
}

// =======================================================================
//  tryMoveToSquare
// =======================================================================
bool GameState::tryMoveToSquare(Square dest) {
  // Find matching legal move
  for (const auto &m : selection_.legalDests) {
    if (m.to() == dest) {
      selection_.clear();
      bool ok = board_.applyMove(m);
      if (ok) {
        if (onMoveMade_)
          onMoveMade_(m);
        updateStatus();
      }
      return ok;
    }
  }
  // Destination not legal — clear selection
  selection_.clear();
  return false;
}

// =======================================================================
//  applyMove  (engine / external)
// =======================================================================
bool GameState::applyMove(const Move &m) {
  if (!isPlaying())
    return false;
  selection_.clear();
  bool ok = board_.applyMove(m);
  if (ok) {
    if (onMoveMade_)
      onMoveMade_(m);
    updateStatus();
  }
  return ok;
}

// =======================================================================
//  undoMove
// =======================================================================
bool GameState::undoMove() {
  if (!board_.canUndo())
    return false;
  selection_.clear();
  board_.undoMove();
  status_ = GameStatus::Playing; // re-open game after undo
  return true;
}

} // namespace XiangQi
