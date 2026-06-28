#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Batch-refresh the Qt translation source (.ts) files of the integrated modules.
#
# Each module under modules/src/<name>/ keeps its translations in a ts/
# subdirectory (e.g. modules/src/m_email/ts/ModuleEMail.de_DE.ts). This script
# runs `lupdate` over each module's own sources so newly added/changed tr()
# strings show up in every locale's .ts as <translation type="unfinished">,
# ready to be filled in with Qt Linguist.
#
# Only the module's top-level sources (*.cpp, *.h, *.ui) are scanned — vendored
# subtrees such as m_email/vmime/ are intentionally skipped, matching what the
# CMake build feeds to qt_add_translations().
#
# This is offline: lupdate never touches the network. It does NOT translate
# anything; it only syncs the source strings into the .ts files.
#
# Usage: scripts/update_module_translations.sh [options] [module ...]
#   module             One or more module dir names to limit to (e.g. m_email).
#                      Default: every module under modules/src that has a ts/ dir.
#       --no-obsolete  Drop entries that no longer exist in the sources
#                      (passes -no-obsolete to lupdate).
#       --list         List the modules that would be processed, then exit.
#       --modules-dir DIR  Root holding the module dirs (default: modules/src).
#   -h, --help         Show this help.
#
# Environment:
#   LUPDATE            Path to the lupdate binary (auto-detected otherwise).
#
# Examples:
#   scripts/update_module_translations.sh
#   scripts/update_module_translations.sh m_email
#   scripts/update_module_translations.sh --no-obsolete m_email m_ver_check

set -uo pipefail

# --- locate repo root ------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# --- defaults --------------------------------------------------------------
MODULES_DIR="${REPO_ROOT}/modules/src"
NO_OBSOLETE=""
LIST_ONLY="no"
declare -a ONLY_MODULES=()

usage() { sed -n '2,/^set -uo/p' "${BASH_SOURCE[0]}" | sed '$d;s/^# \{0,1\}//'; }

# --- parse args ------------------------------------------------------------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-obsolete) NO_OBSOLETE="-no-obsolete" ;;
    --list)        LIST_ONLY="yes" ;;
    --modules-dir) MODULES_DIR="$2"; shift ;;
    -h|--help)     usage; exit 0 ;;
    -*)            echo "error: unknown option: $1" >&2; usage; exit 2 ;;
    *)             ONLY_MODULES+=("$1") ;;
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

if [[ ! -d "${MODULES_DIR}" ]]; then
  echo "error: modules dir not found: ${MODULES_DIR}" >&2
  exit 1
fi

# --- discover modules ------------------------------------------------------
should_process() {
  local name="$1"
  [[ ${#ONLY_MODULES[@]} -eq 0 ]] && return 0
  local m
  for m in "${ONLY_MODULES[@]}"; do [[ "$m" == "$name" ]] && return 0; done
  return 1
}

declare -a MODULE_DIRS=()
for ts_dir in "${MODULES_DIR}"/*/ts; do
  [[ -d "$ts_dir" ]] || continue
  mod_dir="$(dirname "$ts_dir")"
  mod_name="$(basename "$mod_dir")"
  should_process "$mod_name" || continue
  # Only consider modules that actually have .ts files to update.
  compgen -G "${ts_dir}/*.ts" >/dev/null || continue
  MODULE_DIRS+=("$mod_dir")
done

if [[ ${#MODULE_DIRS[@]} -eq 0 ]]; then
  echo "error: no modules with a ts/ directory found under ${MODULES_DIR}" >&2
  [[ ${#ONLY_MODULES[@]} -gt 0 ]] && echo "       (filtered to: ${ONLY_MODULES[*]})" >&2
  exit 1
fi

if [[ "${LIST_ONLY}" == "yes" ]]; then
  echo "Modules with translations:"
  for d in "${MODULE_DIRS[@]}"; do echo "  $(basename "$d")"; done
  exit 0
fi

echo "Using lupdate: ${LUPDATE_BIN}"

# --- run lupdate per module ------------------------------------------------
fail=0
for mod_dir in "${MODULE_DIRS[@]}"; do
  mod_name="$(basename "$mod_dir")"

  # Top-level sources only (maxdepth 1) so vendored subtrees (e.g. vmime) and
  # the ts/ dir itself are excluded.
  mapfile -t sources < <(find "$mod_dir" -maxdepth 1 -type f \
    \( -name '*.cpp' -o -name '*.cc' -o -name '*.h' -o -name '*.hpp' \
       -o -name '*.ui' \) | sort)
  mapfile -t ts_files < <(find "${mod_dir}/ts" -maxdepth 1 -name '*.ts' | sort)

  if [[ ${#sources[@]} -eq 0 ]]; then
    echo "==> ${mod_name}: no sources found, skipping"
    continue
  fi

  echo "==> ${mod_name}: ${#sources[@]} source(s) -> ${#ts_files[@]} .ts file(s)"
  # -locations absolute keeps the existing line-number style of the committed
  # .ts files (e.g. line="49") instead of switching to relative offsets.
  if ! "${LUPDATE_BIN}" -locations absolute ${NO_OBSOLETE} \
        -I "${mod_dir}" \
        "${sources[@]}" -ts "${ts_files[@]}"; then
    echo "error: lupdate failed for ${mod_name}" >&2
    fail=1
  fi
done

exit "${fail}"
