#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check that every build leg and stage that was supposed to run produced a
# build record, and that each record names the artifacts it was supposed to,
# BEFORE anything is signed for provenance.
#
# Why this is a separate gate rather than a step in the signer: the build
# matrix runs with `continue-on-error: true`, so a leg can fail and the
# release still assemble. That is a reasonable trade for a nightly and a bad
# one for a provenance claim -- signing whatever happened to arrive would
# attest to a set nobody chose, and the attestation would look identical
# whether ten legs ran or three.
#
# What it enforces, and why each one:
#
#   schema 2                a record without artifacts[] cannot answer whether
#                           a downloaded file came from this build
#   os + arch + flavor      architecture is part of a leg's identity; two
#                           runners can agree on os and flavour and produce
#                           binaries for different machines
#   one record per stage    unsigned and signed bytes are different bytes, and
#                           conflating them would publish a digest that cannot
#                           match the file a user downloads
#   artifact patterns       a leg that quietly stops producing its MSI fails
#                           here instead of shipping an incomplete release
#   well-formed digests     a malformed or empty digest is a record that looks
#                           checkable and is not
#   one commit              two commits in one run is not one product
#
# Usage: scripts/verify_build_record_set.sh --records DIR --expected FILE
#     --records DIR    directory searched recursively for build-record-*.json
#     --expected FILE  resource/provenance/expected-build-matrix.json

set -u -o pipefail

RECORDS=""
EXPECTED=""

while [ $# -gt 0 ]; do
  case "$1" in
    --records) RECORDS="${2:-}"; shift 2 ;;
    --expected) EXPECTED="${2:-}"; shift 2 ;;
    -h|--help) sed -n '3,36p' "$0"; exit 0 ;;
    *) echo "verify_build_record_set: unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [ -z "$RECORDS" ] || [ -z "$EXPECTED" ]; then
  echo "verify_build_record_set: --records and --expected are required" >&2
  exit 2
fi
[ -d "$RECORDS" ] || { echo "verify_build_record_set: $RECORDS: no such directory" >&2; exit 1; }
[ -f "$EXPECTED" ] || { echo "verify_build_record_set: $EXPECTED: no such file" >&2; exit 1; }
command -v jq >/dev/null || { echo "verify_build_record_set: jq is required" >&2; exit 2; }

FAILURES=0
fail() { echo "  FAIL  $*" >&2; FAILURES=$((FAILURES + 1)); }

# The expected-matrix walk below runs inside pipelines, and a pipeline stage is
# a subshell: a counter incremented there is lost when it exits. So failures
# from those loops are appended to a file instead. This is the one place that
# matters, and getting it wrong would make the gate silently pass.
GF_GATE_FAILFILE="$(mktemp)"
export GF_GATE_FAILFILE
cleanup() { rm -f "$GF_GATE_FAILFILE"; }
trap cleanup EXIT

echo "verifying build records under $RECORDS"

FOUND="$(find "$RECORDS" -type f -name 'build-record-*.json' | sort)"
if [ -z "$FOUND" ]; then
  fail "no build records at all; nothing here is signable"
fi

# "<os>/<arch>/<flavor>/<stage> <path>" per line, so a duplicate can name both
# sides. A plain string rather than an associative array: macOS ships bash 3.2.
SEEN=""
COMMITS=""
COUNT=0

