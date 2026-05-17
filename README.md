# imXiangQi

Chinese Chess (Xiangqi / 象棋) GUI written in C++17 with ImGui + SDL2 + OpenGL.

## Features (v0.1)

- Full Xiangqi board with PNG assets
- All piece move rules (King, Advisor, Elephant, Knight, Rook, Cannon, Pawn)
- Click-to-move with legal move highlighting
- Check / checkmate / stalemate detection
- Move history panel, Undo, Flip board
- Engine adapter interface (UCCI/UCI) — coming next

## Dependencies

| Library | Purpose | Install |
|---------|---------|---------|
| SDL2 | Window + input | `sudo apt install libsdl2-dev` |
| OpenGL + GLEW | Rendering | `sudo apt install libglew-dev` |
| cmake ≥ 3.16 | Build system | `sudo apt install cmake` |
| clang-format | Code formatting | `sudo apt install clang-format` |

### Vendored (not in git, clone manually)

```bash
git clone --depth=1 https://github.com/ocornut/imgui.git vendor/imgui
git clone --depth=1 https://github.com/nothings/stb.git  vendor/stb
```

## Build

```bash
# Debug (default)
./scripts/build.sh

# Release
./scripts/build.sh release

# Format only
./scripts/build.sh fmt

# Build then run
./scripts/build.sh run

# Clean
./scripts/build.sh clean
```

## Project structure

```
imxiangqi/
├── assets/
│   └── images/
│       ├── board/          # xiangqiboard_.png  (900×1000)
│       └── pieces/
│           ├── red/        # rK rA rB rN rR rC rP .png
│           └── black/      # bK bA bB bN bR bC bP .png
├── scripts/
│   └── build.sh
├── src/
│   ├── core/
│   │   ├── Piece.hpp/.cpp      # PieceType, PieceColor, Piece
│   │   ├── Move.hpp/.cpp       # Square, Move, UCCI notation
│   │   ├── Board.hpp/.cpp      # Grid, move generation, undo
│   │   └── GameState.hpp/.cpp  # Selection, turn, status, callbacks
│   ├── renderer/
│   │   ├── TextureManager.hpp/.cpp   # stb_image → OpenGL textures
│   │   └── BoardRenderer.hpp/.cpp    # ImGui draw calls, input
│   └── main.cpp                      # SDL2 + GLEW + ImGui app loop
├── vendor/                     # (git-ignored, clone manually)
│   ├── imgui/
│   └── stb/
├── .clang-format
└── CMakeLists.txt
```
