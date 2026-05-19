#pragma once
#include "Board.hpp"
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace XiangQi {

// -----------------------------------------------------------------------
//  GameStatus  – high-level outcome
// -----------------------------------------------------------------------
enum class GameStatus {
  Playing,
  RedWins,
  BlackWins,
  Draw, // reserved for future 60-move rule / repetition
};

// -----------------------------------------------------------------------
//  GameEvent  – discrete state-change notifications
//
//  Emitted by GameState after the change is fully applied.
//  Subscribers must not mutate GameState inside a listener.
// -----------------------------------------------------------------------
enum class GameEventKind {
  MoveMade,      // a move was applied (human or engine)
  MoveUndone,    // last move was taken back
  GameReset,     // newGame() called
  FenLoaded,     // loadFen() called
  GameOver,      // checkmate / stalemate reached
};

struct GameEvent {
  GameEventKind      kind;
  std::optional<Move> move;       // set for MoveMade
  GameStatus          status{};   // set for GameOver
};

using GameEventListener = std::function<void(const GameEvent &)>;

// -----------------------------------------------------------------------
//  SelectionState  – what the user currently has selected
// -----------------------------------------------------------------------
enum class SelectionPhase {
  None,          // nothing selected
  PieceSelected, // a piece is highlighted, waiting for destination
};

struct Selection {
  SelectionPhase    phase = SelectionPhase::None;
  Square            square{};     // the selected piece's square
  std::vector<Move> legalDests{}; // pre-computed legal destinations

  void clear() {
    phase  = SelectionPhase::None;
    square = {};
    legalDests.clear();
  }
};

// -----------------------------------------------------------------------
//  GameState  – orchestrates Board, turn management, and UI selection
//
//  This is the central model that the renderer and input handler talk to.
//  The engine adapter (future) will also call applyEngineMove().
// -----------------------------------------------------------------------
class GameState {
public:
  // Callback types
  using MoveCallback   = std::function<void(const Move &)>;
  using StatusCallback = std::function<void(GameStatus)>;

  // ------------------------------------------------------------------
  //  Construction / lifecycle
  // ------------------------------------------------------------------
  GameState();
  explicit GameState(const std::string &fen);

  void        newGame();
  void        loadFen(const std::string &fen);
  std::string toFen() const { return board_.toFen(); }

  // ------------------------------------------------------------------
  //  Board access (read-only for renderer)
  // ------------------------------------------------------------------
  const Board &board() const { return board_; }
  PieceColor   sideToMove() const { return board_.sideToMove(); }
  GameStatus   status() const { return status_; }

  bool isPlaying() const { return status_ == GameStatus::Playing; }
  bool isGameOver() const { return status_ != GameStatus::Playing; }

  // ------------------------------------------------------------------
  //  User interaction  (mouse clicks on board squares)
  // ------------------------------------------------------------------

  // Call when the user clicks on a board square.
  // Returns true if a move was made as a result.
  bool onSquareClicked(Square sq);

  // Current selection (for highlighting)
  const Selection &selection() const { return selection_; }

  bool isSelected(Square sq) const {
    return selection_.phase == SelectionPhase::PieceSelected &&
           selection_.square == sq;
  }
  bool isLegalDest(Square sq) const;

  // ------------------------------------------------------------------
  //  Programmatic move (e.g. from engine)
  // ------------------------------------------------------------------
  bool applyMove(const Move &m);

  // Undo the last half-move
  bool undoMove();
  bool canUndo() const { return board_.canUndo(); }

  // ------------------------------------------------------------------
  //  Move history (for move list panel)
  // ------------------------------------------------------------------
  const std::vector<Move> &history() const { return board_.history(); }

  // ------------------------------------------------------------------
  //  Callbacks (optional – renderer / engine adapter can subscribe)
  // ------------------------------------------------------------------
  void setOnMoveMade(MoveCallback cb) { onMoveMade_ = std::move(cb); }
  void setOnStatusChanged(StatusCallback cb) {
    onStatusChanged_ = std::move(cb);
  }

  // ------------------------------------------------------------------
  //  Event system  – subscribe once, receive all game events
  //
  //  Returns an opaque listener ID that can later be passed to
  //  removeGameEventListener() to unsubscribe.
  // ------------------------------------------------------------------
  int  addGameEventListener(GameEventListener listener);
  void removeGameEventListener(int id);

private:
  Board      board_;
  GameStatus status_ = GameStatus::Playing;
  Selection  selection_;

  MoveCallback   onMoveMade_;
  StatusCallback onStatusChanged_;

  // Event system
  struct ListenerEntry {
    int               id;
    GameEventListener fn;
  };
  std::vector<ListenerEntry> listeners_;
  int                        nextListenerId_ = 0;

  void emit(const GameEvent &event) const;

  // Re-evaluate game status after a move
  void updateStatus();

  // Try to execute a move from current selection to `dest`
  bool tryMoveToSquare(Square dest);
};

} // namespace XiangQi