for record in $FOUND; do
  if ! jq -e . "$record" >/dev/null 2>&1; then
    fail "$record: not valid JSON"
    continue
  fi

  schema="$(jq -r '.schema // empty' "$record")"
  if [ "$schema" != "2" ]; then
    fail "$record: schema is \"$schema\", and this gate requires 2 -- a record
        without artifact digests cannot say whether a download came from it"
    continue
  fi

  os="$(jq -r '.leg.os // empty' "$record")"
  arch="$(jq -r '.leg.arch // empty' "$record")"
  flavor="$(jq -r '.leg.flavor // empty' "$record")"
  stage="$(jq -r '.artifact_stage // empty' "$record")"
  build_id="$(jq -r '.product.build_id // empty' "$record")"
  commit="$(jq -r '.product.source_commit // empty' "$record")"

  if [ -z "$os" ] || [ -z "$arch" ] || [ -z "$flavor" ] || [ -z "$stage" ] \
     || [ -z "$build_id" ] || [ -z "$commit" ]; then
    fail "$record: it is missing leg.os, leg.arch, leg.flavor, artifact_stage,
        product.build_id or product.source_commit"
    continue
  fi

  n_artifacts="$(jq -r '.artifacts | length' "$record" 2>/dev/null || echo 0)"
  if [ "$n_artifacts" -eq 0 ]; then
    fail "$os/$arch/$flavor ($stage): it records no artifacts, so it cannot
        identify any bytes this leg produced"
    continue
  fi

  # Every artifact well-formed. A digest that is not 64 lower-case hex, or a
  # size that is absent or zero, is a record that looks checkable and is not.
  bad="$(jq -r '
    .artifacts[]
    | select((.name // "") == ""
             or ((.sha256 // "") | test("^[0-9a-f]{64}$") | not)
             or ((.size // 0) <= 0))
    | .name // "<unnamed>"' "$record")"
  if [ -n "$bad" ]; then
    fail "$os/$arch/$flavor ($stage): these artifact entries are malformed:
        $(printf '%s' "$bad" | tr '\n' ' ')"
    continue
  fi

  dupes="$(jq -r '.artifacts | group_by(.name) | map(select(length > 1))
                  | .[][0].name' "$record")"
  if [ -n "$dupes" ]; then
    fail "$os/$arch/$flavor ($stage): these artifacts are recorded twice:
        $(printf '%s' "$dupes" | tr '\n' ' ')"
    continue
  fi

  key="$os/$arch/$flavor/$stage"
  previous="$(printf '%s\n' "$SEEN" | awk -v k="$key" '$1 == k {print $2}')"
  if [ -n "$previous" ]; then
    fail "$key: claimed twice, by $previous and by $record"
    continue
  fi
  SEEN="$SEEN
$key $record"
  COMMITS="$COMMITS
$commit"
  COUNT=$((COUNT + 1))
  echo "  ok    $key ($n_artifacts artifact(s))"
done

# One product per run. The build id is derived from the product configuration
# alone and deliberately not from the CI run, so legs of one product agree on
# the source commit.
n_commits="$(printf '%s\n' "$COMMITS" | grep -c . || true)"
distinct="$(printf '%s\n' "$COMMITS" | grep . | sort -u | grep -c . || true)"
if [ "$n_commits" -gt 0 ] && [ "$distinct" -gt 1 ]; then
  fail "these records span $distinct different commits; they are not one product"
fi

# Every required leg and stage present, with the artifacts it promised.
jq -c '.legs[]' "$EXPECTED" | while IFS= read -r leg; do
  os="$(printf '%s' "$leg" | jq -r '.os')"
  arch="$(printf '%s' "$leg" | jq -r '.arch')"
  flavor="$(printf '%s' "$leg" | jq -r '.flavor')"
  required="$(printf '%s' "$leg" | jq -r '.required')"

  printf '%s' "$leg" | jq -c '.stages[]' | while IFS= read -r stage_spec; do
    stage="$(printf '%s' "$stage_spec" | jq -r '.stage')"
    key="$os/$arch/$flavor/$stage"
    record="$(printf '%s\n' "$SEEN" | awk -v k="$key" '$1 == k {print $2}')"

    if [ -z "$record" ]; then
      if [ "$required" = "true" ]; then
        echo "  FAIL  $key: required by the expected matrix and absent" >&2
        echo "FAILED" >> "$GF_GATE_FAILFILE"
      else
        echo "  warn  $key: optional and absent"
      fi
      continue
    fi

    printf '%s' "$stage_spec" | jq -r '.artifact_patterns[]' \
    | while IFS= read -r pattern; do
        matched=0
        for name in $(jq -r '.artifacts[].name' "$record"); do
          case "$name" in
            $pattern) matched=1; break ;;
          esac
        done
        if [ "$matched" -eq 0 ]; then
          echo "  FAIL  $key: nothing matching \"$pattern\" was recorded; this
        stage is supposed to produce one" >&2
          echo "FAILED" >> "$GF_GATE_FAILFILE"
        fi
      done
  done
done

# And nothing that was not expected. A record from a leg nobody declared means
# the matrix and this file disagree, and the attestation would cover something
# the release notes do not describe.
printf '%s\n' "$SEEN" | grep . | while IFS=' ' read -r key record; do
  os="${key%%/*}"; rest="${key#*/}"
  arch="${rest%%/*}"; rest="${rest#*/}"
  flavor="${rest%%/*}"; stage="${rest#*/}"
  if ! jq -e --arg os "$os" --arg arch "$arch" --arg flavor "$flavor" \
         --arg stage "$stage" \
         '.legs[] | select(.os == $os and .arch == $arch
                           and .flavor == $flavor)
                  | .stages[] | select(.stage == $stage)' \
         "$EXPECTED" >/dev/null; then
    echo "  FAIL  $key: produced a record but is not in the expected matrix" >&2
    echo "FAILED" >> "$GF_GATE_FAILFILE"
  fi
done

if [ -s "$GF_GATE_FAILFILE" ]; then
  FAILURES=$((FAILURES + $(grep -c FAILED "$GF_GATE_FAILFILE")))
fi

if [ "$FAILURES" -gt 0 ]; then
  echo "verify_build_record_set: this run is not attestable" >&2
  exit 1
fi

echo "  $COUNT build record(s) verified"
