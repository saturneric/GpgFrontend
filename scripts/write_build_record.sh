#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Write one build record: what this CI leg produced, and the exact bytes of it.
#
# A build record answers two questions, and it is worth being precise about
# which is which because they need different evidence:
#
#   which source, build and module set produced this build
#       -> product.*, modules[]  (metadata, and module ENTRY identity)
#
#   is this file I downloaded one of the bytes that build produced
#       -> artifacts[]           (a digest of the file itself)
#
# The second is the one this script exists for. Module `mode`/`value` entries
# bind a module's entry native and say nothing about the AppImage, the MSI or
# the DMG a user actually downloads; artifacts[] closes that gap and does not
# replace them.
#
# ## The stage rule, which is the whole design
#
# An artifact digest is only meaningful together with the stage it was taken
# at. Windows Authenticode signing and macOS codesigning both REWRITE the file,
# so a digest taken before signing does not identify the file afterwards.
# Recording one and calling it the release artifact would be worse than
# recording nothing: it invites a user to check a hash that cannot match.
#
# So each record covers ONE stage, names it, and hashes the artifacts as they
# exist at that stage. A leg that produces both unsigned and signed bytes
# writes two records. Nothing mutates an artifact after it has been hashed
# into a record -- the caller is responsible for the ordering, and the
# completeness gate checks that the stages it expects are all present.
#
# Usage:
#   scripts/write_build_record.sh --output FILE --stage STAGE \
#       --build-id ID --repository OWNER/REPO --commit SHA --ref REF \
#       --build-type TYPE --os RUNNER_OS --arch ARCH --flavor FLAVOR \
#       [--modules-from FILE] [--ci-run-id N] [--ci-run-attempt N] \
#       [--ci-actor NAME] [--ci-workflow-ref REF] \
#       --artifact PATH [--artifact PATH ...]
#
#   --modules-from FILE   output of `gf_module_packager verify-module-set
#                         --print-bindings`, captured while the tool existed
#   --artifact PATH       a file to hash, given as the exact path that is
#                         uploaded or released -- not an equivalent copy

set -u -o pipefail

OUTPUT=""
STAGE=""
BUILD_ID=""
REPOSITORY=""
COMMIT=""
REF=""
BUILD_TYPE=""
LEG_OS=""
LEG_ARCH=""
LEG_FLAVOR=""
MODULES_FROM=""
CI_RUN_ID=""
CI_RUN_ATTEMPT=""
CI_ACTOR=""
CI_WORKFLOW_REF=""
ARTIFACTS=()

while [ $# -gt 0 ]; do
  case "$1" in
    --output) OUTPUT="${2:-}"; shift 2 ;;
    --stage) STAGE="${2:-}"; shift 2 ;;
    --build-id) BUILD_ID="${2:-}"; shift 2 ;;
    --repository) REPOSITORY="${2:-}"; shift 2 ;;
    --commit) COMMIT="${2:-}"; shift 2 ;;
    --ref) REF="${2:-}"; shift 2 ;;
    --build-type) BUILD_TYPE="${2:-}"; shift 2 ;;
    --os) LEG_OS="${2:-}"; shift 2 ;;
    --arch) LEG_ARCH="${2:-}"; shift 2 ;;
    --flavor) LEG_FLAVOR="${2:-}"; shift 2 ;;
    --modules-from) MODULES_FROM="${2:-}"; shift 2 ;;
    --ci-run-id) CI_RUN_ID="${2:-}"; shift 2 ;;
    --ci-run-attempt) CI_RUN_ATTEMPT="${2:-}"; shift 2 ;;
    --ci-actor) CI_ACTOR="${2:-}"; shift 2 ;;
    --ci-workflow-ref) CI_WORKFLOW_REF="${2:-}"; shift 2 ;;
    --artifact) ARTIFACTS[${#ARTIFACTS[@]}]="${2:-}"; shift 2 ;;
    -h|--help) sed -n '3,45p' "$0"; exit 0 ;;
    *) echo "write_build_record: unknown argument: $1" >&2; exit 2 ;;
  esac
done

die() { echo "write_build_record: $*" >&2; exit 1; }

for pair in "OUTPUT:--output" "STAGE:--stage" "BUILD_ID:--build-id" \
            "REPOSITORY:--repository" "COMMIT:--commit" "REF:--ref" \
            "BUILD_TYPE:--build-type" "LEG_OS:--os" "LEG_ARCH:--arch" \
            "LEG_FLAVOR:--flavor"; do
  var="${pair%%:*}"
  flag="${pair##*:}"
  eval "value=\"\${$var}\""
  [ -n "$value" ] || die "$flag is required"
done

command -v jq >/dev/null || die "jq is required"

