#!/usr/bin/env python3
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
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
"""Refuse a workflow step that reads a variable no step ever set.

Each `run:` block is its own process: a variable assigned in one step is gone
by the next, and a job boundary throws away even what `$GITHUB_ENV` carried.
Neither shell says so.  `bash` under `set -u` aborts with "unbound variable",
which is at least loud; **PowerShell expands an undefined variable to the empty
string**, so a gate handed `--expect-count "$EXPECTED"` silently becomes a gate
expecting nothing, and reports a count mismatch rather than the missing
argument that caused it.

That is how `--expect-count` reached CI as 0 while four modules verified.  Two
steps had the fault, in two shells, from one edit -- which is the shape of
thing a check catches and a reader does not.

Stdlib only, and a deliberately small YAML reader: the runner that will
eventually run this has no PyYAML, and a workflow's step list is regular enough
to scan.  Reported per step, with the variable named.
"""

import re
import sys
from pathlib import Path

# Set by the runner, by `env:`, or by the shell itself. Anything here may be
# read without a step assigning it.
AMBIENT = {
    "GITHUB_WORKSPACE", "GITHUB_ENV", "GITHUB_PATH", "GITHUB_OUTPUT",
    "GITHUB_SHA", "GITHUB_REF", "GITHUB_REF_NAME", "GITHUB_EVENT_NAME",
    "GITHUB_RUN_ID", "GITHUB_RUN_NUMBER", "GITHUB_REPOSITORY",
    "GITHUB_SERVER_URL", "GITHUB_ACTOR", "GITHUB_STEP_SUMMARY",
    "RUNNER_OS", "RUNNER_TEMP", "RUNNER_ARCH", "RUNNER_TOOL_CACHE",
    "HOME", "PATH", "PWD", "TMPDIR", "USER", "SHELL", "LANG", "MSYSTEM",
    "XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_CACHE_HOME",
    # `$?`, `$1`, `$@` and friends, plus the loop/case machinery below.
    "_",
}

# PowerShell automatics and the `$env:` prefix it reads the environment with.
PWSH_AMBIENT = {
    "true", "false", "null", "_", "args", "error", "lastexitcode", "psitem",
    "pwd", "home", "profile", "pscommandpath", "matches", "input",
}


def strip_expressions(text):
    """Remove `${{ ... }}`, which the runner substitutes before any shell runs.

    Left in, `${{github.workspace}}` reads as a `$` followed by a brace group
    and invents variables that do not exist.
    """
    return re.sub(r"\$\{\{[^}]*\}\}", "WORKSPACE", text)


def declared_env(lines):
    """Every name declared in an `env:` block, keyed by the line it opens at.

    A step's `env:` is the normal way a secret or a matrix value reaches its
    shell, and those names are set by the runner rather than by the script.
    Returned as a list of (indent, line, names) so a step can claim the blocks
    that enclose it -- workflow, job and its own.
    """
    blocks = []
    current = None
    for number, line in enumerate(lines, 1):
        if not line.strip():
            continue
        indent = len(line) - len(line.lstrip())
        env_match = re.match(r"^(\s*)env:\s*$", line)
        if env_match:
            current = (len(env_match.group(1)), number, set())
            blocks.append(current)
            continue
        if current is None:
            continue
        if indent <= current[0]:
            current = None
            continue
        key = re.match(r"^\s*([A-Za-z_][A-Za-z0-9_]*)\s*:", line)
        if key:
            current[2].add(key.group(1))
    return blocks


def parse_steps(lines):
    """Yield (job, line_number, name, shell, body) for every `run:` step."""
    job = None
    step = None
    for number, line in enumerate(lines, 1):
        job_match = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", line)
        if job_match:
            if step:
                yield step
                step = None
            job = job_match.group(1)
            continue

        name_match = re.match(r"^(\s*)- name:\s*(.*)$", line)
        if name_match:
            if step:
                yield step
            step = {
                "job": job,
                "line": number,
                "name": name_match.group(2).strip(),
                "shell": None,
                "body": [],
                "indent": len(name_match.group(1)),
                "in_run": False,
            }
            continue

        if not step:
            continue

        shell_match = re.match(r"^\s*shell:\s*(\S+)\s*$", line)
        if shell_match and not step["in_run"]:
            step["shell"] = shell_match.group(1)
            continue

        if re.match(r"^\s*run:\s*\|", line):
            step["in_run"] = True
            continue

        if step["in_run"]:
            # The run block ends at the first line indented no further than the
            # keys around it. Blank lines belong to the script.
            if line.strip() and (len(line) - len(line.lstrip())) <= step["indent"] + 1:
                step["in_run"] = False
            else:
                step["body"].append(line)

    if step:
        yield step


