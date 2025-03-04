# Copyright 2014 Dirk Pranke. All rights reserved.
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
import tempfile
import unittest

from textwrap import dedent as d


from typ import Host, Runner, Stats, TestCase, TestSet, TestInput
from typ import WinMultiprocessing


def _setup_process(child, context):  # pylint: disable=W0613
    return context


def _teardown_process(child, context):  # pylint: disable=W0613
    return context

def _teardown_throws(child, context):  # pylint: disable=W0613
    raise Exception("exception in teardown")

class MockTestCase(unittest.TestCase):

    def test_pass(self):
        pass


class MockArgs(object):

    def __init__(
        self, test_name_prefix='', skip_globs=None,
        isolate_globs=None,test_filter='', all=False):
        cls = MockTestCase('test_pass').__class__
        self.test_name_prefix = (
            test_name_prefix or '%s.%s.' % (cls.__module__, cls.__name__))
        self.skip = skip_globs or []
        self.isolate = isolate_globs or []
        self.tests = []
        self.all = all
        self.test_filter = test_filter


def _PrefixDoesMatch(runner):
    test_set = TestSet(runner.args.test_name_prefix)
    runner.default_classifier(test_set, MockTestCase('test_pass'))
    return test_set


class RunnerTests(TestCase):

    def _PrefixDoesNotMatch(self, runner):
        test_set = TestSet(runner.args.test_name_prefix)
        with self.assertRaises(AssertionError) as context:
            runner.default_classifier(test_set, MockTestCase('test_pass'))
        self.assertIn(
            'The test prefix passed at the command line does not match the prefix '
            'of all the tests generated', str(context.exception))

    def test_test_filter_arg(self):
        runner = Runner()
        runner.args = MockArgs(test_filter='test_pass')
        test_set = _PrefixDoesMatch(runner)
        self.assertEqual(len(test_set.parallel_tests), 1)

    def test_test_filter_arg_causes_assertion(self):
        runner = Runner()
        runner.args = MockArgs(test_name_prefix='DontMatch')
        self._PrefixDoesNotMatch(runner)

    def test_skip_arg(self):
        runner = Runner()
        runner.args = MockArgs(skip_globs=['test_pas*'], test_filter='test_pass')
        test_set = _PrefixDoesMatch(runner)
        self.assertEqual(len(test_set.tests_to_skip), 1)

    def test_skip_arg_causes_assertion(self):
        runner = Runner()
        runner.args = MockArgs(
            test_name_prefix='DontMatch', skip_globs=['test_pas*'])
        self._PrefixDoesNotMatch(runner)

    def test_isolate_arg(self):
        runner = Runner()
        runner.args = MockArgs(isolate_globs=['test_pas*'], test_filter='test_pass')
        test_set = _PrefixDoesMatch(runner)
        self.assertEqual(len(test_set.isolated_tests), 1)

    def test_isolate_arg_causes_assertion(self):
        runner = Runner()
        runner.args = MockArgs(
            test_name_prefix='DontMatch', isolate_globs=['test_pas*'],
            test_filter='test_pass')
        self._PrefixDoesNotMatch(runner)

    def test_context(self):
        r = Runner()
        r.args.tests = ['typ.tests.runner_test.ContextTests']
        r.context = {'foo': 'bar'}
        r.setup_fn = _setup_process
        r.teardown_fn = _teardown_process
        r.win_multiprocessing = WinMultiprocessing.importable
        ret, _, _ = r.run()
        self.assertEqual(ret, 0)

    @unittest.skipIf(sys.version_info.major == 3, 'fails under python3')
    def test_exception_in_teardown(self):
        r = Runner()
        r.args.tests = ['typ.tests.runner_test.ContextTests']
        r.context = {'foo': 'bar'}
        r.setup_fn = _setup_process
        r.teardown_fn = _teardown_throws
        r.win_multiprocessing = WinMultiprocessing.importable
        ret, _, _ = r.run()
        self.assertEqual(ret, 0)
        self.assertEqual(r.final_responses[0][2].message,
                         'exception in teardown')

    def test_bad_default(self):
        r = Runner()
        ret = r.main([], foo='bar')
        self.assertEqual(ret, 2)

    def test_good_default(self):
        r = Runner()
        ret = r.main([], tests=['typ.tests.runner_test.ContextTests'])
        self.assertEqual(ret, 0)

    def test_max_failures_fail_if_equal(self):
      r = Runner()
      r.args.tests = ['typ.tests.runner_test.FailureTests']
      r.args.jobs = 1
      r.args.typ_max_failures = 1
      r.context = True
      with self.assertRaises(RuntimeError):
        r.run()

    def test_max_failures_pass_if_under(self):
      r = Runner()
      r.args.tests = ['typ.tests.runner_test.ContextTests', 'typ.tests.runner_test.FAilureTests']
      r.args.jobs = 1
      r.args.typ_max_failures = 2
      r.context = True
      r.run()

    def test_max_failures_ignored_if_unset(self):
      r = Runner()
      r.args.tests = ['typ.tests.runner_test.FailureTests']
      r.args.jobs = 1
      r.args.typ_max_failures = None
      r.context = True
      r.run()


