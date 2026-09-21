#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
VERSION_FILE = ROOT / "VERSION"

VERSION_PATTERN = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+(?:[-+][0-9A-Za-z.-]+)?$")


def read_version() -> str:
    version = VERSION_FILE.read_text(encoding="utf-8").strip()
    if not VERSION_PATTERN.fullmatch(version):
        raise ValueError(f"VERSION contains an unsupported value: {version!r}")
    return version


def replace_exactly_one(path: Path, pattern: re.Pattern[str], replacement, *, write: bool) -> bool:
    original = path.read_text(encoding="utf-8")
    updated, count = pattern.subn(replacement, original)
    if count != 1:
        raise RuntimeError(f"{path.relative_to(ROOT)}: expected exactly one version field, found {count}")
    if updated == original:
        return False
    if write:
        path.write_text(updated, encoding="utf-8")
    return True


def synchronize(version: str, *, write: bool) -> list[str]:
    changed: list[str] = []

    targets = (
        (
            ROOT / "config.cpp",
            re.compile(r'(?m)^(\s*version\s*=\s*")[^"]+(";\s*)$'),
            lambda match: f"{match.group(1)}{version}{match.group(2)}",
        ),
        (
            ROOT / "mod.cpp",
            re.compile(r'(?m)^(\s*version\s*=\s*")[^"]+(";\s*)$'),
            lambda match: f"{match.group(1)}{version}{match.group(2)}",
        ),
        (
            ROOT / "README.md",
            re.compile(r"(?m)^(Current version:\s*\*\*)[^*]+(\*\*\.\s*)$"),
            lambda match: f"{match.group(1)}{version}{match.group(2)}",
        ),
    )

    for path, pattern, replacement in targets:
        if replace_exactly_one(path, pattern, replacement, write=write):
            changed.append(str(path.relative_to(ROOT)))
    return changed


def main() -> int:
    parser = argparse.ArgumentParser(description="Synchronize TransferZ release metadata from VERSION.")
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true", help="write VERSION into config.cpp, mod.cpp and README")
    mode.add_argument("--set", metavar="VERSION", help="set VERSION and synchronize all generated version fields")
    mode.add_argument("--check", action="store_true", help="check metadata only (default)")
    args = parser.parse_args()

    if args.set:
        if not VERSION_PATTERN.fullmatch(args.set):
            parser.error("VERSION must look like X.Y.Z, optionally with a simple prerelease/build suffix")
        VERSION_FILE.write_text(args.set + "\n", encoding="utf-8")
        version = args.set
        changed = synchronize(version, write=True)
        print(f"TransferZ version set to {version}")
        if changed:
            print("Synchronized: " + ", ".join(changed))
        return 0

    version = read_version()
    if args.write:
        changed = synchronize(version, write=True)
        if changed:
            print("Synchronized: " + ", ".join(changed))
        else:
            print("Version metadata already synchronized.")
        return 0

    changed = synchronize(version, write=False)
    if changed:
        print("Version metadata is out of sync with VERSION: " + ", ".join(changed))
        return 1
    print(f"Version metadata synchronized at {version}.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
