// GLEW must come before any other OpenGL header
#include <GL/glew.h>

#include <SDL.h>
#include <SDL_opengl.h>

#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl2.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "core/GameSettings.hpp"
#include "core/GameState.hpp"
#include "renderer/TextureManager.hpp"
#include "ui/AppContext.hpp"
#include "ui/BoardPanel.hpp"
#include "ui/ControlPanel.hpp"
#include "ui/IPanel.hpp"
#include "ui/MoveHistoryPanel.hpp"
#include "ui/SettingsPanel.hpp"
#include "ui/StatusPanel.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace XiangQi;

// -----------------------------------------------------------------------
//  Helpers
// -----------------------------------------------------------------------
static std::string resolveAssetsDir(const char *argv0) {
  namespace fs  = std::filesystem;
  fs::path exe  = fs::absolute(fs::path(argv0)).parent_path();
  fs::path cand = exe / "assets";
  if (fs::is_directory(cand))
    return cand.string();
  cand = fs::current_path() / "assets";
  if (fs::is_directory(cand))
    return cand.string();
  return (exe / "assets").string();
}

// -----------------------------------------------------------------------
//  Application
// -----------------------------------------------------------------------
class Application {
public:
  Application() = default;
  ~Application() { shutdown(); }

  bool init(const char *argv0) {
    return initSDL() && initImGui() && loadAssets(argv0) && buildPanels();
  }

  void run() {
    while (!quit_) {
      processEvents();
      update();
      render();
    }
  }

private:
  // --- SDL / GL -----------------------------------------------------------
  SDL_Window   *window_ = nullptr;
  SDL_GLContext glCtx_  = nullptr;
  bool          quit_   = false;

  // --- Core objects -------------------------------------------------------
  GameSettings   settings_;
  GameState      gameState_;
  TextureManager texMgr_;

  // --- UI -----------------------------------------------------------------
  // Panels are stored as unique_ptr<IPanel> in a flat list.
  // Order = default dock layout priority.
  std::vector<std::unique_ptr<IPanel>> panels_;

  // First-frame flag: set up default dock layout once
  bool dockLayoutBuilt_ = false;

