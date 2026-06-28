#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Batch-refresh the Qt translation source (.ts) files of GpgFrontend and its
# integrated modules.
#
# Two kinds of translation target are handled:
#
#   * The GpgFrontend application itself, whose .ts live in
#     resource/lfs/locale/ts/GpgFrontend.<locale>.ts and whose translatable
#     strings come from src/core and src/ui (plus the top-level src/*.cpp).
#     The test/ and sdk/ trees are skipped — their tr() strings are not part of
#     the shipped UI and are intentionally absent from the committed .ts.
#
#   * Each module under modules/src/<name>/, which keeps its translations in a
#     ts/ subdirectory (e.g. modules/src/m_email/ts/ModuleEMail.de_DE.ts). Only
#     the module's top-level sources (*.cpp, *.h, *.ui) are scanned — vendored
#     subtrees such as m_email/vmime/ are intentionally skipped, matching what
#     the CMake build feeds to qt_add_translations().
#
# For every target this runs `lupdate` over its own sources so newly
# added/changed tr() strings show up in every locale's .ts as
# <translation type="unfinished">, ready to be filled in with Qt Linguist.
#
# This is offline: lupdate never touches the network. It does NOT translate
# anything; it only syncs the source strings into the .ts files.
#
# Usage: scripts/update_module_translations.sh [options] [target ...]
#   target             One or more target names to limit to. The application is
#                      named "GpgFrontend"; modules use their dir name (e.g.
#                      m_email). Default: the application plus every module
#                      under modules/src that has a ts/ dir.
#       --no-app       Skip the GpgFrontend application target.
#       --app-only     Process only the GpgFrontend application target.
#       --no-obsolete  Drop entries that no longer exist in the sources
#                      (passes -no-obsolete to lupdate).
#       --list         List the targets that would be processed, then exit.
#       --modules-dir DIR  Root holding the module dirs (default: modules/src).
#   -h, --help         Show this help.
#
# Environment:
#   LUPDATE            Path to the lupdate binary (auto-detected otherwise).
#
# Examples:
#   scripts/update_module_translations.sh
#   scripts/update_module_translations.sh GpgFrontend
#   scripts/update_module_translations.sh m_email
#   scripts/update_module_translations.sh --no-obsolete GpgFrontend m_email

set -uo pipefail

# --- locate repo root ------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# --- defaults --------------------------------------------------------------
MODULES_DIR="${REPO_ROOT}/modules/src"
APP_NAME="GpgFrontend"
APP_TS_DIR="${REPO_ROOT}/resource/lfs/locale/ts"
# Source roots scanned for the application target. test/ and sdk/ are left out
# on purpose (see header).
APP_SRC_ROOTS=("${REPO_ROOT}/src/core" "${REPO_ROOT}/src/ui")
NO_OBSOLETE=""
LIST_ONLY="no"
INCLUDE_APP="yes"
APP_ONLY="no"
declare -a ONLY_TARGETS=()

usage() { sed -n '2,/^set -uo/p' "${BASH_SOURCE[0]}" | sed '$d;s/^# \{0,1\}//'; }

# --- parse args ------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-app)      INCLUDE_APP="no" ;;
    --app-only)    APP_ONLY="yes" ;;
    --no-obsolete) NO_OBSOLETE="-no-obsolete" ;;
    --list)        LIST_ONLY="yes" ;;
    --modules-dir) MODULES_DIR="$2"; shift ;;
    -h|--help)     usage; exit 0 ;;
    -*)            echo "error: unknown option: $1" >&2; usage; exit 2 ;;
    *)             ONLY_TARGETS+=("$1") ;;
  esac
  shift
done

# --- locate lupdate --------------------------------------------------------
find_lupdate() {
  if [[ -n "${LUPDATE:-}" ]]; then echo "${LUPDATE}"; return; fi
  for c in lupdate lupdate-qt6 \
           /usr/lib/qt6/bin/lupdate /usr/lib/qt6/libexec/lupdate \
           /usr/lib/qt5/bin/lupdate; do
    if command -v "$c" >/dev/null 2>&1; then command -v "$c"; return; fi
    [[ -x "$c" ]] && { echo "$c"; return; }
  done
}

LUPDATE_BIN="$(find_lupdate)"
if [[ -z "${LUPDATE_BIN}" ]]; then
  echo "error: lupdate not found. Set LUPDATE=/path/to/lupdate." >&2
  exit 1
fi

