# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Where the tooling and the product's checkout are, from this package."""

from pathlib import Path

PACKAGE = Path(__file__).resolve().parent
# scripts/release/: the tooling's own files (tools.txt).
RELEASE_DIR = PACKAGE.parent
# The checkout (or the signing host's copy of what it needs of one).
ROOT = RELEASE_DIR.parent.parent

# The product's release configuration, in the checkout.
RELEASE_CONFIG = ROOT / "release"
PRODUCT_FILE = RELEASE_CONFIG / "product.json"
POLICY_FILE = RELEASE_CONFIG / "policy.toml"
TRUST_ROOT_DIR = RELEASE_CONFIG / "root"
INDEX_KEYS_DIR = RELEASE_CONFIG / "index-keys"
SECURITY_FILE = ROOT / "SECURITY.md"
