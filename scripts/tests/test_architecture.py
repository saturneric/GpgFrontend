#!/usr/bin/env python3
# Copyright (C) 2021-2026 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
"""gfrelease's layers hold (scripts/release/gfrelease/__init__.py says
what they are): each module imports only its own layer and those below,
no two modules import each other round, only the command line prints,
only the adapters run tools, and nothing reaches into another module's
private names.

Run: python3 -m unittest discover -s scripts/tests
"""

import ast
import unittest

import _paths

PACKAGE = _paths.GFRELEASE

# Lower first; a module may import its own layer and every one below.
LAYERS = {"base": 0, "domain": 1, "adapters": 2, "app": 3, "cli": 4,
          "entry": 5}
BASE = {"errors", "report", "paths"}


def layer_of(module: str) -> str:
    """The layer of @p module, dotted under gfrelease ("app.index")."""
    head = module.split(".")[0]
    # The package's __init__ holds its docstring and nothing else.
    if head in BASE or head in ("__init__", ""):
        return "base"
    if head == "__main__":
        return "entry"
    return head


def modules() -> dict[str, ast.Module]:
    found = {}
    for path in sorted(PACKAGE.rglob("*.py")):
        rel = path.relative_to(PACKAGE).with_suffix("")
        parts = list(rel.parts)
        if parts[-1] == "__init__" and len(parts) > 1:
            parts = parts[:-1]
        found[".".join(parts)] = ast.parse(path.read_text("utf-8"),
                                           str(path))
    return found


def package_of(module: str, tree_is_package: bool) -> list[str]:
    parts = module.split(".") if module not in ("__init__", "__main__") \
        else []
    return parts if tree_is_package else parts[:-1]


def imports(module: str, tree: ast.Module, names: set[str]) -> dict:
    """{imported gfrelease module: (line, alias it is bound to or None)}."""
    is_package = (PACKAGE / module.replace(".", "/") / "__init__.py").exists()
    here = package_of(module, is_package)
    found = {}
    for node in ast.walk(tree):
        if isinstance(node, ast.ImportFrom):
            if node.level:
                base = here[:len(here) - (node.level - 1)]
                parts = base + (node.module.split(".") if node.module else [])
            elif node.module and node.module.split(".")[0] == "gfrelease":
                parts = node.module.split(".")[1:]
            else:
                continue
            for alias in node.names:
                full = ".".join(parts + [alias.name])
                target = full if full in names else ".".join(parts)
                if target in names or target == "":
                    found.setdefault(target or "__init__",
                                     (node.lineno, alias.asname or alias.name
                                      if full in names else None))
        elif isinstance(node, ast.Import):
            for alias in node.names:
                if alias.name.startswith("gfrelease."):
                    found.setdefault(alias.name[len("gfrelease."):],
                                     (node.lineno, None))
    return found


class ArchitectureTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.trees = modules()
        names = set(cls.trees)
        cls.graph = {m: imports(m, t, names) for m, t in cls.trees.items()}

    def test_each_module_imports_its_own_layer_and_below(self):
        wrong = []
        for module, found in self.graph.items():
            mine = LAYERS[layer_of(module)]
            for target, (line, _) in found.items():
                if LAYERS[layer_of(target)] > mine:
                    wrong.append(f"{module}:{line} imports {target} "
                                 f"({layer_of(module)} -> "
                                 f"{layer_of(target)})")
        self.assertEqual(wrong, [])

    def test_no_two_modules_import_each_other_round(self):
        edges = {m: {t for t in found if t != m and t in self.graph}
                 for m, found in self.graph.items()}
        # Packages' __init__ modules import nothing of their own.
        seen, stack, cycles = set(), [], []

        def visit(m):
            if m in stack:
                cycles.append(" -> ".join(stack[stack.index(m):] + [m]))
                return
            if m in seen:
                return
            seen.add(m)
            stack.append(m)
            for t in sorted(edges[m]):
                visit(t)
            stack.pop()
        for m in sorted(edges):
            visit(m)
        self.assertEqual(cycles, [])

    def calls(self, layers: set[str], what) -> list[str]:
        found = []
        for module, tree in self.trees.items():
            if layer_of(module) not in layers:
                continue
            for node in ast.walk(tree):
                hit = what(node)
                if hit:
                    found.append(f"{module}:{node.lineno}: {hit}")
        return found

    def test_only_the_command_line_prints(self):
        def prints(node):
            return isinstance(node, ast.Call) and \
                isinstance(node.func, ast.Name) and \
                node.func.id == "print" and "print()"
        self.assertEqual(self.calls({"base", "domain", "adapters", "app"},
                                    prints), [])

    def test_only_adapters_and_the_command_line_read_the_environment(self):
        def environ(node):
            return isinstance(node, ast.Attribute) and \
                node.attr == "environ" and "os.environ"
        self.assertEqual(self.calls({"base", "domain", "app"}, environ), [])

    def test_only_adapters_run_tools(self):
        def subprocess(node):
            if isinstance(node, (ast.Import, ast.ImportFrom)):
                names = [a.name for a in node.names] + \
                    [getattr(node, "module", None) or ""]
                return "subprocess" in names and "imports subprocess"
            return None
        self.assertEqual(self.calls({"base", "domain", "app"}, subprocess),
                         [])

    def test_no_module_reaches_into_another_ones_private_names(self):
        wrong = []
        for module, tree in self.trees.items():
            aliases = {alias: target for target, (_, alias)
                       in self.graph[module].items() if alias}
            for node in ast.walk(tree):
                if isinstance(node, ast.Attribute) and \
                        isinstance(node.value, ast.Name) and \
                        node.value.id in aliases and \
                        node.attr.startswith("_") and \
                        not node.attr.startswith("__"):
                    wrong.append(f"{module}:{node.lineno}: "
                                 f"{aliases[node.value.id]}.{node.attr}")
        self.assertEqual(wrong, [])

    def test_every_module_is_in_a_layer(self):
        self.assertEqual(
            sorted(m for m in self.trees if layer_of(m) not in LAYERS), [])


if __name__ == "__main__":
    unittest.main()
