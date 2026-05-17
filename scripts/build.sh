#!/usr/bin/env bash
# =============================================================================
#  build.sh  –  Build script for imXiangQi
#
#  Usage:
#    ./scripts/build.sh                    # Debug build (default)
#    ./scripts/build.sh release            # Release build
#    ./scripts/build.sh clean              # Remove build directories
#    ./scripts/build.sh run                # Debug build then launch
#    ./scripts/build.sh release run        # Release build then launch
#    ./scripts/build.sh noformat           # Skip clang-format step
#    ./scripts/build.sh fmt                # Format only, no build
#
# =============================================================================
set -euo pipefail

# ---- Paths ------------------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_TYPE="Debug"
DO_RUN=false
DO_FORMAT=true
FORMAT_ONLY=false
BUILD_DIR=""

# ---- Parse arguments --------------------------------------------------------
for arg in "$@"; do
    case "${arg,,}" in          # lowercase comparison
        release)    BUILD_TYPE="Release" ;;
        debug)      BUILD_TYPE="Debug"   ;;
        noformat)   DO_FORMAT=false      ;;
        fmt|format) FORMAT_ONLY=true     ;;
        clean)
            echo "[build] Removing build directories..."
            rm -rf "${ROOT_DIR}/build-debug" "${ROOT_DIR}/build-release"
            echo "[build] Done."
            exit 0
            ;;
        run)  DO_RUN=true ;;
        *)
            echo "[build] Unknown argument: ${arg}"
            echo "        Valid: debug | release | clean | run | noformat | fmt"
            exit 1
            ;;
    esac
done

BUILD_DIR="${ROOT_DIR}/build-${BUILD_TYPE,,}"

# ---- Check dependencies -----------------------------------------------------
check_dep() {
    if ! command -v "$1" &>/dev/null; then
        echo "[build] ERROR: '$1' not found. Install it first."
        echo "        Hint: sudo apt install $2"
        exit 1
    fi
}

check_dep cmake      "cmake"
check_dep g++        "build-essential"
check_dep pkg-config "pkg-config"

# Check SDL2 via pkg-config
if ! pkg-config --exists sdl2 2>/dev/null; then
    echo "[build] ERROR: SDL2 development headers not found."
    echo "        Hint: sudo apt install libsdl2-dev"
    exit 1
fi

# Check GLEW via pkg-config
if ! pkg-config --exists glew 2>/dev/null; then
    echo "[build] ERROR: GLEW development headers not found."
    echo "        Hint: sudo apt install libglew-dev"
    exit 1
fi

# ---- clang-format ------------------------------------------------------------
run_format() {
    # Collect all source files under src/ (exclude vendor/)
    local extensions="c,h,cc,hh,cpp,hpp"
    local files
    files=$(find "${ROOT_DIR}/src" \
                 -type f \
                 \( -name "*.c"   -o -name "*.h"   \
                 -o -name "*.cc"  -o -name "*.hh"  \
                 -o -name "*.cpp" -o -name "*.hpp" \) \
                 2>/dev/null)

    if [[ -z "${files}" ]]; then
        echo "[fmt]   No source files found under src/"
        return
    fi

    local count=0
    local changed=0
    while IFS= read -r f; do
        local before after
        before=$(cat "${f}")
        clang-format --style=file:"${ROOT_DIR}/.clang-format" -i "${f}"
        after=$(cat "${f}")
        count=$(( count + 1 ))
        if [[ "${before}" != "${after}" ]]; then
            echo "[fmt]   formatted: ${f#"${ROOT_DIR}/"}"
            changed=$(( changed + 1 ))
        fi
    done <<< "${files}"

    if [[ ${changed} -eq 0 ]]; then
        echo "[fmt]   ${count} file(s) checked — already clean."
    else
        echo "[fmt]   ${count} file(s) checked, ${changed} reformatted."
    fi
}

if [[ "${DO_FORMAT}" == true ]]; then
    if command -v clang-format &>/dev/null; then
        if [[ -f "${ROOT_DIR}/.clang-format" ]]; then
            echo "[fmt] Running clang-format..."
            run_format
        else
            echo "[fmt] WARNING: .clang-format not found at project root, skipping."
        fi
    else
        echo "[fmt] WARNING: clang-format not installed, skipping."
        echo "      Hint: sudo apt install clang-format"
    fi
fi

# fmt-only mode: exit after formatting
if [[ "${FORMAT_ONLY}" == true ]]; then
    echo "[fmt] Done."
    exit 0
fi

# ---- CMake configure (only if needed) ---------------------------------------
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    echo "[build] Configuring (${BUILD_TYPE})..."
    cmake -S "${ROOT_DIR}" \
          -B "${BUILD_DIR}" \
          -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
          -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    # Symlink compile_commands.json to root for editor LSP support
    ln -sf "${BUILD_DIR}/compile_commands.json" "${ROOT_DIR}/compile_commands.json" 2>/dev/null || true
else
    echo "[build] CMake cache exists, skipping configure."
fi

# ---- Build ------------------------------------------------------------------
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
echo "[build] Building with ${NPROC} jobs (${BUILD_TYPE})..."
cmake --build "${BUILD_DIR}" -j"${NPROC}"

BINARY="${BUILD_DIR}/imxiangqi"
if [[ ! -f "${BINARY}" ]]; then
    echo "[build] ERROR: Binary not found after build: ${BINARY}"
    exit 1
fi

echo ""
echo "[build] ✓ Build successful: ${BINARY}"
echo ""

# ---- Run (optional) ---------------------------------------------------------
if [[ "${DO_RUN}" == true ]]; then
    echo "[build] Launching imXiangQi..."
    exec "${BINARY}"
fi
