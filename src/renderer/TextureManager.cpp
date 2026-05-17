#include "TextureManager.hpp"

// stb_image: single-header, compile-time implementation
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// GLEW must be included before any other OpenGL header
#include <GL/glew.h>
#include <cstdio>
#include <utility>

namespace XiangQi {

// =======================================================================
//  Texture – RAII
// =======================================================================
Texture::~Texture() { release(); }

Texture::Texture(Texture &&o) noexcept
    : id_(o.id_)
    , width_(o.width_)
    , height_(o.height_) {
  o.id_     = 0;
  o.width_  = 0;
  o.height_ = 0;
}

Texture &Texture::operator=(Texture &&o) noexcept {
  if (this != &o) {
    release();
    id_      = o.id_;
    width_   = o.width_;
    height_  = o.height_;
    o.id_    = 0;
    o.width_ = o.height_ = 0;
  }
  return *this;
}

void Texture::release() {
  if (id_) {
    glDeleteTextures(1, &id_);
    id_ = 0;
  }
}

bool Texture::loadFromFile(const std::string &path) {
  int w, h, channels;
  stbi_set_flip_vertically_on_load(false);
  uint8_t *data = stbi_load(path.c_str(), &w, &h, &channels, 4);
  if (!data) {
    fprintf(stderr,
            "[TextureManager] Failed to load: %s  (%s)\n",
            path.c_str(),
            stbi_failure_reason());
    return false;
  }
  bool ok = loadFromMemory(data, w, h);
  stbi_image_free(data);
  return ok;
}

bool Texture::loadFromMemory(const uint8_t *rgba, int width, int height) {
  release();
  glGenTextures(1, &id_);
  glBindTexture(GL_TEXTURE_2D, id_);

  glTexParameteri(GL_TEXTURE_2D,
                  GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               width,
               height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               rgba);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);

  width_  = width;
  height_ = height;
  return true;
}

// =======================================================================
//  TextureManager
// =======================================================================
bool TextureManager::loadTexture(const std::string &key,
                                 const std::string &path) {
  Texture tex;
  if (!tex.loadFromFile(path))
    return false;
  textures_.emplace(key, std::move(tex));
  return true;
}

const Texture *TextureManager::get(const std::string &key) const {
  auto it = textures_.find(key);
  return (it != textures_.end()) ? &it->second : nullptr;
}

// piece key: "rK", "bP", etc.
// File names follow xiangqi-gui convention: rK.png, bC.png, etc.
std::string TextureManager::pieceKey(const Piece &piece) {
  if (piece.empty())
    return "";
  char colorChar = (piece.color() == PieceColor::Red) ? 'r' : 'b';
  char typeChar  = '.';
  switch (piece.type()) {
  case PieceType::King: typeChar = 'K'; break;
  case PieceType::Advisor: typeChar = 'A'; break;
  case PieceType::Elephant: typeChar = 'B'; break;
  case PieceType::Knight: typeChar = 'N'; break;
  case PieceType::Rook: typeChar = 'R'; break;
  case PieceType::Cannon: typeChar = 'C'; break;
  case PieceType::Pawn: typeChar = 'P'; break;
  default: return "";
  }
  return std::string{colorChar, typeChar};
}

const Texture *TextureManager::pieceTexture(const Piece &piece) const {
  return get(pieceKey(piece));
}

bool TextureManager::loadAll(const std::string &assetsRoot) {
  bool allOk = true;

  // --- Board ---
  std::string boardPath = assetsRoot + "/images/board/xiangqiboard_.png";
  if (!loadTexture("board", boardPath)) {
    fprintf(stderr,
            "[TextureManager] Board texture missing: %s\n",
            boardPath.c_str());
    allOk = false;
  }

  // --- Pieces ---
  // Red pieces
  static constexpr const char *pieceLetters[] =
      {"K", "A", "B", "N", "R", "C", "P"};
  for (const char *letter : pieceLetters) {
    std::string key  = std::string("r") + letter;
    std::string path = assetsRoot + "/images/pieces/red/r" + letter + ".png";
    if (!loadTexture(key, path))
      allOk = false;
  }
  // Black pieces
  for (const char *letter : pieceLetters) {
    std::string key  = std::string("b") + letter;
    std::string path = assetsRoot + "/images/pieces/black/b" + letter + ".png";
    if (!loadTexture(key, path))
      allOk = false;
  }

  loaded_ = allOk;
  return allOk;
}

} // namespace XiangQi
