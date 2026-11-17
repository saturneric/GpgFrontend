# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""Checks more than one role makes, made one way: the maintainer's stage
before anything leaves (stable.py), and the signing host when it admits a
release (index.py)."""

from __future__ import annotations

from pathlib import Path

from ..adapters.windows import authenticode, msix
from .product import Product


def windows_signed(product: Product, file: Path, info: dict,
                   authenticode_ca: str | None, *,
                   recheck: bool = False) -> None:
    """The Windows package @p file (@p info its artifact entry) and every
    program inside it the product's rule names carry a valid Authenticode
    signature of the pinned certificate, time-stamped when the root says
    so; on a @p recheck of a release admitted before, of one it replaced
    too."""
    authenticode.verify_package(
        file, info,
        product.authenticode_subjects if recheck
        else [product.authenticode_subject],
        authenticode_ca, authenticode.Rules.of(product),
        require_timestamp=product.authenticode_timestamp_required)


def msix_of(product: Product, file: Path, info: dict, msi_file: Path,
            authenticode_ca: str | None, *, recheck: bool = False) -> None:
    """The MSIX @p file is signed as windows_signed() says, and holds the
    files of @p msi_file, the signed MSI it was made of, and no others."""
    windows_signed(product, file, info, authenticode_ca, recheck=recheck)
    msix.check_derived(file, msi_file)
