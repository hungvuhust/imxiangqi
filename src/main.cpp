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
#include "engine/EngineController.hpp"
#include "renderer/TextureManager.hpp"
#include "ui/AppContext.hpp"
#include "ui/BoardPanel.hpp"
#include "ui/ControlPanel.hpp"
#include "ui/EngineLogPanel.hpp"
#include "ui/IPanel.hpp"
#include "ui/MoveHistoryPanel.hpp"
#include "ui/SettingsDialog.hpp"
#include "ui/StatusPanel.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

using namespace XiangQi;

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

class Application {
public:
  Application() = default;
  ~Application() { shutdown(); }

  bool init(const char *argv0) {
    if (!initSDL() || !initImGui() || !loadAssets(argv0) || !buildPanels())
      return false;
    loadSettings();
    registerGameEventListener();
    return true;
  }

  void run() {
    while (!quit_) {
      processEvents();
      update();
      render();
    }
  }

private:
  SDL_Window   *window_ = nullptr;
  SDL_GLContext glCtx_  = nullptr;
  bool          quit_   = false;

  GameSettings     settings_;
  GameState        gameState_;
  TextureManager   texMgr_;
  EngineController engineRed_;   // engine dùng cho bên Đỏ
  EngineController engineBlack_; // engine dùng cho bên Đen

  std::optional<EngineSuggestion> lastHint_;

  std::vector<std::unique_ptr<IPanel>> panels_;
  bool                                 dockLayoutBuilt_ = false;
  SettingsDialog                       settingsDialog_;
  ImFont                              *fontVN_ = nullptr;

  std::chrono::steady_clock::time_point undoDelayUntil_{};
  static constexpr auto UNDO_RESTART_DELAY = std::chrono::seconds(3);

  int  gameEventListenerId_        = -1;
  bool pendingAnalyzeRestartRed_   = false;
  bool pendingAnalyzeRestartBlack_ = false;

  // Khi engine mode: giữ nước đi lại 500ms để hint arrow hiển thị trước
  std::string                           pendingEngineMove_;
  std::chrono::steady_clock::time_point pendingEngineMoveAt_{};

  // Trả về engine đang đến lượt (theo sideToMove)
  EngineController &activeEngine() {
    return (gameState_.sideToMove() == PieceColor::Red) ? engineRed_
                                                        : engineBlack_;
  }
  // Trả về engine của bên cụ thể
  EngineController &engineFor(PieceColor c) {
    return (c == PieceColor::Red) ? engineRed_ : engineBlack_;
  }

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

  bool initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
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

