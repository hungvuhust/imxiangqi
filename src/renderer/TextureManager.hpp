#pragma once
#include "../core/Piece.hpp"

#include <cstdint>
#include <string>
#include <unordered_map>

// Forward declare OpenGL type to avoid pulling in GL headers here
using GLuint = unsigned int;

namespace XiangQi {

// -----------------------------------------------------------------------
//  Texture  – thin RAII wrapper around an OpenGL texture object
// -----------------------------------------------------------------------
class Texture {
public:
  Texture() = default;
  ~Texture();

  // Non-copyable, movable
  Texture(const Texture &)            = delete;
  Texture &operator=(const Texture &) = delete;
  Texture(Texture &&o) noexcept;
  Texture &operator=(Texture &&o) noexcept;

  // Load from file via stb_image; returns false on failure
  bool loadFromFile(const std::string &path);

  // Load from raw RGBA pixels
  bool loadFromMemory(const uint8_t *rgba, int width, int height);

  bool   valid() const { return id_ != 0; }
  GLuint id() const { return id_; }
  int    width() const { return width_; }
  int    height() const { return height_; }

  // ImGui-friendly cast
  void *imguiID() const {
    return reinterpret_cast<void *>(static_cast<uintptr_t>(id_));
  }

private:
  GLuint id_     = 0;
  int    width_  = 0;
  int    height_ = 0;

  void release();
};

// -----------------------------------------------------------------------
//  TextureManager  – singleton that owns all loaded textures
//
//  Keys for piece textures follow the convention:
//    "rK", "rA", "rB", "rN", "rR", "rC", "rP"  (red pieces)
//    "bK", "bA", "bB", "bN", "bR", "bC", "bP"  (black pieces)
//  Board texture key: "board"
// -----------------------------------------------------------------------
class TextureManager {
public:
  TextureManager()  = default;
  ~TextureManager() = default;

  // Non-copyable, non-movable (owns textures by value in map)
  TextureManager(const TextureManager &)            = delete;
  TextureManager &operator=(const TextureManager &) = delete;

  // Load all assets from the given root directory.
  // Expected layout:
  //   <root>/images/board/xiangqiboard_.png
  //   <root>/images/pieces/red/r{K,A,B,N,R,C,P}.png
  //   <root>/images/pieces/black/b{K,A,B,N,R,C,P}.png
  bool loadAll(const std::string &assetsRoot);

  // Retrieve a texture by key; returns nullptr if not found
  const Texture *get(const std::string &key) const;

  // Convenience: get texture for a specific piece
  const Texture *pieceTexture(const Piece &piece) const;

  // Board background texture
  const Texture *boardTexture() const { return get("board"); }

  bool loaded() const { return loaded_; }

private:
  std::unordered_map<std::string, Texture> textures_;
  bool                                     loaded_ = false;

  // Build the asset key string from piece color/type
  static std::string pieceKey(const Piece &piece);

  bool loadTexture(const std::string &key, const std::string &path);
};

} // namespace XiangQi
