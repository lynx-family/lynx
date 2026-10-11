#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from pathlib import Path
import os
import sys
import tempfile
import time
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
from gperf import find_gperf, run_gperf, write_if_changed


class GperfTest(unittest.TestCase):
    def test_finds_packaged_executable(self):
        with tempfile.TemporaryDirectory() as temporary_dir:
            executable = "gperf.exe" if os.name == "nt" else "gperf"
            packaged = Path(temporary_dir) / "gperf" / "bin" / executable
            packaged.parent.mkdir(parents=True)
            packaged.touch()
            self.assertEqual(find_gperf(temporary_dir), str(packaged.resolve()))

    def test_runs_absolute_tool_and_normalizes_legacy_output(self):
        with tempfile.TemporaryDirectory() as temporary_dir:
            root = Path(temporary_dir)
            tool = root / "fake_gperf.py"
            tool.write_text(
                "#!/usr/bin/env python3\n"
                "print('register int value;')\n"
                "print('/*FALLTHROUGH*/')\n",
                encoding="utf-8",
            )
            tool.chmod(0o755)
            template_dir = root / "templates"
            template_dir.mkdir()
            template = template_dir / "input.gperf"
            template.write_text("%%\n", encoding="utf-8")
            self.assertEqual(
                run_gperf(tool, template, []),
                "int value;\n[[fallthrough]];\n",
            )

    def test_write_if_changed_preserves_timestamp(self):
        with tempfile.TemporaryDirectory() as temporary_dir:
            output = Path(temporary_dir) / "generated.cc"
            write_if_changed(output, "contents\n")
            initial_timestamp = output.stat().st_mtime_ns
            time.sleep(0.001)
            write_if_changed(output, "contents\n")
            self.assertEqual(output.stat().st_mtime_ns, initial_timestamp)


if __name__ == "__main__":
    unittest.main()
