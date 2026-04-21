"""
Extra regression tests for the pipe lab (not from course staff).
Run from the lab directory: python3 -m unittest -v test_lab1_crucial
"""

import errno
import subprocess
import unittest


def _make():
    return subprocess.run(["make"], capture_output=True, text=True)


class TestLab1Crucial(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.built = _make().returncode == 0

    def setUp(self):
        self.assertTrue(self.built, msg="make failed; build ./pipe first")

    def test_no_arguments_exits_einval(self):
        r = subprocess.run(["./pipe"], capture_output=True, text=True)
        self.assertEqual(r.returncode, errno.EINVAL)

    def test_single_true(self):
        r = subprocess.run(["./pipe", "true"])
        self.assertEqual(r.returncode, 0)

    def test_single_false(self):
        r = subprocess.run(["./pipe", "false"])
        self.assertEqual(r.returncode, 1)

    def test_first_stage_reads_parent_stdin(self):
        r = subprocess.run(
            ["./pipe", "cat"],
            input="stdin-check\n",
            text=True,
            capture_output=True,
        )
        self.assertEqual(r.returncode, 0)
        self.assertEqual(r.stdout, "stdin-check\n")

    def test_invalid_first_command_einval(self):
        r = subprocess.run(
            ["./pipe", "__no_such_command_lab1__"],
            capture_output=True,
            text=True,
        )
        self.assertEqual(r.returncode, errno.EINVAL)
        self.assertNotEqual(r.stderr.strip(), "")

    def test_eight_stage_pipeline_matches_shell(self):
        # Eight executables, no per-stage argv (lab rule).
        ref = subprocess.run(
            [
                "bash",
                "-c",
                "printf 'one two three\\n' "
                "| cat | cat | cat | cat | cat | cat | cat | wc",
            ],
            capture_output=True,
            text=True,
        )
        self.assertEqual(ref.returncode, 0, msg="reference shell pipeline failed")

        pipe = subprocess.run(
            [
                "./pipe",
                "cat",
                "cat",
                "cat",
                "cat",
                "cat",
                "cat",
                "cat",
                "wc",
            ],
            input="one two three\n",
            text=True,
            capture_output=True,
        )
        self.assertEqual(pipe.returncode, 0)
        self.assertEqual(
            pipe.stdout,
            ref.stdout,
            msg="./pipe should match shell for eight-stage pipeline",
        )

    def test_last_stage_exit_status_like_bash(self):
        # bash: pipeline status defaults to last command.
        bt = subprocess.run(["./pipe", "false", "true"])
        self.assertEqual(bt.returncode, 0)

        tb = subprocess.run(["./pipe", "true", "false"])
        self.assertEqual(tb.returncode, 1)

    def test_empty_stdin_no_deadlock(self):
        r = subprocess.run(
            ["./pipe", "wc"],
            input="",
            text=True,
            capture_output=True,
            timeout=5,
        )
        self.assertEqual(r.returncode, 0)
        parts = r.stdout.split()
        self.assertTrue(parts, msg="wc should print counts")
        self.assertEqual(parts[0], "0")

    def test_reader_closes_early_no_deadlock(self):
        # yes writes forever; head reads a bounded prefix then exits (writer may get SIGPIPE).
        # bash uses the last command's status (head -> 0). ./pipe should match that.
        ref = subprocess.run(
            ["bash", "-c", "yes | head"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        pipe = subprocess.run(
            ["./pipe", "yes", "head"],
            capture_output=True,
            text=True,
            timeout=5,
        )
        self.assertEqual(
            pipe.returncode,
            ref.returncode,
            msg="pipeline exit status should match bash (typically last stage, not SIGPIPE writer)",
        )
        self.assertTrue(pipe.stdout.strip())