  // -----------------------------------------------------------------------
  bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
      fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
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
                               1200,
                               780,
                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE |
                                   SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
      fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
      return false;
    }

    glCtx_ = SDL_GL_CreateContext(window_);
    if (!glCtx_) {
      fprintf(stderr, "SDL_GL_CreateContext: %s\n", SDL_GetError());
      return false;
    }
    SDL_GL_MakeCurrent(window_, glCtx_);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum err       = glewInit();
    if (err != GLEW_OK) {
      fprintf(stderr, "glewInit: %s\n", glewGetErrorString(err));
      return false;
    }
    return true;
  }

  // -----------------------------------------------------------------------
  bool initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    // Enable docking + multi-viewport
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle &style    = ImGui::GetStyle();
    style.WindowRounding = 5.f;
    style.FrameRounding  = 3.f;
    style.ItemSpacing    = {8, 5};
    style.WindowPadding  = {10, 10};

    ImGui_ImplSDL2_InitForOpenGL(window_, glCtx_);
    ImGui_ImplOpenGL3_Init("#version 330");
    return true;
  }

  // -----------------------------------------------------------------------
  bool loadAssets(const char *argv0) {
    std::string assets = resolveAssetsDir(argv0);
    printf("[App] Assets dir: %s\n", assets.c_str());
    if (!texMgr_.loadAll(assets))
      fprintf(stderr, "[App] Warning: some textures failed; using fallback.\n");
    return true;
  }

  // -----------------------------------------------------------------------
  bool buildPanels() {
    panels_.emplace_back(std::make_unique<BoardPanel>(texMgr_));
    panels_.emplace_back(std::make_unique<ControlPanel>());
    panels_.emplace_back(std::make_unique<StatusPanel>());
    panels_.emplace_back(std::make_unique<MoveHistoryPanel>());
    panels_.emplace_back(std::make_unique<SettingsPanel>());
    return true;
  }

  // -----------------------------------------------------------------------
  void processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT)
        quit_ = true;
      if (e.type == SDL_WINDOWEVENT &&
          e.window.event == SDL_WINDOWEVENT_CLOSE &&
          e.window.windowID == SDL_GetWindowID(window_))
        quit_ = true;

      if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_n: gameState_.newGame(); break;
        case SDLK_z:
          if (gameState_.canUndo())
            gameState_.undoMove();
          break;
        case SDLK_ESCAPE: quit_ = true; break;
        default: break;
        }
      }
    }
  }

  // -----------------------------------------------------------------------
  void update() {
    AppContext ctx{gameState_, settings_, texMgr_};
    for (auto &p : panels_) p->onUpdate(ctx);
  }

  // -----------------------------------------------------------------------
  void render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderMainMenuBar();
    renderDockSpace();

    AppContext ctx{gameState_, settings_, texMgr_};
    for (auto &p : panels_)
      if (p->isOpen())
        p->onRender(ctx);

    ImGui::Render();

    int w, h;
    SDL_GL_GetDrawableSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.11f, 0.11f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
  }

  // -----------------------------------------------------------------------
  //  Main menu bar
  // -----------------------------------------------------------------------
  void renderMainMenuBar() {
    if (!ImGui::BeginMainMenuBar())
      return;

    if (ImGui::BeginMenu("Game")) {
      if (ImGui::MenuItem("New Game", "N"))
        gameState_.newGame();
      if (ImGui::MenuItem("Undo Move", "Z") && gameState_.canUndo())
        gameState_.undoMove();
      ImGui::Separator();
      if (ImGui::MenuItem("Quit", "ESC"))
        quit_ = true;
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      for (auto &p : panels_) {
        bool open = p->isOpen();
        if (ImGui::MenuItem(p->title().c_str(), nullptr, &open))
          p->setOpen(open);
      }
      ImGui::Separator();
      if (ImGui::MenuItem("Reset layout"))
        dockLayoutBuilt_ = false; // force rebuild on next frame
      ImGui::EndMenu();
    }

    // Right-side status text
    const char *turn  = (gameState_.sideToMove() == PieceColor::Red)
                            ? "  Red to move"
                            : "  Black to move";
    float       menuW = ImGui::GetContentRegionAvail().x;
    float       textW = ImGui::CalcTextSize(turn).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + menuW - textW - 10.f);
    ImGui::TextDisabled("%s", turn);

    ImGui::EndMainMenuBar();
  }

  // -----------------------------------------------------------------------
  //  Full-screen DockSpace host window
  // -----------------------------------------------------------------------
  void renderDockSpace() {
    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    ImGuiViewport *vp    = ImGui::GetMainViewport();
    // Offset by menu bar height
    float          menuH = ImGui::GetFrameHeight();
    ImGui::SetNextWindowPos({vp->Pos.x, vp->Pos.y + menuH});
    ImGui::SetNextWindowSize({vp->Size.x, vp->Size.y - menuH});
    ImGui::SetNextWindowViewport(vp->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});
    ImGui::Begin("##DockHost", nullptr, hostFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockId = ImGui::GetID("MainDock");
    ImGui::DockSpace(dockId, {0, 0}, ImGuiDockNodeFlags_PassthruCentralNode);

    // Build default layout once (first frame)
    if (!dockLayoutBuilt_) {
      buildDefaultDockLayout(dockId);
      dockLayoutBuilt_ = true;
    }

    ImGui::End();
  }

  // -----------------------------------------------------------------------
  //  Default dock layout:
  //
  //   +-------------------------+----------+
  //   |                         | Controls |
  //   |         Board           +----------+
  //   |                         |  Status  |
  //   +-------------------------+----------+
  //   |   Move History          | Settings |
  //   +-------------------------+----------+
  // -----------------------------------------------------------------------
  void buildDefaultDockLayout(ImGuiID dockId) {
    ImGui::DockBuilderRemoveNode(dockId);
    ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);

    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::DockBuilderSetNodeSize(dockId, vp->Size);

    // Split left (board) / right (sidebar)  70% / 30%
    ImGuiID leftId, rightId;
    ImGui::DockBuilderSplitNode(dockId,
                                ImGuiDir_Left,
                                0.70f,
                                &leftId,
                                &rightId);

    // Split left into top (board) / bottom (history)  75% / 25%
    ImGuiID boardId, historyId;
    ImGui::DockBuilderSplitNode(leftId,
                                ImGuiDir_Up,
                                0.75f,
                                &boardId,
                                &historyId);

    // Split right into top (controls+status) / bottom (settings)  60% / 40%
    ImGuiID rightTopId, settingsId;
    ImGui::DockBuilderSplitNode(rightId,
                                ImGuiDir_Up,
                                0.60f,
                                &rightTopId,
                                &settingsId);

    // Split right-top into controls / status  50% / 50%
    ImGuiID controlsId, statusId;
    ImGui::DockBuilderSplitNode(rightTopId,
                                ImGuiDir_Up,
                                0.50f,
                                &controlsId,
                                &statusId);

    // Dock panels by title
    ImGui::DockBuilderDockWindow("Board", boardId);
    ImGui::DockBuilderDockWindow("Move History", historyId);
    ImGui::DockBuilderDockWindow("Controls", controlsId);
    ImGui::DockBuilderDockWindow("Status", statusId);
    ImGui::DockBuilderDockWindow("Settings", settingsId);

    ImGui::DockBuilderFinish(dockId);
  }

  // -----------------------------------------------------------------------
  void shutdown() {
    panels_.clear();
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
int main(int argc, char *argv[]) {
  Application app;
  if (!app.init(argc > 0 ? argv[0] : "./imxiangqi"))
    return EXIT_FAILURE;
  app.run();
  return EXIT_SUCCESS;
}
