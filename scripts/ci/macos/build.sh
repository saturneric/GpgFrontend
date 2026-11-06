#!/bin/bash
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
#
# This file is part of GpgFrontend.
#
# GpgFrontend is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# GpgFrontend is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Build the UNSIGNED macOS app with Xcode and copy it to build/package/.
#
#   build.sh --flavor installed   Developer ID: GnuPG + rPGP, ccache-wrapped
#   build.sh --flavor sandbox     App Store: rPGP only
#
# Environment: BUILD_TYPE, GPGFRONTEND_BUILD_MODE; CCACHE_EPOCH for installed.
#
# No signing identity, team id or provisioning profile is ever passed: the
# build jobs have none, by design. The bundle is signed by sign-apple.yml.

set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
: "${BUILD_TYPE:?}" "${GPGFRONTEND_BUILD_MODE:?}"

FLAVOR=""
while [ $# -gt 0 ]; do
  case "$1" in
    --flavor) FLAVOR="$2"; shift 2 ;;
    *) echo "unknown argument: $1" >&2; exit 2 ;;
  esac
done
case "$FLAVOR" in
  installed|sandbox) ;;
  *) echo "--flavor installed|sandbox is required" >&2; exit 2 ;;
esac

BUILD="$ROOT/build"

MACOS_MAJOR=$(sw_vers -productVersion | cut -d. -f1)
MACOS_MINOR=$(sw_vers -productVersion | cut -d. -f2)
case "$MACOS_MAJOR" in
  13|14|15|26) DEPLOY_TARGET="${MACOS_MAJOR}.0" ;;
  *) DEPLOY_TARGET="${MACOS_MAJOR}.${MACOS_MINOR}" ;;
esac
echo "Set MacOS Deployment Target: $DEPLOY_TARGET"

CMAKE_ARGS=(
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOY_TARGET"
  -DGPGFRONTEND_BUILD_MODE="$GPGFRONTEND_BUILD_MODE"
  -DGPGFRONTEND_BUILD_FLAVOR="$FLAVOR"
)

# DEVELOPMENT_TEAM and PROVISIONING_PROFILE_SPECIFIER are cleared as well as
# the identity: src/CMakeLists.txt bakes a profile specifier into the project,
# and Xcode would otherwise try to resolve a profile this job has no business
# holding.
XCODE_SETTINGS=(
  CODE_SIGN_IDENTITY=""
  CODE_SIGNING_REQUIRED=NO
  CODE_SIGNING_ALLOWED=NO
  DEVELOPMENT_TEAM=""
  PROVISIONING_PROFILE_SPECIFIER=""
)

if [ "$FLAVOR" = installed ]; then
  : "${CCACHE_EPOCH:?}"

  # The GnuPG engine links a static GpgME. The sandbox flavour is rPGP-only.
  echo "::group::Build GpgME"
  (
    cd "$ROOT/third_party/gpgme"
    export CC="ccache gcc"
    export CXX="ccache g++"
    ./autogen.sh
    mkdir -p build && cd build
    ../configure --enable-static \
      --disable-shared \
      --enable-silent-rules \
      --disable-dependency-tracking \
      --enable-languages=cl \
      --disable-gpgconf-test \
      --disable-gpg-test \
      --disable-gpgsm-test \
      --disable-g13-test
    make -j"$(sysctl -n hw.logicalcpu)"
    sudo make install
  )
  ccache -s -v
  echo "::endgroup::"

  # Homebrew's openssl@3 is keg-only, so it is not on the default search path
  # and find_package(OpenSSL) would pick up the unusable LibreSSL headers that
  # ship with macOS. vmime's TLS backend needs the real one.
  OPENSSL_ROOT="$(brew --prefix openssl@3)"
  echo "Using OpenSSL from: $OPENSSL_ROOT"
  CMAKE_ARGS+=(-DOPENSSL_ROOT_DIR="$OPENSSL_ROOT" -DGPGFRONTEND_LINK_GPGME_INTO_CORE=ON)

  # ccache, as xcodebuild build settings rather than a CMake launcher, which
  # the Xcode generator ignores. CC/CXX go through a wrapper; LD/LDPLUSPLUS
  # stay the plain driver so links never see a compile-only flag.
  # -fno-pch-timestamp: Xcode precompiles the prefix header itself, and a .pch
  # restored from the cache would otherwise be refused against this checkout's
  # header mtimes. The index store is an IDE feature and only adds an
  # uncacheable output.
  #
  # The *_DEPLOYMENT_TARGET settings are blanked for Xcode 26: with these
  # compiler overrides it exported every platform's default deployment target
  # into script phases, and AUTOMOC's moc_predefs step -- the real clang++
  # -dM -E, no -target -- then dies with "conflicting deployment targets ...
  # present in environment". Clang treats an empty value as unset. This app is
  # macOS only; MACOSX_DEPLOYMENT_TARGET is untouched and still comes from
  # CMake.
  WRAP="${RUNNER_TEMP:?}/ccache-xcode"
  mkdir -p "$WRAP"
  REAL_CC="$(xcrun -f clang)"
  REAL_CXX="$(xcrun -f clang++)"
  # shellcheck disable=SC2016 # "$@" belongs to the generated wrapper
  printf '#!/bin/sh\nexec ccache "%s" -Xclang -fno-pch-timestamp "$@"\n' "$REAL_CC" > "$WRAP/clang"
  # shellcheck disable=SC2016
  printf '#!/bin/sh\nexec ccache "%s" -Xclang -fno-pch-timestamp "$@"\n' "$REAL_CXX" > "$WRAP/clang++"
  chmod +x "$WRAP/clang" "$WRAP/clang++"
  XCODE_SETTINGS+=(
    CC="$WRAP/clang"
    CXX="$WRAP/clang++"
    LD="$REAL_CC"
    LDPLUSPLUS="$REAL_CXX"
    COMPILER_INDEX_STORE_ENABLE=NO
    IPHONEOS_DEPLOYMENT_TARGET=
    TVOS_DEPLOYMENT_TARGET=
    WATCHOS_DEPLOYMENT_TARGET=
    XROS_DEPLOYMENT_TARGET=
    DRIVERKIT_DEPLOYMENT_TARGET=
  )
  # Counted from here, so the report below is this build's and not GpgME's as
  # well.
  ccache -z
fi

echo "::group::Build GpgFrontend"
cmake -B "$BUILD" -S "$ROOT" -G Xcode "${CMAKE_ARGS[@]}"

cd "$BUILD"
xcodebuild -project "$BUILD/GpgFrontend.xcodeproj" \
  -scheme GpgFrontend \
  -configuration "$BUILD_TYPE" \
  -archivePath "$BUILD/GpgFrontend.xcarchive" \
  archive \
  "${XCODE_SETTINGS[@]}"
echo "::endgroup::"

if [ "$FLAVOR" = installed ]; then
  ccache -s -v
  # Keep only what this build used, so the saved cache stays the working set
  # instead of growing toward max-size run after run.
  ccache --evict-older-than "$(( $(date +%s) - CCACHE_EPOCH + 60 ))s"
  ccache -s
fi

# Skip -exportArchive: the generated ExportOptions.plist is method=developer-id
# with signingStyle=manual, so exporting demands a Developer ID identity in the
# keychain. The app is copied out of the archive instead, so macdeployqt can
# deploy into it.
mkdir -p "$BUILD/package"
cp -R "$BUILD/GpgFrontend.xcarchive/Products/Applications/GpgFrontend.app" \
  "$BUILD/package/"