def job_environment(lines):
    """The names that steps in a job write to `$GITHUB_ENV`, keyed by job.

    Those genuinely do cross steps -- but only within one job, which is why
    this is collected per job rather than per file.
    """
    found = {}
    job = None
    for line in lines:
        job_match = re.match(r"^  ([A-Za-z0-9_-]+):\s*$", line)
        if job_match:
            job = job_match.group(1)
            continue
        for name in re.findall(
            r"""["']?([A-Za-z_][A-Za-z0-9_]*)=.*?>>\s*["']?\$\{?GITHUB_ENV""", line
        ):
            found.setdefault(job, set()).add(name)
    return found


def literal_free(body):
    """Blank what the shell will not expand, keeping every line in place.

    Three things a naive scan reads as shell variables and should not:

    * a **quoted heredoc** (`<<'SH'`), which is literal text -- usually a
      script written to disk that reads its own environment;
    * a **single-quoted span**, which is how a jq program, an awk program and
      a `$ORIGIN` RUNPATH are all written on purpose;
    * an **escaped** `\\$`.

    The quoting is tracked with a state machine rather than a regex because a
    `'` inside double quotes is an apostrophe, not a quote.  Getting that wrong
    is not a theoretical concern: the word "machine" in a diagnostic here
    opened a span that swallowed the next twenty lines of real code, and the
    check reported a variable assigned in plain sight as undefined.  A check
    that fails on a healthy build is worse than no check.

    Blanked rather than dropped, so a reported offset still names the line the
    reader has to open.
    """
    out = []
    heredoc_tag = None
    # Quoting carries ACROSS lines: a jq or awk program is one single-quoted
    # span holding a dozen of them, and resetting per line reads its `$name`
    # fields as shell variables.
    in_single = False
    in_double = False
    for line in body:
        if heredoc_tag is not None:
            if line.strip() == heredoc_tag:
                heredoc_tag = None
            out.append("")
            continue
        if not in_single and not in_double and line.lstrip().startswith("#"):
            out.append("")
            continue

        opener = re.search(r"<<-?'([A-Za-z_][A-Za-z0-9_]*)'", line)
        if opener:
            heredoc_tag = opener.group(1)

        kept = []
        index = 0
        while index < len(line):
            char = line[index]
            if in_single:
                kept.append(" ")
                if char == "'":
                    in_single = False
            elif char == "'" and not in_double:
                in_single = True
                kept.append(" ")
            elif char == "\\" and in_double and index + 1 < len(line):
                kept.append("  ")
                index += 1
            elif char == "\\" and not in_double and index + 1 < len(line):
                kept.append("  ")
                index += 1
            elif char == "#" and not in_double and (
                not kept or kept[-1].isspace()
            ):
                # A trailing comment. Not a quote context, and an apostrophe in
                # it must not open one.
                break
            else:
                if char == '"':
                    in_double = not in_double
                kept.append(char)
            index += 1
        out.append("".join(kept))
    return out


def bash_assignments(body):
    assigned = set()
    for line in body:
        assigned |= set(re.findall(r"(?:^|[;&|)!]|\bexport\s+|\blocal\s+|\bdeclare\s+)\s*([A-Za-z_][A-Za-z0-9_]*)=", line))
        assigned |= set(re.findall(r"\b(?:local|declare|readonly)\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", line))
        assigned |= set(re.findall(r"\bfor\s+([A-Za-z_][A-Za-z0-9_]*)\s+in\b", line))
        assigned |= set(re.findall(r"\bread\s+(?:-r\s+)?((?:[A-Za-z_][A-Za-z0-9_]*\s*)+)", line))
        assigned |= set(re.findall(r"\bwhile\s+IFS=[^ ]*\s+read\s+-r\s+([A-Za-z_][A-Za-z0-9_]*)", line))
    # `read a b c` contributes a run of names.
    return {name for chunk in assigned for name in chunk.split()}


