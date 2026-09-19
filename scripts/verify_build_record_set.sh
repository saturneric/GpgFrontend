#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Check that every build leg that was supposed to run actually produced a
# build record, BEFORE anything is signed for provenance.
#
# Why this is a separate gate rather than a step in the signer: the build
# matrix runs with `continue-on-error: true`, so a leg can fail and the
# release still assemble. That is a reasonable trade for a nightly and a bad
# one for a provenance claim -- signing whatever happened to arrive would
# attest to a set nobody chose, and the attestation would look identical
# whether ten legs ran or three.
#
# It also checks the thing a human would never notice: that every record
# agrees on the product build id. Two records claiming different ids means two
# different products were built in one run, and a provenance bundle covering
# both would be a statement about nothing.
#
# Usage: scripts/verify_build_record_set.sh --records DIR --expected FILE
#     --records DIR    directory searched recursively for build-record.json
#     --expected FILE  resource/provenance/expected-build-matrix.json

set -u -o pipefail

RECORDS=""
EXPECTED=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --records) RECORDS="${2:-}"; shift 2 ;;
    --expected) EXPECTED="${2:-}"; shift 2 ;;
    -h|--help) sed -n '3,22p' "$0"; exit 0 ;;
    *) echo "verify_build_record_set: unknown argument: $1" >&2; exit 2 ;;
  esac
done

if [[ -z "$RECORDS" || -z "$EXPECTED" ]]; then
  echo "verify_build_record_set: --records and --expected are required" >&2
  exit 2
fi
if [[ ! -d "$RECORDS" ]]; then
  echo "verify_build_record_set: $RECORDS: no such directory" >&2
  exit 1
fi
if [[ ! -f "$EXPECTED" ]]; then
  echo "verify_build_record_set: $EXPECTED: no such file" >&2
  exit 1
fi
command -v jq >/dev/null || { echo "verify_build_record_set: jq is required" >&2; exit 2; }

FAILURES=0
fail() { echo "  FAIL  $*" >&2; FAILURES=$((FAILURES + 1)); }

echo "verifying build records under $RECORDS"

mapfile -t FOUND < <(find "$RECORDS" -type f -name 'build-record.json' | sort)
if [[ ${#FOUND[@]} -eq 0 ]]; then
  fail "no build records at all; nothing here is signable"
fi

# leg key -> record path, so a duplicate can name both sides.
declare -A SEEN=()
BUILD_IDS=""

for record in "${FOUND[@]}"; do
  if ! jq -e . "$record" >/dev/null 2>&1; then
    fail "$record: not valid JSON"
    continue
  fi

  os="$(jq -r '.leg.os // empty' "$record")"
  flavor="$(jq -r '.leg.flavor // empty' "$record")"
  build_id="$(jq -r '.product.build_id // empty' "$record")"
  stage="$(jq -r '.artifact_stage // empty' "$record")"

  if [[ -z "$os" || -z "$flavor" || -z "$build_id" || -z "$stage" ]]; then
    fail "$record: it is missing leg.os, leg.flavor, product.build_id or artifact_stage"
    continue
  fi

  key="$os/$flavor"
  if [[ -n "${SEEN[$key]:-}" ]]; then
    fail "$key: claimed twice, by ${SEEN[$key]} and by $record"
    continue
  fi
  SEEN["$key"]="$record"
  BUILD_IDS+="$build_id"$'\n'
  echo "  ok    $key ($stage)"
done

# One product per run. The build id is derived from the product configuration
# alone -- repository, commit, ref, os, arch, flavour, build type, SDK ABI, Qt
# -- and deliberately NOT from the CI run, so legs of one product agree on it
# up to the per-leg fields. Only the non-per-leg part is compared.
distinct_commits="$(jq -r '.product.source_commit // empty' "${FOUND[@]}" 2>/dev/null | sort -u | grep -c . || true)"
if [[ "${#FOUND[@]}" -gt 0 && "$distinct_commits" -gt 1 ]]; then
  fail "these records span $distinct_commits different commits; they are not one product"
fi

# Every required leg present.
while read -r leg; do
  os="$(jq -r '.os' <<<"$leg")"
  flavor="$(jq -r '.flavor' <<<"$leg")"
  required="$(jq -r '.required' <<<"$leg")"
  key="$os/$flavor"
  if [[ -z "${SEEN[$key]:-}" ]]; then
    if [[ "$required" == "true" ]]; then
      fail "$key: required by the expected matrix and absent"
    else
      echo "  warn  $key: optional and absent"
    fi
  fi
done < <(jq -c '.legs[]' "$EXPECTED")

# And nothing that was not expected. A record from a leg nobody declared is
# not a bonus: it means the matrix and this file disagree, and the attestation
# would cover something the release notes do not describe.
for key in "${!SEEN[@]}"; do
  os="${key%%/*}"
  flavor="${key##*/}"
  if ! jq -e --arg os "$os" --arg flavor "$flavor" \
       '.legs[] | select(.os == $os and .flavor == $flavor)' \
       "$EXPECTED" >/dev/null; then
    fail "$key: produced a record but is not in the expected matrix"
  fi
done

if [[ $FAILURES -gt 0 ]]; then
  echo "verify_build_record_set: this run is not attestable" >&2
  exit 1
fi

echo "  ${#FOUND[@]} build record(s) verified"
