#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Tests for the build-record writer and the provenance completeness gate.
#
# These are shell tools that only ever run inside CI, which is the worst place
# to find out they are wrong: a mistake here either blocks a release or, far
# worse, lets one through with provenance that cannot be checked. So they get
# a harness that runs anywhere, against synthetic records, in under a second.
#
# Usage: scripts/test_build_records.sh

set -u -o pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(dirname "$HERE")"
WRITE="$HERE/write_build_record.sh"
GATE="$HERE/verify_build_record_set.sh"
EXPECTED="$REPO/resource/provenance/expected-build-matrix.json"

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

PASSED=0
FAILED=0

ok()   { echo "  ok    $*"; PASSED=$((PASSED + 1)); }
bad()  { echo "  FAIL  $*" >&2; FAILED=$((FAILED + 1)); }

check() {
  local what="$1"; shift
  if "$@" >/dev/null 2>&1; then ok "$what"; else bad "$what"; fi
}
check_fails() {
  local what="$1"; shift
  if "$@" >/dev/null 2>&1; then bad "$what (it succeeded, and should not)"
  else ok "$what"; fi
}

# A record for one leg and stage, with artifacts made from the named contents.
# `write_record <dir> <os> <arch> <flavor> <stage> <name>=<content> ...`
write_record() {
  local dir="$1" os="$2" arch="$3" flavor="$4" stage="$5"; shift 5
  local art_dir="$dir/$os-$arch-$flavor-$stage"
  mkdir -p "$art_dir"
  local args=()
  local pair name content
  for pair in "$@"; do
    name="${pair%%=*}"
    content="${pair#*=}"
    printf '%s' "$content" > "$art_dir/$name"
    args[${#args[@]}]="--artifact"
    args[${#args[@]}]="$art_dir/$name"
  done
  "$WRITE" \
    --output "$dir/build-record-$os-$arch-$flavor-$stage.json" \
    --stage "$stage" --build-id gfb1-test --repository o/r \
    --commit deadbeef --ref refs/heads/main --build-type RelWithDebInfo \
    --os "$os" --arch "$arch" --flavor "$flavor" \
    --ci-run-id 1 --ci-run-attempt 1 --ci-actor t --ci-workflow-ref w \
    "${args[@]}"
}

# A complete, passing set: every leg and stage the expected matrix requires.
populate_full() {
  local dir="$1"
  write_record "$dir" ubuntu-22.04     x86_64 installed unsigned "GpgFrontend-a-linux-x86_64.AppImage=linux-i"
  write_record "$dir" ubuntu-22.04     x86_64 portable  unsigned "GpgFrontend-a-linux-x86_64-portable.AppImage=linux-p"
  write_record "$dir" ubuntu-24.04-arm arm64  installed unsigned "GpgFrontend-a-linux-arm64.AppImage=arm-i"
  write_record "$dir" ubuntu-24.04-arm arm64  portable  unsigned "GpgFrontend-a-linux-arm64-portable.AppImage=arm-p"
  write_record "$dir" windows-2022     x86_64 installed unsigned "GpgFrontend-a-windows-x86_64.msi=msi"
  write_record "$dir" windows-2022     x86_64 portable  unsigned "GpgFrontend-a-windows-x86_64-portable.zip=zip"
  local m
  for m in "macos-15-intel x86_64" "macos-15 arm64" "macos-26-intel x86_64" "macos-26 arm64"; do
    set -- $m
    write_record "$dir" "$1" "$2" installed unsigned        "GpgFrontend.app.zip=unsigned-$1"
    write_record "$dir" "$1" "$2" installed signed-notarized "GpgFrontend-$1.dmg=signed-$1"
  done
}

echo "=== 1. a modified artifact changes its recorded digest ==="
D="$WORK/t1"; mkdir -p "$D"
write_record "$D" ubuntu-22.04 x86_64 installed unsigned "GpgFrontend-a-linux-x86_64.AppImage=original" >/dev/null
FIRST="$(jq -r '.artifacts[0].sha256' "$D/build-record-ubuntu-22.04-x86_64-installed-unsigned.json")"
FIRST_SIZE="$(jq -r '.artifacts[0].size' "$D/build-record-ubuntu-22.04-x86_64-installed-unsigned.json")"
write_record "$D" ubuntu-22.04 x86_64 installed unsigned "GpgFrontend-a-linux-x86_64.AppImage=tampered" >/dev/null
SECOND="$(jq -r '.artifacts[0].sha256' "$D/build-record-ubuntu-22.04-x86_64-installed-unsigned.json")"
if [ "$FIRST" != "$SECOND" ]; then ok "one byte changed, the digest changed"
else bad "the digest did not change when the artifact did"; fi
if [ "$FIRST_SIZE" -eq 8 ]; then ok "the size is the artifact's real size"
else bad "size was $FIRST_SIZE, expected 8"; fi
# And the digest is the real SHA-256, not something this script also computes
# wrongly: compare against the system hasher directly.
REAL="$(printf 'original' | sha256sum | awk '{print $1}')"
if [ "$FIRST" = "$REAL" ]; then ok "the digest is the file's actual SHA-256"
else bad "recorded $FIRST, sha256sum says $REAL"; fi

echo "=== 2. a missing expected artifact fails, at both steps ==="
check_fails "the writer refuses an artifact that is not there" \
  "$WRITE" --output "$WORK/t2.json" --stage unsigned --build-id b \
  --repository o/r --commit c --ref r --build-type t \
  --os ubuntu-22.04 --arch x86_64 --flavor installed \
  --artifact "$WORK/does-not-exist"
check_fails "the writer refuses a record with no artifacts at all" \
  "$WRITE" --output "$WORK/t2b.json" --stage unsigned --build-id b \
  --repository o/r --commit c --ref r --build-type t \
  --os ubuntu-22.04 --arch x86_64 --flavor installed
D="$WORK/t2c"; mkdir -p "$D"
populate_full "$D" >/dev/null
# The MSI leg records something, but not an msi.
write_record "$D" windows-2022 x86_64 installed unsigned "notes.txt=oops" >/dev/null
check_fails "the gate refuses a stage whose expected artifact is absent" \
  "$GATE" --records "$D" --expected "$EXPECTED"
D="$WORK/t2d"; mkdir -p "$D"
populate_full "$D" >/dev/null
rm -f "$D/build-record-windows-2022-x86_64-installed-unsigned.json"
check_fails "the gate refuses a missing required leg" \
  "$GATE" --records "$D" --expected "$EXPECTED"

echo "=== 3. several artifacts from one leg are all recorded ==="
D="$WORK/t3"; mkdir -p "$D"
write_record "$D" windows-2022 x86_64 installed unsigned \
  "GpgFrontend-a-windows-x86_64.msi=msi-bytes" \
  "GpgFrontend-a-windows-x86_64-portable.zip=zip-bytes" >/dev/null
R="$D/build-record-windows-2022-x86_64-installed-unsigned.json"
N="$(jq -r '.artifacts | length' "$R")"
if [ "$N" -eq 2 ]; then ok "both artifacts recorded"; else bad "recorded $N, expected 2"; fi
U="$(jq -r '[.artifacts[].sha256] | unique | length' "$R")"
if [ "$U" -eq 2 ]; then ok "each has its own digest"; else bad "digests collided"; fi
SORTED="$(jq -r '[.artifacts[].name] == ([.artifacts[].name] | sort)' "$R")"
if [ "$SORTED" = "true" ]; then ok "artifacts are in a deterministic order"
else bad "artifact order is not deterministic"; fi
check_fails "two files with one name are refused rather than silently merged" \
  bash -c 'mkdir -p "$1/a" "$1/b"; echo one > "$1/a/same.msi"; echo two > "$1/b/same.msi";
           "$2" --output "$1/dup.json" --stage unsigned --build-id b --repository o/r \
             --commit c --ref r --build-type t --os windows-2022 --arch x86_64 \
             --flavor installed --artifact "$1/a/same.msi" --artifact "$1/b/same.msi"' \
  _ "$WORK/t3" "$WRITE"

echo "=== 4. architecture distinguishes otherwise identical legs ==="
D="$WORK/t4"; mkdir -p "$D"
populate_full "$D" >/dev/null
check "the full set passes" "$GATE" --records "$D" --expected "$EXPECTED"
# Two legs that agree on os and flavour and differ only by architecture: if
# arch were not part of the key, one would look like a duplicate of the other
# and the second would be reported as claimed twice.
A="$(jq -r '.leg.arch' "$D/build-record-macos-15-arm64-installed-unsigned.json")"
B="$(jq -r '.leg.arch' "$D/build-record-macos-15-intel-x86_64-installed-unsigned.json")"
if [ "$A" = "arm64" ] && [ "$B" = "x86_64" ]; then
  ok "the two macOS legs record different architectures"
else bad "arch was '$A' and '$B'"; fi
D="$WORK/t4b"; mkdir -p "$D"
populate_full "$D" >/dev/null
# Relabel the arm64 macOS leg as x86_64: now two records claim one key, and
# the leg that should have covered arm64 is gone.
jq '.leg.arch = "x86_64"' "$D/build-record-macos-15-arm64-installed-unsigned.json" \
  > "$D/tmp" && mv "$D/tmp" "$D/build-record-macos-15-arm64-installed-unsigned.json"
check_fails "the gate notices a leg claiming the wrong architecture" \
  "$GATE" --records "$D" --expected "$EXPECTED"

echo "=== 5. unsigned and signed stages cannot be conflated ==="
D="$WORK/t5"; mkdir -p "$D"
populate_full "$D" >/dev/null
UNS="$D/build-record-macos-15-arm64-installed-unsigned.json"
SIG="$D/build-record-macos-15-arm64-installed-signed-notarized.json"
UD="$(jq -r '.artifacts[0].sha256' "$UNS")"
SD="$(jq -r '.artifacts[0].sha256' "$SIG")"
if [ "$UD" != "$SD" ]; then ok "the two stages record different bytes"
else bad "signed and unsigned recorded the same digest"; fi
UN="$(jq -r '.artifacts[0].name' "$UNS")"
SN="$(jq -r '.artifacts[0].name' "$SIG")"
if [ "$UN" = "GpgFrontend.app.zip" ] && [ "${SN##*.}" = "dmg" ]; then
  ok "each stage names the artifact it actually produced"
else bad "unsigned named '$UN' and signed named '$SN'"; fi
# Deleting the signed stage must fail: an unsigned digest does not identify
# the file a user downloads, so "the unsigned record is there" is not enough.
D="$WORK/t5b"; mkdir -p "$D"
populate_full "$D" >/dev/null
rm -f "$D/build-record-macos-15-arm64-installed-signed-notarized.json"
check_fails "an unsigned record cannot stand in for the signed stage" \
  "$GATE" --records "$D" --expected "$EXPECTED"
# And a record whose stage is not one the matrix declares is refused rather
# than ignored.
D="$WORK/t5c"; mkdir -p "$D"
populate_full "$D" >/dev/null
write_record "$D" macos-15 arm64 installed made-up-stage "GpgFrontend-x.dmg=x" >/dev/null
check_fails "an undeclared stage is refused" \
  "$GATE" --records "$D" --expected "$EXPECTED"

echo "=== extra: malformed records are refused ==="
D="$WORK/t6"; mkdir -p "$D"
populate_full "$D" >/dev/null
jq '.artifacts[0].sha256 = "not-a-digest"' "$UNS" > "$D/build-record-macos-15-arm64-installed-unsigned.json"
check_fails "a malformed digest is refused" \
  "$GATE" --records "$D" --expected "$EXPECTED"
D="$WORK/t7"; mkdir -p "$D"
populate_full "$D" >/dev/null
jq '.schema = 1 | del(.artifacts)' "$UNS" > "$D/build-record-macos-15-arm64-installed-unsigned.json"
check_fails "a pre-artifacts schema 1 record is refused" \
  "$GATE" --records "$D" --expected "$EXPECTED"
D="$WORK/t8"; mkdir -p "$D"
populate_full "$D" >/dev/null
jq '.product.source_commit = "cafebabe"' "$UNS" > "$D/build-record-macos-15-arm64-installed-unsigned.json"
check_fails "records spanning two commits are refused" \
  "$GATE" --records "$D" --expected "$EXPECTED"

echo
echo "passed=$PASSED failed=$FAILED"
[ "$FAILED" -eq 0 ] || exit 1
