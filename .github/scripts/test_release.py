import copy
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import release


SHA = "a" * 40
REPOSITORY = "csound/plugins"
MERGED_PR = {
    "merged_at": "2026-10-29T10:35:04Z",
    "merge_commit_sha": SHA,
    "base": {"ref": "main", "repo": {"full_name": REPOSITORY}},
    "labels": [{"name": "release"}],
}


class VersionTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.root = Path(directory.name)

    def write_manifests(self, version, lock_version=None, root_version=None):
        (self.root / "package.json").write_text(json.dumps({"version": version}))
        (self.root / "package-lock.json").write_text(json.dumps({
            "version": lock_version or version,
            "packages": {"": {"version": root_version or version}},
        }))

    def test_stable_and_prerelease_versions(self):
        for version in ["7.0.0", "7.1.0-beta.2", "7.0.0-rc1", "7.0.0-alpha-beta.0"]:
            with self.subTest(version=version):
                self.write_manifests(version)
                self.assertEqual(release.read_version(self.root), version)

    def test_invalid_versions(self):
        for version in [None, 7, "", "v7.0.0", "7.0", "07.0.0", "7.0.0-beta.01",
                        "7.0.0-", "7.0.0-beta..1", "7.0.0+build", "7.0.0\nrelease=true"]:
            with self.subTest(version=version):
                self.write_manifests(version)
                with self.assertRaisesRegex(ValueError, "SemVer"):
                    release.read_version(self.root)

    def test_both_lockfile_versions_must_match(self):
        for mismatch in [{"lock_version": "7.0.1"}, {"root_version": "7.0.1"}]:
            with self.subTest(mismatch=mismatch):
                self.write_manifests("7.0.0", **mismatch)
                with self.assertRaisesRegex(ValueError, "must match"):
                    release.read_version(self.root)


class ReleaseTests(unittest.TestCase):
    def metadata(self, version="7.0.0", event="push", ref="refs/heads/main", prs=None):
        return release.release_metadata(
            version, event, ref, SHA, REPOSITORY, [MERGED_PR] if prs is None else prs
        )

    def test_labelled_main_merge_releases(self):
        self.assertEqual(self.metadata(), {
            "version": "7.0.0", "release": "true", "prerelease": "false",
        })

    def test_labelled_develop_merge_prereleases(self):
        pr = copy.deepcopy(MERGED_PR)
        pr["base"]["ref"] = "develop"
        self.assertEqual(self.metadata("7.1.0-beta.2", ref="refs/heads/develop", prs=[pr]), {
            "version": "7.1.0-beta.2", "release": "true", "prerelease": "true",
        })

    def test_only_push_events_to_release_branches_can_release(self):
        for event, ref in [
            ("pull_request", "refs/heads/main"),
            ("pull_request_target", "refs/heads/main"),
            ("workflow_dispatch", "refs/heads/main"),
            ("push", "refs/heads/topic"),
            ("push", "refs/tags/7.0.0"),
        ]:
            with self.subTest(event=event, ref=ref):
                self.assertEqual(self.metadata(event=event, ref=ref)["release"], "false")

    def test_requires_exact_merge_base_repository_and_label(self):
        for field, value in [
            ("merged_at", None),
            ("merge_commit_sha", "b" * 40),
            ("base", {"ref": "develop", "repo": {"full_name": REPOSITORY}}),
            ("base", {"ref": "main", "repo": {"full_name": "fork/plugins"}}),
            ("labels", []),
            ("labels", [{"name": "no-release"}]),
            ("labels", [{"name": "release-candidate"}]),
        ]:
            with self.subTest(field=field, value=value):
                pr = copy.deepcopy(MERGED_PR)
                pr[field] = value
                self.assertEqual(self.metadata(prs=[pr])["release"], "false")
        self.assertEqual(self.metadata(prs=[])["release"], "false")

    def test_release_version_must_match_branch(self):
        with self.assertRaisesRegex(ValueError, "main needs a stable version"):
            self.metadata(version="7.0.0-beta.2")
        pr = copy.deepcopy(MERGED_PR)
        pr["base"]["ref"] = "develop"
        with self.assertRaisesRegex(ValueError, "develop needs a prerelease version"):
            self.metadata(ref="refs/heads/develop", prs=[pr])

    def test_ordinary_builds_allow_both_version_types(self):
        for ref in ["refs/heads/main", "refs/heads/develop"]:
            for version in ["7.0.0", "7.0.0-beta.2"]:
                with self.subTest(ref=ref, version=version):
                    self.assertEqual(self.metadata(version, ref=ref, prs=[])["release"], "false")


class CommandTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.output = Path(directory.name) / "output"
        self.output.write_text("previous=value\n")
        self.environment = {
            "GITHUB_EVENT_NAME": "push",
            "GITHUB_REF": "refs/heads/main",
            "GITHUB_SHA": SHA,
            "GITHUB_REPOSITORY": REPOSITORY,
            "GITHUB_OUTPUT": str(self.output),
        }
        self.addCleanup(patch.stopall)
        patch.dict(os.environ, self.environment).start()
        patch("sys.argv", ["release.py"]).start()
        patch.object(release, "read_version", return_value="7.0.0").start()

    def test_paginated_pr_lookup_and_output(self):
        unrelated = copy.deepcopy(MERGED_PR)
        unrelated["merge_commit_sha"] = "b" * 40
        pages = json.dumps([[unrelated] * 100, [MERGED_PR]])
        with patch.object(release.subprocess, "check_output", return_value=pages) as api:
            release.main()
        api.assert_called_once_with([
            "gh", "api", "--paginate", "--slurp",
            f"repos/{REPOSITORY}/commits/{SHA}/pulls?per_page=100",
        ], text=True)
        self.assertEqual(self.output.read_text(),
                         "previous=value\nversion=7.0.0\nrelease=true\nprerelease=false\n")

    def test_pr_and_manual_runs_do_not_query_api(self):
        for event in ["pull_request", "workflow_dispatch"]:
            with self.subTest(event=event), patch.dict(os.environ, {"GITHUB_EVENT_NAME": event}):
                with patch.object(release.subprocess, "check_output") as api:
                    release.main()
                api.assert_not_called()
        self.assertNotIn("release=true", self.output.read_text())

    def test_api_failure_does_not_write_release_outputs(self):
        with patch.object(release.subprocess, "check_output",
                          side_effect=subprocess.CalledProcessError(1, "gh")):
            with self.assertRaises(subprocess.CalledProcessError):
                release.main()
        self.assertEqual(self.output.read_text(), "previous=value\n")

    def test_tag_check_does_not_query_api_or_write_outputs(self):
        with patch("sys.argv", ["release.py", "--check-tag"]):
            with patch.object(release, "check_tag") as check:
                release.main()
        check.assert_called_once_with("7.0.0", SHA)
        self.assertEqual(self.output.read_text(), "previous=value\n")


class TagTests(unittest.TestCase):
    def test_missing_tag_is_allowed(self):
        with patch.object(release.subprocess, "run",
                          return_value=subprocess.CompletedProcess("git", 1)):
            with patch.object(release.subprocess, "check_output") as resolve:
                release.check_tag("7.0.0", SHA)
        resolve.assert_not_called()

    def test_existing_tag_must_resolve_to_release_commit(self):
        for target in [SHA, "b" * 40]:
            with self.subTest(target=target):
                with patch.object(release.subprocess, "run",
                                  return_value=subprocess.CompletedProcess("git", 0)):
                    with patch.object(release.subprocess, "check_output",
                                      return_value=f"{target}\n") as resolve:
                        if target == SHA:
                            release.check_tag("7.0.0", SHA)
                        else:
                            with self.assertRaisesRegex(ValueError, "choose a new version"):
                                release.check_tag("7.0.0", SHA)
                    resolve.assert_called_once_with(
                        ["git", "rev-parse", "--verify", "refs/tags/7.0.0^{commit}"], text=True
                    )

    def test_git_error_does_not_count_as_missing_tag(self):
        with patch.object(release.subprocess, "run",
                          return_value=subprocess.CompletedProcess("git", 128)):
            with self.assertRaises(subprocess.CalledProcessError):
                release.check_tag("7.0.0", SHA)


if __name__ == "__main__":
    unittest.main()
