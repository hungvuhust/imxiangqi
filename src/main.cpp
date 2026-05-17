// GLEW must come before any other OpenGL header
#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>

#include "core/GameState.hpp"
#include "renderer/BoardRenderer.hpp"
#include "renderer/TextureManager.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

// -----------------------------------------------------------------------
//  Resolve the assets directory relative to the executable
// -----------------------------------------------------------------------
static std::string resolveAssetsDir(const char *argv0) {
  namespace fs    = std::filesystem;
  // argv0 may be bare name or full path; get its parent directory
  fs::path exeDir = fs::absolute(fs::path(argv0)).parent_path();
  fs::path assets = exeDir / "assets";
  if (fs::is_directory(assets))
    return assets.string();

  // Fallback: look relative to CWD
  assets = fs::current_path() / "assets";
  if (fs::is_directory(assets))
    return assets.string();

  // Give up – return a best guess so error messages are useful
  return (exeDir / "assets").string();
}

// -----------------------------------------------------------------------
//  Application  – owns SDL window, GL context, ImGui, game
// -----------------------------------------------------------------------
class Application {
public:
  Application() = default;
  ~Application() { shutdown(); }

  bool init(const char *argv0) {
    assetsDir_ = resolveAssetsDir(argv0);
    return initSDL() && initImGui() && loadAssets();
  }

  void run() {
    while (!quit_) {
      processEvents();
      beginFrame();
      renderUI();
      endFrame();
    }
  }

private:
  // ------------------------------------------------------------------
  //  SDL + OpenGL
  // ------------------------------------------------------------------
  SDL_Window   *window_ = nullptr;
  SDL_GLContext glCtx_  = nullptr;
  bool          quit_   = false;

  // ------------------------------------------------------------------
  //  Game objects
  // ------------------------------------------------------------------
  std::string             assetsDir_;
  XiangQi::TextureManager texMgr_;
  XiangQi::GameState      gameState_;
  XiangQi::BoardRenderer *renderer_ = nullptr;

  // ------------------------------------------------------------------
  bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      fprintf(stderr, "SDL_Init error: %s\n", SDL_GetError());
      return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    window_ = SDL_CreateWindow("imXiangQi – Chinese Chess",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               1100,
                               720,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
      fprintf(stderr, "SDL_CreateWindow error: %s\n", SDL_GetError());
      return false;
    }

    glCtx_ = SDL_GL_CreateContext(window_);
    if (!glCtx_) {
      fprintf(stderr, "SDL_GL_CreateContext error: %s\n", SDL_GetError());
      return false;
    }
    SDL_GL_MakeCurrent(window_, glCtx_);
    SDL_GL_SetSwapInterval(1); // vsync

    // Init GLEW to load OpenGL 3.x function pointers
    glewExperimental = GL_TRUE;
    GLenum glewErr   = glewInit();
    if (glewErr != GLEW_OK) {
      fprintf(stderr, "glewInit error: %s\n", glewGetErrorString(glewErr));
      return false;
    }
    return true;
  }

  // ------------------------------------------------------------------
  bool initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle &style    = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding  = 4.0f;
    style.ItemSpacing    = {8, 6};
    style.WindowPadding  = {12, 12};

    // Backends
    ImGui_ImplSDL2_InitForOpenGL(window_, glCtx_);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
  }

  // ------------------------------------------------------------------
  bool loadAssets() {
    printf("[App] Loading assets from: %s\n", assetsDir_.c_str());
    if (!texMgr_.loadAll(assetsDir_)) {
      fprintf(stderr,
              "[App] Warning: some textures failed to load. "
              "Falling back to vector graphics.\n");
    }
    renderer_ = new XiangQi::BoardRenderer(texMgr_);
    return true;
  }

  // ------------------------------------------------------------------
  void processEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        quit_ = true;
      if (event.type == SDL_WINDOWEVENT &&
          event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window_))
        quit_ = true;

      // Keyboard shortcuts
      if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_n: gameState_.newGame(); break;
        case SDLK_z: gameState_.undoMove(); break;
        case SDLK_ESCAPE: quit_ = true; break;
        default: break;
        }
      }
    }
  }

  // ------------------------------------------------------------------
  void beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
  }

  // ------------------------------------------------------------------
  void renderUI() {
    int winW, winH;
    SDL_GetWindowSize(window_, &winW, &winH);

    // Full-screen host window
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize(
        {static_cast<float>(winW), static_cast<float>(winH)});
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {16, 16});
    ImGui::Begin("##main", nullptr, flags);

    // Title bar row
    ImGui::TextColored({1.0f, 0.7f, 0.2f, 1.0f}, "imXiangQi");
    ImGui::SameLine();
    ImGui::TextDisabled(" |  N=New Game   Z=Undo   ESC=Quit");
    ImGui::Separator();
    ImGui::Spacing();

    // Dynamically resize board to fit window height with padding
    // boardSize = height of the board (PNG aspect ratio 900:1000 = 0.9).
    // Fit by height; make sure the width also fits in the available area.
    ImVec2 avail    = ImGui::GetContentRegionAvail();
    float  byHeight = avail.y - 8.0f;
    float byWidth = (avail.x - 240.0f) / 0.9f; // 240 px reserved for side panel
    renderer_->config().boardSize =
        std::max(300.0f, std::min(byHeight, byWidth));

    renderer_->render(gameState_);

    ImGui::End();
    ImGui::PopStyleVar();
  }

  // ------------------------------------------------------------------
  void endFrame() {
    ImGui::Render();
    int displayW, displayH;
    SDL_GL_GetDrawableSize(window_, &displayW, &displayH);
    glViewport(0, 0, displayW, displayH);
    glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
  }

  // ------------------------------------------------------------------
  void shutdown() {
    delete renderer_;
    renderer_ = nullptr;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (glCtx_)
      SDL_GL_DeleteContext(glCtx_);
    if (window_)
      SDL_DestroyWindow(window_);
    SDL_Quit();
  }
};

// -----------------------------------------------------------------------
//  main
// -----------------------------------------------------------------------
int main(int argc, char *argv[]) {
  Application app;
  if (!app.init(argc > 0 ? argv[0] : "./imxiangqi"))
    return EXIT_FAILURE;
  app.run();
  return EXIT_SUCCESS;
}
