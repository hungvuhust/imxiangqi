cmake_minimum_required(VERSION 3.16)
project(imxiangqi VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------- Dependencies ----------
find_package(SDL2   REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW   REQUIRED)
find_package(Threads REQUIRED)

# ---------- ImGui ----------
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ---------- Project sources ----------
set(SOURCES
    src/main.cpp
    # core
    src/core/Piece.cpp
    src/core/Move.cpp
    src/core/Board.cpp
    src/core/GameState.cpp
    src/core/GameSettings.cpp
    # renderer
    src/renderer/TextureManager.cpp
    src/renderer/BoardRenderer.cpp
    # engine
    src/engine/EngineLog.cpp
    src/engine/EngineProtocol.cpp
    src/engine/EngineProcess.cpp
    src/engine/EngineController.cpp
    # ui panels
    src/ui/BoardPanel.cpp
    src/ui/EngineLogPanel.cpp
    src/ui/ControlPanel.cpp
    src/ui/StatusPanel.cpp
    src/ui/MoveHistoryPanel.cpp
    src/ui/SettingsPanel.cpp
    src/ui/EngineSettingsPanel.cpp
)

add_executable(imxiangqi ${SOURCES} ${IMGUI_SOURCES})

target_include_directories(imxiangqi PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${CMAKE_SOURCE_DIR}/vendor/stb
    ${CMAKE_SOURCE_DIR}/vendor/portable-file-dialogs
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/renderer
    ${CMAKE_SOURCE_DIR}/src/ui
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(imxiangqi PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    GLEW::GLEW
    Threads::Threads
    ${CMAKE_DL_LIBS}
)

# Copy assets to build dir
add_custom_command(TARGET imxiangqi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:imxiangqi>/assets
    COMMENT "Copying assets..."
)
