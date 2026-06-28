#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Batch-refresh the Qt translation source (.ts) files of GpgFrontend and its
# integrated modules, keeping every target on the same set of languages.
#
# The list of supported locales lives in one place, cmake/Translations.cmake
# (GPGFRONTEND_SUPPORTED_LOCALES). The application and every module derive their
# .ts file set from it, and so does this script: for each target it makes sure a
# <base>.<locale>.ts exists for every supported locale (creating the missing
# ones) and then runs `lupdate` so newly added/changed tr() strings show up in
# every locale as <translation type="unfinished">, ready for Qt Linguist.
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
# This is offline: lupdate never touches the network. It does NOT translate
# anything; it only syncs the source strings into the .ts files.
#
# Usage: scripts/update_translations.sh [options] [target ...]
#   target             One or more target names to limit to. The application is
#                      named "GpgFrontend"; modules use their dir name (e.g.
#                      m_email). Default: the application plus every module
#                      under modules/src that has a ts/ dir.
#       --no-app       Skip the GpgFrontend application target.
#       --app-only     Process only the GpgFrontend application target.
#       --no-obsolete  Drop entries that no longer exist in the sources
#                      (passes -no-obsolete to lupdate).
#       --list         List the targets and supported locales, then exit.
#       --modules-dir DIR  Root holding the module dirs (default: modules/src).
#   -h, --help         Show this help.
#
# Environment:
#   LUPDATE            Path to the lupdate binary (auto-detected otherwise).
#   GPGFRONTEND_LOCALES  Space-separated locale list overriding the one parsed
#                      from cmake/Translations.cmake (e.g. "en_US de_DE").
#
# Examples:
#   scripts/update_translations.sh
#   scripts/update_translations.sh GpgFrontend
#   scripts/update_translations.sh m_email
#   scripts/update_translations.sh --no-obsolete GpgFrontend m_email

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
LOCALES_CMAKE="${REPO_ROOT}/cmake/Translations.cmake"
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

# --- determine the supported locale list -----------------------------------
# Parse GPGFRONTEND_SUPPORTED_LOCALES out of cmake/Translations.cmake so the
# script always tracks the same languages as the build. An env override wins.
parse_supported_locales() {
  [[ -f "${LOCALES_CMAKE}" ]] || return 1
  # Grab everything between the set(...) opening and its closing paren, then
  # keep only xx_YY tokens.
  sed -n '/set(GPGFRONTEND_SUPPORTED_LOCALES/,/)/p' "${LOCALES_CMAKE}" \
    | grep -oE '[a-z]{2}_[A-Z]{2}' | sort -u
}

declare -a LOCALES=()
if [[ -n "${GPGFRONTEND_LOCALES:-}" ]]; then
  read -r -a LOCALES <<<"${GPGFRONTEND_LOCALES}"
else
  mapfile -t LOCALES < <(parse_supported_locales)
fi
if [[ ${#LOCALES[@]} -eq 0 ]]; then
  echo "error: no supported locales found." >&2
  echo "       expected GPGFRONTEND_SUPPORTED_LOCALES in ${LOCALES_CMAKE}" >&2
  echo "       (or set GPGFRONTEND_LOCALES=\"en_US de_DE ...\")" >&2
  exit 1
fi

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

# Derive a module's .ts base name (e.g. "ModuleEMail") from an existing .ts
# file. en_US is mandatory, so match against it; fall back to any supported
# locale.
module_base_name() {
  local ts_dir="$1" loc f
  for loc in en_US "${LOCALES[@]}"; do
    for f in "${ts_dir}"/*."${loc}".ts; do
      [[ -e "$f" ]] || continue
      basename "$f" ".${loc}.ts"; return 0
    done
  done
  return 1
}

# Collect the targets to process into parallel arrays. Each target carries a
# name, the .ts base name, the dir holding its .ts files, and a "kind" that
# decides how its sources are gathered ("app" = recursive over APP_SRC_ROOTS;
# "module" = top-level sources of a single module dir).
declare -a T_NAMES=() T_BASES=() T_TS_DIRS=() T_KINDS=() T_SRC_DIRS=()

# Application target.
if [[ "${INCLUDE_APP}" == "yes" ]] && should_process "${APP_NAME}"; then
  if [[ -d "${APP_TS_DIR}" ]]; then
    T_NAMES+=("${APP_NAME}")
    T_BASES+=("${APP_NAME}")
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
    base="$(module_base_name "$ts_dir")" || {
      echo "warning: ${mod_name}: cannot determine .ts base name, skipping" >&2
      continue
    }
    T_NAMES+=("$mod_name")
    T_BASES+=("$base")
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
  echo "Supported locales: ${LOCALES[*]}"
  echo "Translation targets:"
  for i in "${!T_NAMES[@]}"; do echo "  ${T_NAMES[$i]} (${T_KINDS[$i]}, base ${T_BASES[$i]})"; done
  exit 0
fi

echo "Using lupdate: ${LUPDATE_BIN}"
echo "Supported locales: ${LOCALES[*]}"

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

# Report .ts files in a dir whose locale is not in the supported set so stray
# files (e.g. an orphaned ru_RU.ts or a mis-cased locale) get noticed.
warn_unexpected_ts() {
  local ts_dir="$1" base="$2" f loc supported
  for f in "${ts_dir}/${base}".*.ts; do
    [[ -e "$f" ]] || continue
    loc="$(basename "$f" .ts)"; loc="${loc##*.}"
    supported="no"
    for l in "${LOCALES[@]}"; do [[ "$l" == "$loc" ]] && { supported="yes"; break; }; done
    [[ "$supported" == "no" ]] && \
      echo "    note: $(basename "$f") has unsupported locale '${loc}' (left untouched)"
  done
}

# --- run lupdate per target ------------------------------------------------
fail=0
for i in "${!T_NAMES[@]}"; do
  name="${T_NAMES[$i]}"
  base="${T_BASES[$i]}"
  ts_dir="${T_TS_DIRS[$i]}"
  kind="${T_KINDS[$i]}"
  src_dir="${T_SRC_DIRS[$i]}"

  declare -a sources=()
  gather_sources "$kind" "$src_dir"

  # Desired .ts set = one file per supported locale. lupdate creates any that
  # are missing, so this is what brings every target onto the same languages.
  declare -a ts_files=() created=()
  for loc in "${LOCALES[@]}"; do
    f="${ts_dir}/${base}.${loc}.ts"
    [[ -e "$f" ]] || created+=("$(basename "$f")")
    ts_files+=("$f")
  done

  if [[ ${#sources[@]} -eq 0 ]]; then
    echo "==> ${name}: no sources found, skipping"
    continue
  fi

  echo "==> ${name}: ${#sources[@]} source(s) -> ${#ts_files[@]} .ts file(s)"
  [[ ${#created[@]} -gt 0 ]] && echo "    creating: ${created[*]}"
  warn_unexpected_ts "$ts_dir" "$base"

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
