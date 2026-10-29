"""Read the release version and select labelled merges for native releases."""

import argparse
import json
import os
from pathlib import Path
import re
import subprocess


VERSION = re.compile(
    r"(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)"
    r"(?:-([0-9A-Za-z-]+(?:\.[0-9A-Za-z-]+)*))?"
)
RELEASE_BRANCHES = {"refs/heads/main", "refs/heads/develop"}


def read_version(root):
    package = json.loads((root / "package.json").read_text())
    lock = json.loads((root / "package-lock.json").read_text())
    version = package["version"]
    match = VERSION.fullmatch(version) if isinstance(version, str) else None
    if not match or any(
        part.isdigit() and len(part) > 1 and part.startswith("0")
        for part in (match[4] or "").split(".")
    ):
        raise ValueError("package.json must contain a SemVer version without build metadata")
    if lock["version"] != version or lock["packages"][""]["version"] != version:
        raise ValueError("package.json and package-lock.json versions must match")
    return version


def release_metadata(version, event, ref, sha, repository, pull_requests):
    release = event == "push" and ref in RELEASE_BRANCHES and any(
        pr["merged_at"]
        and pr["merge_commit_sha"] == sha
        and pr["base"]["ref"] == ref.removeprefix("refs/heads/")
        and pr["base"]["repo"]["full_name"] == repository
        and any(label["name"] == "release" for label in pr["labels"])
        for pr in pull_requests
    )
    prerelease = "-" in version
    if release and ref == "refs/heads/main" and prerelease:
        raise ValueError("A release from main needs a stable version in package.json")
    if release and ref == "refs/heads/develop" and not prerelease:
        raise ValueError("A release from develop needs a prerelease version in package.json")
    return {
        "version": version,
        "release": str(bool(release)).lower(),
        "prerelease": str(prerelease).lower(),
    }


def check_tag(version, sha):
    ref = f"refs/tags/{version}"
    result = subprocess.run(["git", "show-ref", "--verify", "--quiet", ref])
    if result.returncode == 1:
        return
    result.check_returncode()
    target = subprocess.check_output(
        ["git", "rev-parse", "--verify", f"{ref}^{{commit}}"], text=True
    ).strip()
    if target != sha:
        raise ValueError(
            f"Tag {version} already points to {target}, not {sha}; choose a new version"
        )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-tag", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    version = read_version(root)
    if args.check_tag:
        check_tag(version, os.environ["GITHUB_SHA"])
        return

    event = os.environ["GITHUB_EVENT_NAME"]
    ref = os.environ["GITHUB_REF"]
    sha = os.environ["GITHUB_SHA"]
    repository = os.environ["GITHUB_REPOSITORY"]
    pull_requests = []
    if event == "push" and ref in RELEASE_BRANCHES:
        pages = json.loads(subprocess.check_output(
            ["gh", "api", "--paginate", "--slurp",
             f"repos/{repository}/commits/{sha}/pulls?per_page=100"],
            text=True,
        ))
        pull_requests = [pr for page in pages for pr in page]
    metadata = release_metadata(version, event, ref, sha, repository, pull_requests)
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        for name, value in metadata.items():
            print(f"{name}={value}", file=output)


if __name__ == "__main__":
    main()
