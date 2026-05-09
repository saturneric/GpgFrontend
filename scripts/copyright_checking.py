import sys
import re
from pathlib import Path

# 1. 定义你当前期望的最新标准年份
TARGET_YEAR_STR = "2021-2026"

# 是否允许脚本自动修改文件 (True 为自动修复年份或补充声明，False 为仅扫描报错)
AUTO_FIX = True

# 使用 f-string 动态注入年份
copyright_text_source = f"""/**
 * Copyright (C) {TARGET_YEAR_STR} Saturneric <eric@bktus.com>
 *
 * This file is part of GpgFrontend.
 *
 * GpgFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GpgFrontend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GpgFrontend. If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from
 * the gpg4usb project, which is under GPL-3.0-or-later.
 *
 * All the source code of GpgFrontend was modified and released by
 * Saturneric <eric@bktus.com> starting on May 12, 2021.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */"""

copyright_text_script = f"""# Copyright (C) {TARGET_YEAR_STR} Saturneric <eric@bktus.com>
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
# The initial version of the source code is inherited from
# the gpg4usb project, which is under GPL-3.0-or-later.
#
# All the source code of GpgFrontend was modified and released by
# Saturneric <eric@bktus.com> starting on May 12, 2021.
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""


def get_flexible_pattern(target_text: str) -> re.Pattern:
    """
    Convert the standard copyright text to a regular expression:
    Allow the year part to be any format like "2024", "2021-2024", "2021-2025".
    """
    escaped_text = re.escape(target_text)
    # \d{4}(?:\-\d{4})? 可以匹配 4位年份 或者 4位-4位年份
    year_regex = r"\d{4}(?:\-\d{4})?"  # Matches 4-digit year or 4-digit-4-digit year
    # Replace the escaped target year with regex wildcard
    escaped_text = escaped_text.replace(re.escape(TARGET_YEAR_STR), year_regex)
    # Compatible with Windows (\r\n) and Linux (\n) line endings
    escaped_text = escaped_text.replace(r"\n", r"\r?\n")
    return re.compile(escaped_text)


def process_copyright(directory: str, target_text: str, suffixes: tuple) -> bool:
    all_passed = True
    base_dir = Path(directory)

    if not base_dir.exists():
        print(f"Warning: Directory '{directory}' does not exist.")
        return True

    pattern = get_flexible_pattern(target_text)

    for suffix in suffixes:
        for file_path in base_dir.rglob(f"*{suffix}"):
            if not file_path.is_file():
                continue

            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()

                # Search for structurally similar copyright statements (regardless of year)
                match = pattern.search(content)

                if match:
                    matched_str = match.group(0)
                    if matched_str == target_text:
                        # 1. Perfect match, no modification needed
                        continue
                    else:
                        # 2. Structure matches but year is outdated
                        if AUTO_FIX:
                            # Replace the old copyright block
                            new_content = (
                                content[: match.start()]
                                + target_text
                                + content[match.end() :]
                            )
                            with open(file_path, "w", encoding="utf-8") as f:
                                f.write(new_content)
                            print(f"[Updated] Fixed year in: {file_path}")
                        else:
                            print(f"[Warn] Outdated copyright year in: {file_path}")
                            all_passed = False
                else:
                    # 3. No copyright statement found, need to add
                    if AUTO_FIX:
                        # Intelligently handle script file shebang (e.g. #!/bin/bash or #!/usr/bin/env python)
                        if content.startswith("#!"):
                            parts = content.split("\n", 1)
                            if len(parts) == 2:
                                new_content = (
                                    parts[0] + "\n\n" + target_text + "\n\n" + parts[1]
                                )
                            else:
                                new_content = parts[0] + "\n\n" + target_text + "\n"
                        else:
                            new_content = target_text + "\n\n" + content

                        with open(file_path, "w", encoding="utf-8") as f:
                            f.write(new_content)
                        print(f"[Added] Inserted missing copyright to: {file_path}")
                    else:
                        print(f"[Missing] No copyright declaration in: {file_path}")
                        all_passed = False

            except UnicodeDecodeError:
                print(f"[Error] Encoding issue (not UTF-8): {file_path}")
                all_passed = False
            except Exception as e:
                print(f"[Error] Failed to process {file_path}: {e}")
                all_passed = False

    return all_passed


def main():
    check_configs = [
        ("../src", copyright_text_source, (".c", ".cpp", ".h", ".hpp", ".rs")),
        ("../src", copyright_text_script, (".txt",)),
        ("../modules", copyright_text_source, (".c", ".cpp", ".h", ".hpp", ".rs")),
        ("../modules", copyright_text_script, (".txt",)),
    ]

    action_word = "fix" if AUTO_FIX else "check"
    print(f"Starting copyright {action_word}...")

    overall_success = True
    for directory, text, suffixes in check_configs:
        if not process_copyright(directory, text, suffixes):
            overall_success = False

    if overall_success:
        print(f"✅ Copyright {action_word} done: All files are up-to-date.")
        sys.exit(0)
    else:
        print(f"❌ Copyright {action_word} failed: Some files need attention.")
        sys.exit(1)


if __name__ == "__main__":
    main()
