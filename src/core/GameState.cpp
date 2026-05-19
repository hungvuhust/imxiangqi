#include "GameState.hpp"
#include <algorithm>

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
  emit({GameEventKind::GameReset, std::nullopt, status_});
}

void GameState::loadFen(const std::string &fen) {
  board_.loadFen(fen);
  status_ = GameStatus::Playing;
  selection_.clear();
  emit({GameEventKind::FenLoaded, std::nullopt, status_});
  updateStatus();
}

// =======================================================================
//  Event system
// =======================================================================
int GameState::addGameEventListener(GameEventListener listener) {
  int id = nextListenerId_++;
  listeners_.push_back({id, std::move(listener)});
  return id;
}

void GameState::removeGameEventListener(int id) {
  listeners_.erase(
      std::remove_if(listeners_.begin(),
                     listeners_.end(),
                     [id](const ListenerEntry &e) { return e.id == id; }),
      listeners_.end());
}

void GameState::emit(const GameEvent &event) const {
  for (const auto &entry : listeners_) entry.fn(event);
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
    emit({GameEventKind::GameOver, std::nullopt, status_});
  } else if (board_.isStalemate()) {
    // Xiangqi: stalemate = the stalemated side loses
    status_ = (board_.sideToMove() == PieceColor::Red) ? GameStatus::BlackWins
                                                       : GameStatus::RedWins;
    if (onStatusChanged_)
      onStatusChanged_(status_);
    emit({GameEventKind::GameOver, std::nullopt, status_});
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
    if (m.to() == dest)
      return applyMove(m); // delegates to applyMove → emits event
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
    emit({GameEventKind::MoveMade, m, status_});
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
  emit({GameEventKind::MoveUndone, std::nullopt, status_});
  return true;
}

} // namespace XiangQi
