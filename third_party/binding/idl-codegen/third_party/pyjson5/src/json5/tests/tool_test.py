# Copyright 2017 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import sys
import unittest

import json5

from ..fakes.host_fake import FakeHost
from ..host import Host
from ..version import VERSION


if sys.version_info[0] < 3:
    # pylint: disable=redefined-builtin
    str = unicode


class CheckMixin(object):
    def _write_files(self, host, files):
        for path, contents in list(files.items()):
            host.write_text_file(path, contents)

    def check_cmd(self, args, stdin=None, files=None,
                  returncode=None, out=None, err=None):
        host = self._host()
        orig_wd, tmpdir = None, None
        try:
            orig_wd = host.getcwd()
            tmpdir = host.mkdtemp()
            host.chdir(tmpdir)
            if files:
                self._write_files(host, files)
            rv = self._call(host, args, stdin, returncode, out, err)
            actual_ret, actual_out, actual_err = rv
        finally:
            if tmpdir:
              host.rmtree(tmpdir)
            if orig_wd:
              host.chdir(orig_wd)

        return actual_ret, actual_out, actual_err


class UnitTestMixin(object):
    def _host(self):
        return FakeHost()

    def _call(self, host, args, stdin=None,
              returncode=None, out=None, err=None):
        if stdin is not None:
            host.stdin.write(str(stdin))
            host.stdin.seek(0)
        actual_ret = json5.tool.main(args, host)
        actual_out = host.stdout.getvalue()
        actual_err = host.stderr.getvalue()
        if returncode is not None:
            self.assertEqual(returncode, actual_ret)
        if out is not None:
            self.assertEqual(out, actual_out)
        if err is not None:
            self.assertEqual(err, actual_err)
        return actual_ret, actual_out, actual_err


class ToolTest(UnitTestMixin, CheckMixin, unittest.TestCase):
    maxDiff = None

    def test_help(self):
        self.check_cmd(['--help'], returncode=0)

    def test_json_switches(self):
        self.check_cmd(['-c', '{"foo": 1}'], returncode=0,
                       out=u'{foo:1}\n')
        self.check_cmd(['--json', '-c', '{"foo": 1}'], returncode=0,
                       out=u'{"foo": 1}\n')

    def test_read_command(self):
        self.check_cmd(['-c', '"foo"'], returncode=0, out=u"'foo'\n")

    def test_read_from_stdin(self):
        self.check_cmd([], stdin='"foo"\n', returncode=0, out=u"'foo'\n")

    def test_read_from_a_file(self):
        files = {
            'foo.json5': '"foo"\n',
        }
        self.check_cmd(['foo.json5'], files=files, returncode=0, out=u"'foo'\n")

    def test_unknown_switch(self):
        self.check_cmd(['--unknown-switch'], returncode=2,
            err=u'json5: error: unrecognized arguments: --unknown-switch\n\n')

    def test_version(self):
        self.check_cmd(['--version'], returncode=0,
                       out=str(VERSION) + '\n')


if __name__ == '__main__':  # pragma: no cover
    unittest.main()