  bool loadAssets(const char *argv0) {
    std::string assets = resolveAssetsDir(argv0);
    printf("[App] Assets dir: %s\n", assets.c_str());
    if (!texMgr_.loadAll(assets))
      fprintf(stderr, "[App] Warning: some textures failed; using fallback.\n");

    // Load Roboto with Vietnamese glyph ranges
    std::string  fontPath = assets + "/fonts/Roboto-Regular.ttf";
    ImGuiIO     &io       = ImGui::GetIO();
    // Build a glyph range covering Latin + Vietnamese
    ImFontConfig cfg;
    cfg.OversampleH               = 2;
    cfg.OversampleV               = 2;
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0102, 0x0103, // Vietnamese
        0x0110, 0x0111, 0x0128, 0x0129, 0x0168, 0x0169, 0x01A0,
        0x01A1, 0x01AF, 0x01B0, 0x1EA0, 0x1EF9, // Vietnamese extended block
        0x2013, 0x2014,                         // em/en dash
        0,
    };
    fontVN_ =
        io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f, &cfg, ranges);
    if (!fontVN_)
      fprintf(stderr,
              "[App] Warning: failed to load Roboto font from %s\n",
              fontPath.c_str());

    return true;
  }

  bool buildPanels() {
    panels_.emplace_back(std::make_unique<BoardPanel>(texMgr_));
    panels_.emplace_back(std::make_unique<ControlPanel>());
    panels_.emplace_back(std::make_unique<StatusPanel>());
    panels_.emplace_back(std::make_unique<MoveHistoryPanel>());

    panels_.emplace_back(std::make_unique<EngineLogPanel>());
    return true;
  }

  // -----------------------------------------------------------------------
  //  Game event system – single subscriber that owns all engine reactions
  // -----------------------------------------------------------------------
  void registerGameEventListener() {
    gameEventListenerId_ = gameState_.addGameEventListener(
        [this](const GameEvent &ev) { onGameEvent(ev); });
  }

  void onGameEvent(const GameEvent &ev) {
    switch (ev.kind) {

    case GameEventKind::MoveMade:
      lastHint_.reset();
      engineRed_.clearSnapshot();
      engineBlack_.clearSnapshot();
      // If analyze mode is active, stop and re-queue restart on new position
      for (auto *eng : {&engineRed_, &engineBlack_}) {
        if (eng->isAnalyzing()) {
          eng->stopAnalyze();
          if (eng == &engineRed_)
            pendingAnalyzeRestartRed_ = true;
          else
            pendingAnalyzeRestartBlack_ = true;
        }
      }
      // Ponder / engine-move requests are handled by maybeRequestEngineMove()
      break;

    case GameEventKind::MoveUndone: {
      lastHint_.reset();
      pendingEngineMove_.clear();
      bool wasRedAnalyzing   = engineRed_.isAnalyzing();
      bool wasBlackAnalyzing = engineBlack_.isAnalyzing();
      for (auto *eng : {&engineRed_, &engineBlack_}) {
        eng->clearSnapshot();
        eng->stopAnalyze();
        eng->stopPonder();
        eng->cancelThinking();
      }
      pendingAnalyzeRestartRed_   = wasRedAnalyzing;
      pendingAnalyzeRestartBlack_ = wasBlackAnalyzing;
      undoDelayUntil_ = std::chrono::steady_clock::now() + UNDO_RESTART_DELAY;
      break;
    }

    case GameEventKind::GameReset:
    case GameEventKind::FenLoaded: {
      lastHint_.reset();
      pendingEngineMove_.clear();
      bool wasRedAnalyzing   = engineRed_.isAnalyzing();
      bool wasBlackAnalyzing = engineBlack_.isAnalyzing();
      for (auto *eng : {&engineRed_, &engineBlack_}) {
        eng->clearSnapshot();
        eng->stopAnalyze();
        eng->stopPonder();
        eng->cancelThinking();
      }
      pendingAnalyzeRestartRed_   = wasRedAnalyzing;
      pendingAnalyzeRestartBlack_ = wasBlackAnalyzing;
      break;
    }

    case GameEventKind::GameOver:
      for (auto *eng : {&engineRed_, &engineBlack_}) {
        eng->clearSnapshot();
        eng->stopAnalyze();
        eng->stopPonder();
        eng->cancelThinking();
      }
      pendingAnalyzeRestartRed_   = false;
      pendingAnalyzeRestartBlack_ = false;
      break;
    }
  }

  bool isEngineTurn() const {
    PieceColor stm = gameState_.sideToMove();
    if (stm == PieceColor::Red)
      return settings_.redPlayer == PlayerMode::Engine;
    if (stm == PieceColor::Black)
      return settings_.blackPlayer == PlayerMode::Engine;
    return false;
  }

  bool tryApplyEngineMoveUcci(const std::string &ucci) {
    if (ucci.size() < 4)
      return false;

    Square from = Square::fromString(ucci.substr(0, 2));
    Square to   = Square::fromString(ucci.substr(2, 2));
    if (!from.valid() || !to.valid())
      return false;

    const Piece &moved    = gameState_.board().at(from);
    const Piece &captured = gameState_.board().at(to);
    if (moved.empty())
      return false;

    Move m{from, to, moved, captured};
    return gameState_.applyMove(m);
  }

  // Consume outputs từ cả 2 engine; chỉ engine đang đến lượt mới sinh move
  void consumeEngineOutputs() {
    auto now = std::chrono::steady_clock::now();

    for (auto *eng : {&engineRed_, &engineBlack_}) {
      EngineSuggestion hint;
      if (eng->consumePendingHint(hint)) {
        if (now >= undoDelayUntil_)
          lastHint_ = hint;
      }

      std::string moveUcci;
      if (eng->consumePendingMoveUcci(moveUcci)) {
        if (now < undoDelayUntil_) {
          eng->log().push(EngineLogDir::Sys,
                          "ignoring engine move during undo delay: " +
                              moveUcci);
          continue;
        }
        // Lưu pending, delay 500ms để hint arrow hiển thị trước khi apply
        pendingEngineMove_   = moveUcci;
        pendingEngineMoveAt_ = now + std::chrono::milliseconds(500);
      }
    }

    // Flush pending move sau delay
    if (!pendingEngineMove_.empty() && now >= pendingEngineMoveAt_) {
      std::string moveUcci = pendingEngineMove_;
      pendingEngineMove_.clear();

      // Lấy engine của bên đang đến lượt TRƯỚC khi apply (bên vừa tính xong)
      EngineController &justMovedEngine = activeEngine();

      if (!tryApplyEngineMoveUcci(moveUcci)) {
        justMovedEngine.log().push(EngineLogDir::Err,
                                   "failed to apply bestmove: " + moveUcci);
      } else {
        lastHint_.reset();
        // Sau apply, sideToMove đã đổi — maybeStartPonder sẽ dùng
        // activeEngine() mới
        maybeStartPonder(justMovedEngine, moveUcci);
      }
    }
  }

  // Pondering: engine vừa đi xong, ta gợi ý ponder cho engine đối thủ
  void maybeStartPonder(EngineController &justMovedEng,
                        const std::string & /*engineMoveUcci*/) {
    // Bên vừa đi xong → sideToMove đã đổi sang đối thủ
    EngineController     &opponentEng = activeEngine();
    const EngineSettings &opponentSettings =
        (gameState_.sideToMove() == PieceColor::Red) ? settings_.engineRed
                                                     : settings_.engineBlack;

    if (!opponentSettings.ponder)
      return;
    if (!gameState_.isPlaying())
      return;
    if (!opponentEng.isReady())
      return;

    // Extract ponder move từ PV của engine vừa đi
    if (justMovedEng.lastInfo() && !justMovedEng.lastInfo()->pv.empty()) {
      const std::string &pv         = justMovedEng.lastInfo()->pv;
      std::string        ponderMove = pv.substr(0, pv.find(' '));
      if (!ponderMove.empty())
        opponentEng.startPonder(gameState_, ponderMove);
    }
  }

  void maybeRequestEngineMove() {
    if (!settings_.hasEngine())
      return;
    if (!gameState_.isPlaying())
      return;
    if (!isEngineTurn())
      return;

    // Wait if we just did an undo
    if (std::chrono::steady_clock::now() < undoDelayUntil_)
      return;

    EngineController &eng = activeEngine();

    // If we are pondering and the opponent just played, check ponderhit
    if (eng.isPondering()) {
      eng.sendPonderHit();
      return;
    }

    if (!eng.isReady())
      return;
    if (eng.isThinking())
      return;

    eng.requestMove(gameState_);
  }

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
        case SDLK_n:
          gameState_
              .newGame(); // → GameReset event → onGameEvent handles engine
          break;
        case SDLK_z:
          if (gameState_.canUndo())
            gameState_
                .undoMove(); // → MoveUndone event → onGameEvent handles engine
          break;
        case SDLK_ESCAPE: quit_ = true; break;
        default: break;
        }
      }
    }
  }

  void update() {
    engineRed_.configure(settings_.engineRed);
    engineRed_.update(gameState_);
    engineBlack_.configure(settings_.engineBlack);
    engineBlack_.update(gameState_);

    consumeEngineOutputs();

    // Restart analyze once engine returns to Ready after a position change
    if (pendingAnalyzeRestartRed_ && engineRed_.isReady()) {
      pendingAnalyzeRestartRed_ = false;
      engineRed_.startAnalyze(gameState_);
    }
    if (pendingAnalyzeRestartBlack_ && engineBlack_.isReady()) {
      pendingAnalyzeRestartBlack_ = false;
      engineBlack_.startAnalyze(gameState_);
    }

    maybeRequestEngineMove();

    AppContext ctx{gameState_,
                   settings_,
                   engineRed_,
                   engineBlack_,
                   texMgr_,
                   fontVN_,
                   lastHint_};
    for (auto &p : panels_) p->onUpdate(ctx);
  }

  void render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    renderMainMenuBar();
    renderDockSpace();

    AppContext ctx{gameState_,
                   settings_,
                   engineRed_,
                   engineBlack_,
                   texMgr_,
                   fontVN_,
                   lastHint_};
    for (auto &p : panels_)
      if (p->isOpen())
        p->onRender(ctx);

    settingsDialog_.render(ctx);

    ImGui::Render();

    int w, h;
    SDL_GL_GetDrawableSize(window_, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.11f, 0.11f, 0.13f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window_);
  }

  void renderMainMenuBar() {
    if (!ImGui::BeginMainMenuBar())
      return;

    if (ImGui::BeginMenu("Game")) {
      if (ImGui::MenuItem("New Game", "N"))
        gameState_.newGame(); // → event → onGameEvent
      if (ImGui::MenuItem("Undo Move", "Z") && gameState_.canUndo())
        gameState_.undoMove(); // → event → onGameEvent
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
      if (ImGui::MenuItem("Save layout as default")) {
        ImGui::SaveIniSettingsToDisk("imgui.ini");
      }
      if (ImGui::MenuItem("Reset layout"))
        dockLayoutBuilt_ = false;
      ImGui::EndMenu();
    }

    if (ImGui::MenuItem("Settings..."))
      settingsDialog_.open();

    const char *turn  = (gameState_.sideToMove() == PieceColor::Red)
                            ? "  Red to move"
                            : "  Black to move";
    float       menuW = ImGui::GetContentRegionAvail().x;
    float       textW = ImGui::CalcTextSize(turn).x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + menuW - textW - 10.f);
    ImGui::TextDisabled("%s", turn);

    ImGui::EndMainMenuBar();
  }

  void renderDockSpace() {
    ImGuiWindowFlags hostFlags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar;

    ImGuiViewport *vp    = ImGui::GetMainViewport();
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

    if (!dockLayoutBuilt_) {
      if (!std::filesystem::exists("imgui.ini")) {
        buildDefaultDockLayout(dockId);
      }
      dockLayoutBuilt_ = true;
    }

    ImGui::End();
  }

  void buildDefaultDockLayout(ImGuiID dockId) {
    ImGui::DockBuilderRemoveNode(dockId);
    ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);

    ImGuiViewport *vp = ImGui::GetMainViewport();
    ImGui::DockBuilderSetNodeSize(dockId, vp->Size);

    ImGuiID leftId, rightId;
    ImGui::DockBuilderSplitNode(dockId,
                                ImGuiDir_Left,
                                0.70f,
                                &leftId,
                                &rightId);

    ImGuiID boardId, historyId;
    ImGui::DockBuilderSplitNode(leftId,
                                ImGuiDir_Up,
                                0.75f,
                                &boardId,
                                &historyId);

    ImGuiID rightTopId, settingsId;
    ImGui::DockBuilderSplitNode(rightId,
                                ImGuiDir_Up,
                                0.60f,
                                &rightTopId,
                                &settingsId);

    ImGuiID controlsId, statusId;
    ImGui::DockBuilderSplitNode(rightTopId,
                                ImGuiDir_Up,
                                0.50f,
                                &controlsId,
                                &statusId);

    ImGui::DockBuilderDockWindow("Board", boardId);
    ImGui::DockBuilderDockWindow("Move History", historyId);
    ImGui::DockBuilderDockWindow("Controls", controlsId);
    ImGui::DockBuilderDockWindow("Status", statusId);
    ImGui::DockBuilderDockWindow("Settings", settingsId);
    ImGui::DockBuilderDockWindow("Engine Settings", settingsId);
    ImGui::DockBuilderDockWindow("Engine Log", settingsId);

    ImGui::DockBuilderFinish(dockId);
  }

  static constexpr const char *SETTINGS_FILE = "settings.ini";

  void loadSettings() {
    std::ifstream f(SETTINGS_FILE);
    if (!f.is_open())
      return;
    std::string data((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    settings_ = GameSettings::deserialize(data);
    printf("[App] Settings loaded from %s\n", SETTINGS_FILE);
  }

  void saveSettings() {
    std::ofstream f(SETTINGS_FILE);
    if (!f.is_open()) {
      fprintf(stderr, "[App] Failed to save settings to %s\n", SETTINGS_FILE);
      return;
    }
    f << settings_.serialize();
    printf("[App] Settings saved to %s\n", SETTINGS_FILE);
  }

  void shutdown() {
    saveSettings();
    engineRed_.stop();
    engineBlack_.stop();
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

int main(int argc, char *argv[]) {
  Application app;
  if (!app.init(argc > 0 ? argv[0] : "./imxiangqi"))
    return EXIT_FAILURE;
  app.run();
  return EXIT_SUCCESS;
}
