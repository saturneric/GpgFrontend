# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease: sign, verify and publish the product's releases.

The package is in layers. A module imports its own layer and those below
it, never one above; scripts/tests/test_architecture.py holds that, and
the rules after each layer.

  base       errors.py   what goes wrong, and whether trying again helps
             report.py   what a role says as it goes (warnings, progress)
             paths.py    where the checkout and its release/ files are

  domain/    what the release process decides, from what it is given:
             semver, refs (which refs publish what), policy
             (release/policy.toml and product.json), layout (every path
             in the stores, the host's state and a stage's work, and what
             each signature says it signs), formats (JSON, times,
             versions), limits (what clients hold to as well), manifest (a
             candidate's SHA256SUMS.txt), descriptor (release.json),
             index_model (what the index says of what was admitted),
             release_notes and github_release (a stable release as its
             GitHub release shows it).
             Runs no tool, prints nothing, reads no environment.

  adapters/  one outside thing each: process (running a tool), files,
             clock, git, store (rclone), minisign, openpgp (gpg), cosign,
             github (its REST API), and windows/ (cab, msi, authenticode, msix, x509). Prints
             nothing.

  app/       what each role does, from the domain and the adapters:
             product (the product and the trust it is published under),
             trust and trust_admin (the trust root and index keys),
             candidate and stage (CI), promote and index (the signing
             host), stable and imported (the maintainer), github_sync
             (stable releases mirrored to GitHub), verify, views,
             doctor, check_order, checks (what more than one role checks)
             and hints (what fixes a problem). Returns what it did; runs
             tools only through the adapters, prints nothing, reads no
             environment.

  cli/       the command line: parser, main, commands/ (one module per
             group), forwarded (the commands with a command line of their
             own: policy, candidate, ci), host (the signing host's steps,
             with the lines and exit statuses its container reads),
             settings, ui (all that is printed), topics and completion.

The roles: CI stages candidates (gfrelease stage), the signing host
promotes them and publishes the signed index (promote, index, resign),
the maintainer signs stable releases offline (stable, import) and keeps
the trust root (trust root, trust keys); anyone checks what is published
(verify, show, status).
"""