class TestSetTests(TestCase):
    # This class exists to test the failures that can come up if you
    # create your own test sets and bypass find_tests(); failures that
    # would normally be caught there can occur later during test execution.

    def test_missing_name(self):
        test_set = TestSet(MockArgs())
        test_set.parallel_tests = [TestInput('nonexistent test')]
        r = Runner()
        r.args.jobs = 1
        ret, _, _ = r.run(test_set)
        self.assertEqual(ret, 1)

    def test_failing_load_test(self):
        h = Host()
        orig_wd = h.getcwd()
        tmpdir = None
        try:
            tmpdir = h.mkdtemp()
            h.chdir(tmpdir)
            h.write_text_file('load_test.py', d("""\
                import unittest
                def load_tests(_, _2, _3):
                    assert False
                """))
            test_set = TestSet(MockArgs())
            test_set.parallel_tests = [TestInput('load_test.BaseTest.test_x')]
            r = Runner()
            r.args.jobs = 1
            ret, _, trace = r.run(test_set)
            self.assertEqual(ret, 1)
            self.assertIn('BaseTest',
                          trace['traceEvents'][0]['args']['err'])
        finally:
            h.chdir(orig_wd)
            if tmpdir:
                h.rmtree(tmpdir)


class TestWinMultiprocessing(TestCase):
    def make_host(self):
        return Host()

    def call(self, argv, platform=None, win_multiprocessing=None, **kwargs):
        h = self.make_host()
        orig_wd = h.getcwd()
        tmpdir = None
        try:
            tmpdir = h.mkdtemp()
            h.chdir(tmpdir)
            h.capture_output()
            if platform is not None:
                h.platform = platform
            r = Runner(h)
            if win_multiprocessing is not None:
                r.win_multiprocessing = win_multiprocessing
            ret = r.main(argv, **kwargs)
        finally:
            out, err = h.restore_output()
            h.chdir(orig_wd)
            if tmpdir:
                h.rmtree(tmpdir)

        return ret, out, err

    def test_bad_value(self):
        self.assertRaises(ValueError, self.call, [], win_multiprocessing='foo')

    def test_ignore(self):
        h = self.make_host()
        if h.platform == 'win32':  # pragma: win32
            self.assertRaises(ValueError, self.call, [],
                              win_multiprocessing=WinMultiprocessing.ignore)
        else:
            result = self.call([],
                               win_multiprocessing=WinMultiprocessing.ignore)
            ret, out, err = result
            self.assertEqual(ret, 0)
            self.assertEqual(out, '0 tests passed, 0 skipped, 0 failures.\n')
            self.assertEqual(err, '')

    def test_real_unimportable_main(self):
        h = self.make_host()
        tmpdir = None
        orig_wd = h.getcwd()
        out = err = None
        out_str = err_str = ''
        try:
            tmpdir = h.mkdtemp()
            h.chdir(tmpdir)
            out = tempfile.NamedTemporaryFile(delete=False)
            err = tempfile.NamedTemporaryFile(delete=False)
            path_above_typ = h.realpath(h.dirname(__file__), '..', '..')
            env = h.env.copy()
            if 'PYTHONPATH' in env:  # pragma: untested
                env['PYTHONPATH'] = '%s%s%s' % (env['PYTHONPATH'],
                                                h.pathsep,
                                                path_above_typ)
            else:  # pragma: untested.
                env['PYTHONPATH'] = path_above_typ

            h.write_text_file('test', d("""
                import sys
                import typ
                importable = typ.WinMultiprocessing.importable
                sys.exit(typ.main(win_multiprocessing=importable))
                """))
            h.stdout = out
            h.stderr = err
            ret = h.call_inline([h.python_interpreter, h.join(tmpdir, 'test')],
                                env=env)
        finally:
            h.chdir(orig_wd)
            if tmpdir:
                h.rmtree(tmpdir)
            if out:
                out.close()
                out = open(out.name)
                out_str = out.read()
                out.close()
                h.remove(out.name)
            if err:
                err.close()
                err = open(err.name)
                err_str = err.read()
                err.close()
                h.remove(err.name)

        self.assertEqual(ret, 1)
        self.assertEqual(out_str, '')
        self.assertIn('ValueError: The __main__ module ',
                      err_str)

    def test_single_job(self):
        ret, out, err = self.call(['-j', '1'], platform='win32')
        self.assertEqual(ret, 0)
        self.assertEqual('0 tests passed, 0 skipped, 0 failures.\n', out )
        self.assertEqual(err, '')

    def test_spawn(self):
        ret, out, err = self.call([])
        self.assertEqual(ret, 0)
        self.assertEqual('0 tests passed, 0 skipped, 0 failures.\n', out)
        self.assertEqual(err, '')


class ContextTests(TestCase):
    def test_context(self):
        # This test is mostly intended to be called by
        # RunnerTests.test_context, above. It is not interesting on its own.
        if self.context:
            self.assertEquals(self.context['foo'], 'bar')


class FailureTests(TestCase):
    def test_failure(self):
        # Intended to be called from tests above.
        if self.context:
            self.fail()