# Every hasher spells its output differently; normalise to bare hex.
sha256_of() {
  if command -v sha256sum >/dev/null; then
    sha256sum "$1" | awk '{print $1}'
  elif command -v shasum >/dev/null; then
    shasum -a 256 "$1" | awk '{print $1}'
  else
    die "no sha256sum or shasum on this machine"
  fi
}

size_of() {
  # BSD stat and GNU stat disagree about everything except that both exist.
  if stat -f%z "$1" >/dev/null 2>&1; then
    stat -f%z "$1"
  else
    stat -c%s "$1"
  fi
}

[ ${#ARTIFACTS[@]} -gt 0 ] || die \
  "no --artifact was given; a record with no artifact digests cannot answer
  whether a downloaded file came from this build, which is most of the point"

# Fail closed on every way an artifact can be wrong, and say which.
ARTIFACT_JSON="[]"
SEEN_NAMES=""
for path in "${ARTIFACTS[@]}"; do
  [ -n "$path" ] || die "an empty --artifact path was given"
  [ -e "$path" ] || die "$path: no such file"
  [ -f "$path" ] || die "$path: not a regular file"
  [ -r "$path" ] || die "$path: not readable"

  name="$(basename "$path")"
  if printf '%s\n' "$SEEN_NAMES" | grep -qxF "$name"; then
    # Two files with one name cannot both be identified by that name in the
    # record, and silently keeping one would make the record a lie about the
    # other.
    die "$name: recorded twice, from two different paths"
  fi
  SEEN_NAMES="$SEEN_NAMES
$name"

  digest="$(sha256_of "$path")" || die "$path: could not be hashed"
  case "$digest" in
    [0-9a-f]*) ;;
    *) die "$path: its digest is not hex: $digest" ;;
  esac
  [ ${#digest} -eq 64 ] || die "$path: its digest is not 256 bits"

  size="$(size_of "$path")" || die "$path: its size could not be read"
  [ "$size" -gt 0 ] || die "$path: it is empty"

  ARTIFACT_JSON="$(printf '%s' "$ARTIFACT_JSON" | jq \
    --arg name "$name" --arg sha256 "$digest" --argjson size "$size" \
    '. + [{name: $name, sha256: $sha256, size: $size}]')" \
    || die "$path: its record entry could not be built"
done

# Sorted by name: the same leg produces the same bytes in the same order
# whatever the shell happened to glob first.
ARTIFACT_JSON="$(printf '%s' "$ARTIFACT_JSON" | jq 'sort_by(.name)')"

MODULE_JSON="[]"
if [ -n "$MODULES_FROM" ]; then
  [ -f "$MODULES_FROM" ] || die "$MODULES_FROM: no such file"
  MODULE_JSON="$(sed -n 's/^binding //p' "$MODULES_FROM" \
    | jq -R 'split(" ") | {module: .[0], mode: .[1], value: .[2]}' \
    | jq -s 'sort_by(.module)')" || die "$MODULES_FROM: could not be parsed"
fi

mkdir -p "$(dirname "$OUTPUT")"

# -S for sorted keys: the record is regenerated by several different steps and
# a stable shape makes two of them diffable.
jq -S -n \
  --arg stage "$STAGE" \
  --arg build_id "$BUILD_ID" \
  --arg repository "$REPOSITORY" \
  --arg commit "$COMMIT" \
  --arg ref "$REF" \
  --arg build_type "$BUILD_TYPE" \
  --arg os "$LEG_OS" \
  --arg arch "$LEG_ARCH" \
  --arg flavor "$LEG_FLAVOR" \
  --arg run_id "$CI_RUN_ID" \
  --arg run_attempt "$CI_RUN_ATTEMPT" \
  --arg actor "$CI_ACTOR" \
  --arg workflow_ref "$CI_WORKFLOW_REF" \
  --argjson modules "$MODULE_JSON" \
  --argjson artifacts "$ARTIFACT_JSON" \
  '{
    schema: 2,
    artifact_stage: $stage,
    product: {
      build_id: $build_id,
      repository: $repository,
      source_commit: $commit,
      ref: $ref,
      build_type: $build_type
    },
    leg: { os: $os, arch: $arch, flavor: $flavor },
    modules: $modules,
    artifacts: $artifacts,
    ci: {
      run_id: $run_id,
      run_attempt: $run_attempt,
      actor: $actor,
      workflow_ref: $workflow_ref
    }
  }' > "$OUTPUT" || die "$OUTPUT: could not be written"

echo "wrote $OUTPUT"
jq -r '"  stage   " + .artifact_stage,
       "  leg     " + .leg.os + "/" + .leg.arch + "/" + .leg.flavor,
       "  modules \(.modules | length)",
       (.artifacts[] | "  artifact \(.name)  \(.sha256)  \(.size) bytes")' \
  "$OUTPUT"
