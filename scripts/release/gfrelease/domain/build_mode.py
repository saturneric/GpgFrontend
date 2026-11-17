# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""What kind of build a CI run is, decided once for every job of it
(`gfrelease ci build-mode` prints it for $GITHUB_OUTPUT).

Trunk model: main is the next minor release, release/X.Y receives fixes
for one series, and versions come from tags (cmake/GitVersion.cmake). Each
channel is fed by one kind of ref (refs.py):

  a release tag vX.Y.Z       release build                publish: final
  a push to main             nightly build                publish: nightly-main
  a push to release/X.Y      beta build of that series    publish: beta
                             (the opt-in beta channel of that series)
  a release candidate tag    beta build of that series    publish: rc
  vX.Y.0-rc.N (only of X.Y.0, before the series' first release; a
  prerelease, offered as a beta is)
  any other tag (a base tag vX.Y.0-alpha.0, an rc of a patch, a stray
  prerelease tag)            nightly build, published nowhere
  a pull request             validation build, published nowhere
  workflow_dispatch          the chosen build_mode, so a release package
                             can be made without re-tagging; published
                             nowhere

`publish_kind` is the one answer to "does this run sign and stage what it
builds": every signing, provenance, staging and upload job gates on it
(`publishes` is it as a boolean). `ref_kind` is what the ref is, whatever
the event (main, release-branch, release-tag, rc-tag or other): a workflow
that signs on a manual run too (mas-sandbox.yml) gates on that.
"""

from __future__ import annotations

import re

from ..errors import ReleaseError
from . import refs

EVENTS = ("push", "pull_request", "workflow_dispatch")
# The CMake option the build's identity is given as (nightly or release),
# and the field that carries its value: the option's name, lower case.
CMAKE_BUILD_MODE = "GPGFRONTEND_BUILD_MODE"
IDENTITY = CMAKE_BUILD_MODE.lower()
# What every job is told, in this order.
FIELDS = ("version", "artifact_version", "build_number", "build_mode",
          "ref_kind", "build_type", IDENTITY,
          "screenshot_cmake_flag", "publish_kind", "publishes",
          "play_upload", "android_package")
PUBLISH_KINDS = {"main": "nightly-main", "release-branch": "beta",
                 "rc-tag": "rc", "release-tag": "final"}
SCREENSHOT_FLAGS = ("-DGPGFRONTEND_BUILD_SCREENSHOT_MODE=ON "
                    "-DGPGFRONTEND_BUILD_TESTS=ON")
_FILESAFE = re.compile(r"[0-9A-Za-z.-]+")
_BUILD_NUMBER = re.compile(r"[1-9][0-9]*")


def ref_kind(ref: str) -> str:
    """main, release-branch, release-tag, rc-tag or other."""
    if ref == refs.MAIN:
        return "main"
    if refs.release_series(ref):
        return "release-branch"
    if refs.is_release_tag(refs.tag_of(ref)):
        return "release-tag"
    if refs.is_rc_tag(refs.tag_of(ref)):
        return "rc-tag"
    return "other"


def decide(event: str, ref: str, *, build_mode: str = "",
           screenshot: bool = False, android: dict) -> dict[str, str]:
    """Every field but the version's, for a run of @p event on @p ref:
    @p build_mode is workflow_dispatch's choice, @p android the product's
    application ids (product.json's "android")."""
    kind = ref_kind(ref)
    if event == "workflow_dispatch":
        if build_mode not in ("nightly", "release"):
            raise ReleaseError(f"build_mode must be nightly or release, not "
                               f"{build_mode!r}")
        mode, build_type = build_mode, "Release"
    elif event == "pull_request":
        mode, build_type = "pr", "RelWithDebInfo"
    elif kind == "release-tag":
        mode, build_type = "release", "Release"
    else:
        mode, build_type = "nightly", "Release"

    publish = PUBLISH_KINDS.get(kind, "none") if event == "push" else "none"
    # Stable release builds drop the "Testing" suffix from the app name;
    # nightly and pull request builds are both "nightly" to CMake. A
    # release/X.Y beta is built as a release too: it is the opt-in beta
    # channel of stable users, so it must install over the release and open
    # its profile, not the nightly's. So is a release candidate, which is a
    # beta of the next series.
    identity = "release" if mode == "release" or publish in ("beta", "rc") \
        else "nightly"
    # The screenshot mode only ever rides on a nightly-style build. CMake
    # refuses it without the tests: its own tests are compiled into gf_test.
    if screenshot and identity == "release":
        raise ReleaseError("the screenshot build cannot be a release build")
    return {
        "build_mode": mode,
        "ref_kind": kind,
        "build_type": build_type,
        IDENTITY: identity,
        "screenshot_cmake_flag": SCREENSHOT_FLAGS if screenshot else "",
        "publish_kind": publish,
        "publishes": "true" if publish != "none" else "false",
        # Google Play: every build that publishes, each to the app of its
        # android_package. Play takes only a versionCode above every one the
        # app holds; play_upload.py --only-if-newer skips one that is not.
        "play_upload": "true" if publish != "none" else "false",
        # The Android application id follows the build mode, as
        # src/mob/CMakeLists.txt derives it: a nightly is an application of
        # its own, beside the release.
        "android_package": android["package"] if identity == "release"
        else android["nightly_package"],
    }


def check_version(version: str, filesafe: str, build_number: str) -> None:
    """The version fields as cmake/GitVersion.cmake gives them: a file-safe
    version every artifact can be named by, and a positive build number."""
    if not version:
        raise ReleaseError("no version")
    if not _FILESAFE.fullmatch(filesafe):
        raise ReleaseError(f"unexpected file-safe version: {filesafe}")
    if not _BUILD_NUMBER.fullmatch(build_number):
        raise ReleaseError(f"unexpected build number: {build_number}")