# --- target selection helper -----------------------------------------------
should_process() {
  local name="$1"
  [[ ${#ONLY_TARGETS[@]} -eq 0 ]] && return 0
  local t
  for t in "${ONLY_TARGETS[@]}"; do [[ "$t" == "$name" ]] && return 0; done
  return 1
}

# Collect the targets to process into parallel arrays. Each target is described
# by a name, the directory holding its .ts files, and a "kind" that decides how
# its sources are gathered ("app" = recursive over APP_SRC_ROOTS; "module" =
# top-level sources of a single module dir).
declare -a T_NAMES=() T_TS_DIRS=() T_KINDS=() T_SRC_DIRS=()

# Application target.
if [[ "${INCLUDE_APP}" == "yes" ]] && should_process "${APP_NAME}"; then
  if [[ -d "${APP_TS_DIR}" ]] && compgen -G "${APP_TS_DIR}/${APP_NAME}.*.ts" >/dev/null; then
    T_NAMES+=("${APP_NAME}")
    T_TS_DIRS+=("${APP_TS_DIR}")
    T_KINDS+=("app")
    T_SRC_DIRS+=("")  # app uses APP_SRC_ROOTS
  fi
fi

# Module targets (skipped entirely in --app-only mode).
if [[ "${APP_ONLY}" != "yes" ]]; then
  if [[ ! -d "${MODULES_DIR}" ]]; then
    echo "error: modules dir not found: ${MODULES_DIR}" >&2
    exit 1
  fi
  for ts_dir in "${MODULES_DIR}"/*/ts; do
    [[ -d "$ts_dir" ]] || continue
    mod_dir="$(dirname "$ts_dir")"
    mod_name="$(basename "$mod_dir")"
    should_process "$mod_name" || continue
    # Only consider modules that actually have .ts files to update.
    compgen -G "${ts_dir}/*.ts" >/dev/null || continue
    T_NAMES+=("$mod_name")
    T_TS_DIRS+=("$ts_dir")
    T_KINDS+=("module")
    T_SRC_DIRS+=("$mod_dir")
  done
fi

if [[ ${#T_NAMES[@]} -eq 0 ]]; then
  echo "error: no translation targets found" >&2
  [[ ${#ONLY_TARGETS[@]} -gt 0 ]] && echo "       (filtered to: ${ONLY_TARGETS[*]})" >&2
  exit 1
fi

if [[ "${LIST_ONLY}" == "yes" ]]; then
  echo "Translation targets:"
  for i in "${!T_NAMES[@]}"; do echo "  ${T_NAMES[$i]} (${T_KINDS[$i]})"; done
  exit 0
fi

echo "Using lupdate: ${LUPDATE_BIN}"

# --- source gathering ------------------------------------------------------
# Populate the global `sources` array for a target.
gather_sources() {
  local kind="$1" src_dir="$2"
  sources=()
  case "$kind" in
    app)
      # Recursive over the application source roots, plus the top-level
      # src/*.cpp (Command.cpp, Security.cpp, GpgFrontend.cpp, ...).
      mapfile -t sources < <({
        find "${APP_SRC_ROOTS[@]}" -type f \
          \( -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' \
             -o -name '*.ui' \)
        find "${REPO_ROOT}/src" -maxdepth 1 -type f \
          \( -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' \
             -o -name '*.ui' \)
      } | sort)
      ;;
    module)
      # Top-level sources only (maxdepth 1) so vendored subtrees (e.g. vmime)
      # and the ts/ dir itself are excluded.
      mapfile -t sources < <(find "$src_dir" -maxdepth 1 -type f \
        \( -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' \
           -o -name '*.ui' \) | sort)
      ;;
  esac
}

# --- run lupdate per target ------------------------------------------------
fail=0
for i in "${!T_NAMES[@]}"; do
  name="${T_NAMES[$i]}"
  ts_dir="${T_TS_DIRS[$i]}"
  kind="${T_KINDS[$i]}"
  src_dir="${T_SRC_DIRS[$i]}"

  declare -a sources=()
  gather_sources "$kind" "$src_dir"
  mapfile -t ts_files < <(find "${ts_dir}" -maxdepth 1 -name '*.ts' | sort)

  if [[ ${#sources[@]} -eq 0 ]]; then
    echo "==> ${name}: no sources found, skipping"
    continue
  fi

  echo "==> ${name}: ${#sources[@]} source(s) -> ${#ts_files[@]} .ts file(s)"
  # -locations absolute keeps the existing line-number style of the committed
  # .ts files (e.g. line="49") instead of switching to relative offsets.
  #
  # The -I roots let lupdate resolve #include directives; for the app we hand
  # it the source roots, for a module its own dir.
  declare -a include_args=()
  if [[ "$kind" == "app" ]]; then
    for root in "${APP_SRC_ROOTS[@]}" "${REPO_ROOT}/src"; do include_args+=(-I "$root"); done
  else
    include_args+=(-I "$src_dir")
  fi

  if ! "${LUPDATE_BIN}" -locations absolute ${NO_OBSOLETE} \
        "${include_args[@]}" \
        "${sources[@]}" -ts "${ts_files[@]}"; then
    echo "error: lupdate failed for ${name}" >&2
    fail=1
  fi
done

exit "${fail}"