def bash_uses(body):
    uses = {}
    for offset, line in enumerate(body):
        stripped = line.lstrip()
        if stripped.startswith("#"):
            continue
        for name in re.findall(r"\$\{([A-Za-z_][A-Za-z0-9_]*)[}:]", line):
            uses.setdefault(name, offset)
        for name in re.findall(r"\$([A-Za-z_][A-Za-z0-9_]*)", line):
            uses.setdefault(name, offset)
    return uses


def pwsh_assignments(body):
    assigned = set()
    for line in body:
        assigned |= set(re.findall(r"\$([A-Za-z_][A-Za-z0-9_]*)\s*=", line))
        assigned |= set(re.findall(r"\bforeach\s*\(\s*\$([A-Za-z_][A-Za-z0-9_]*)", line))
    return {name.lower() for name in assigned}


def pwsh_uses(body):
    uses = {}
    for offset, line in enumerate(body):
        stripped = line.lstrip()
        if stripped.startswith("#"):
            continue
        # `$env:NAME` reads the environment and is not a step variable.
        line = re.sub(r"\$env:[A-Za-z_][A-Za-z0-9_]*", "", line, flags=re.I)
        for name in re.findall(r"\$([A-Za-z_][A-Za-z0-9_]*)", line):
            uses.setdefault(name.lower(), offset)
    return uses


def _past_step(lines, step, line):
    """True once `line` has left the step that starts at `step["line"]`."""
    for number in range(step["line"] + 1, line):
        text = lines[number - 1]
        if text.strip() and (len(text) - len(text.lstrip())) <= step["indent"]:
            return True
    return False


def check(path):
    """Report every step-variable in `path` that no reachable step sets."""
    lines = strip_expressions(path.read_text()).splitlines()
    exported = job_environment(lines)
    env_blocks = declared_env(lines)

    failures = 0
    checked = 0
    for step in parse_steps(lines):
        if not step["body"]:
            continue
        checked += 1
        shell = (step["shell"] or "bash").lower()
        carried = set(exported.get(step["job"], set()))
        # Every `env:` block that encloses this step: the workflow's, the
        # job's, and the step's own, which sits just above its `run:`.
        for indent, line, names in env_blocks:
            if line < step["line"] or (
                line > step["line"]
                and indent > step["indent"]
                and not _past_step(lines, step, line)
            ):
                carried |= names

        if "pwsh" in shell or "powershell" in shell:
            # PowerShell variable names are case-insensitive, so `$EXPECTED`
            # and `$expected` are one variable and both sides fold.
            assigned = pwsh_assignments(step["body"])
            known = assigned | PWSH_AMBIENT | {n.lower() for n in AMBIENT | carried}
            uses = pwsh_uses(step["body"])
        else:
            body = literal_free(step["body"])
            assigned = bash_assignments(body)
            known = assigned | AMBIENT | carried
            uses = bash_uses(body)

        for name, offset in sorted(uses.items(), key=lambda item: item[1]):
            if name in known:
                continue
            failures += 1
            print(
                f"FAIL {path}:{step['line'] + offset}: "
                f"[{step['job']}] {step['name']}\n"
                f"       reads ${name}, which no step in this job sets"
            )
    return checked, failures


def main():
    if len(sys.argv) > 1:
        paths = [Path(argument) for argument in sys.argv[1:]]
    else:
        paths = sorted(Path(".github/workflows").glob("*.yml"))
    if not paths:
        print("no workflows to check", file=sys.stderr)
        return 2

    checked = 0
    failures = 0
    for path in paths:
        if not path.is_file():
            print(f"{path}: no such file", file=sys.stderr)
            return 2
        one_checked, one_failed = check(path)
        checked += one_checked
        failures += one_failed
        print(f"  {path}: {one_checked} run step(s)")

    if failures:
        print(f"{failures} undefined variable use(s) across {checked} step(s)")
        return 1
    print(f"{checked} run step(s): every variable read is set by a step that "
          f"can reach it")
    return 0


if __name__ == "__main__":
    sys.exit(main())
