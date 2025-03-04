# Copyright 2020 Google Inc. All rights reserved.
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

import base64
import json
import os
import unittest

from typ import expectations_parser
from typ import json_results
from typ import result_sink
from typ.fakes import host_fake


DEFAULT_LUCI_CONTEXT = {
    'result_sink': {
        'address': 'address',
        'auth_token': 'auth_token',
    },
}
ARTIFACT_DIR = os.path.join('artifact', 'dir')


def CreateResult(input_dict):
    """Creates a ResultSet with Results.

    Args:
        input_dict: A dict describing the result to create.

    Returns:
        A Result filled with the information from |input_dict|
    """
    return json_results.Result(name=input_dict['name'],
                               actual=input_dict['actual'],
                               started=True,
                               took=1,
                               worker=None,
                               expected=input_dict.get('expected'),
                               out=input_dict.get('out', 'stdout'),
                               err=input_dict.get('err', 'stderr'),
                               artifacts=input_dict.get('artifacts'))


def StubWithRetval(retval):
    def stub(*args, **kwargs):
        stub.args = args
        stub.kwargs = kwargs
        return retval
    stub.args = None
    stub.kwargs = None
    return stub


def GetTestResultFromPostedJson(json_string):
    dict_content = json.loads(json_string)
    return dict_content['testResults'][0]


def CreateExpectedTestResult(
        test_id=None, status=None, expected=None, duration=None,
        summary_html=None, artifacts=None, tags=None):
    test_id = test_id or 'test_name_prefix.test_name'
    return {
        'testId': test_id,
        'status': status or json_results.ResultType.Pass,
        'expected': expected or True,
        'duration': duration or '1.000000000s',
        'summaryHtml': (summary_html
                        or '<pre>stdout: stdout\nstderr: stderr</pre>'),
        'artifacts': artifacts or {},
        'tags': tags or [
            {'key': 'test_name', 'value': test_id},
            {'key': 'typ_expectation', 'value': json_results.ResultType.Pass},
            {'key': 'raw_typ_expectation', 'value': 'Pass'},
            {'key': 'typ_tag', 'value': 'bar_tag'},
            {'key': 'typ_tag', 'value': 'foo_tag'},],
    }


def CreateTestExpectations(expectation_definitions=None):
    expectation_definitions = expectation_definitions or [{'name': 'test_name'}]
    tags = set()
    results = set()
    lines = []
    for ed in expectation_definitions:
        name = ed['name']
        t = ed.get('tags', ['foo_tag', 'bar_tag'])
        r = ed.get('results', ['Pass'])
        str_t = '[ ' + ' '.join(t) + ' ]' if t else ''
        str_r = '[ ' + ' '.join(r) + ' ]'
        lines.append('%s %s %s' % (str_t, name, str_r))
        tags |= set(t)
        results |= set(r)
    data = '# tags: [ %s ]\n# results: [ %s ]\n%s' % (
            ' '.join(tags), ' '.join(results), '\n'.join(lines))
    # TODO(crbug.com/1148060): Remove the passing in of tags to the constructor
    # once tags are properly updated through parse_tagged_list().
    expectations = expectations_parser.TestExpectations(list(tags))
    expectations.parse_tagged_list(data)
    return expectations


class ResultSinkReporterTest(unittest.TestCase):
    maxDiff = None

    def setUp(self):
        self._host = host_fake.FakeHost()
        self._luci_context_file = '/tmp/luci_context_file.json'

    def setLuciContextWithContent(self, content):
        self._host.env['LUCI_CONTEXT'] = self._luci_context_file
        self._host.files[self._luci_context_file] = json.dumps(content)

    def testNoLuciContext(self):
        if 'LUCI_CONTEXT' in self._host.env:
            del self._host.env['LUCI_CONTEXT']
        rsr = result_sink.ResultSinkReporter(self._host)
        self.assertFalse(rsr.resultdb_supported)

    def testExplicitDisable(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host, True)
        self.assertFalse(rsr.resultdb_supported)

    def testNoSinkKey(self):
        self.setLuciContextWithContent({})
        rsr = result_sink.ResultSinkReporter(self._host)
        self.assertFalse(rsr.resultdb_supported)

    def testValidSinkKey(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        self.assertTrue(rsr.resultdb_supported)

    def testReportIndividualTestResultEarlyReturnIfNotSupported(self):
        self.setLuciContextWithContent({})
        rsr = result_sink.ResultSinkReporter(self._host)
        rsr._post = lambda: 1/0  # Shouldn't be called.
        self.assertEqual(
                rsr.report_individual_test_result(None, None, None, None), 0)

    def testReportIndividualTestResultBasicCase(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        result = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
        })
        rsr._post = StubWithRetval(2)
        retval = rsr.report_individual_test_result(
                'test_name_prefix.', result, ARTIFACT_DIR,
                CreateTestExpectations())
        self.assertEqual(retval, 2)
        expected_result = CreateExpectedTestResult()
        self.assertEqual(GetTestResultFromPostedJson(rsr._post.args[0]),
                         expected_result)

    def testReportIndividualTestResultNoTestExpectations(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        result = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
        })
        rsr._post = StubWithRetval(2)
        retval = rsr.report_individual_test_result(
                'test_name_prefix.', result, ARTIFACT_DIR, None)
        self.assertEqual(retval, 2)
        expected_result = CreateExpectedTestResult(tags=[
            {'key': 'test_name', 'value': 'test_name_prefix.test_name'},
            {'key': 'typ_expectation', 'value': json_results.ResultType.Pass},
            {'key': 'raw_typ_expectation', 'value': 'Pass'},
        ])
        self.assertEqual(GetTestResultFromPostedJson(rsr._post.args[0]),
                         expected_result)

    def testReportIndividualTestResultLongSummary(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        result = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
            'out': 'a' * 4097,
            'err': '',
        })
        rsr._post = StubWithRetval(0)
        rsr.report_individual_test_result(
                'test_name_prefix.', result, ARTIFACT_DIR,
                CreateTestExpectations())

        test_result = GetTestResultFromPostedJson(rsr._post.args[0])
        truncated_summary = '<pre>stdout: %s%s' % (
            'a' * (result_sink.MAX_HTML_SUMMARY_LENGTH
                   - len(result_sink.TRUNCATED_SUMMARY_MESSAGE)
                   - 13),
            result_sink.TRUNCATED_SUMMARY_MESSAGE
        )
        artifact_contents = 'stdout: %s\nstderr: ' % ('a' * 4097)
        expected_result = CreateExpectedTestResult(
                summary_html=truncated_summary,
                artifacts={
                    'Test Log': {
                        'contents': base64.b64encode(artifact_contents)
                    }
                })
        self.assertEqual(test_result, expected_result)

    def testReportIndividualTestResultLongSummaryUnicode(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        result = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
            'out': u'\u00A5' + 'a' * 4096,
            'err': '',
        })
        rsr._post = StubWithRetval(0)
        rsr.report_individual_test_result(
                'test_name_prefix.', result, ARTIFACT_DIR,
                CreateTestExpectations())

        test_result = GetTestResultFromPostedJson(rsr._post.args[0])
        truncated_summary = '<pre>stdout: %s%s' % (
            u'\u00A5' + 'a' * (result_sink.MAX_HTML_SUMMARY_LENGTH
                               - len(result_sink.TRUNCATED_SUMMARY_MESSAGE)
                               - 15),
            result_sink.TRUNCATED_SUMMARY_MESSAGE
        )
        self.assertEqual(len(truncated_summary.encode('utf-8')),
                         result_sink.MAX_HTML_SUMMARY_LENGTH)
        artifact_contents = 'stdout: %s\nstderr: ' % (u'\u00A5' + 'a' * 4096)
        expected_result = CreateExpectedTestResult(
                summary_html=truncated_summary,
                artifacts={
                    'Test Log': {
                        'contents': base64.b64encode(
                                artifact_contents.encode('utf-8'))
                    }
                })
        self.assertEqual(test_result, expected_result)

    def testReportIndividualTestResultConflictingKeyLongSummary(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        rsr._post = lambda: 1/0
        result = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
            'artifacts': {
                'Test Log': [''],
            },
            'out': 'a' * 4097,
        })
        with self.assertRaises(AssertionError):
            rsr.report_individual_test_result(
                    'test_name_prefix', result, ARTIFACT_DIR,
                    CreateTestExpectations())

    def testReportIndividualTestResultSingleArtifact(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        rsr._post = StubWithRetval(2)
        results = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
            'artifacts': {
                'artifact_name': ['some_artifact'],
            },
        })
        retval = rsr.report_individual_test_result(
                'test_name_prefix.', results, ARTIFACT_DIR,
                CreateTestExpectations())
        self.assertEqual(retval, 2)

        test_result = GetTestResultFromPostedJson(rsr._post.args[0])
        expected_result = CreateExpectedTestResult(
                artifacts={
                    'artifact_name': {
                        'filePath': self._host.join(self._host.getcwd(),
                                                    ARTIFACT_DIR,
                                                    'some_artifact'),
                    },
                })
        self.assertEqual(test_result, expected_result)

    def testReportIndividualTestResultMultipleArtifacts(self):
        self.setLuciContextWithContent(DEFAULT_LUCI_CONTEXT)
        rsr = result_sink.ResultSinkReporter(self._host)
        rsr._post = StubWithRetval(2)
        results = CreateResult({
            'name': 'test_name',
            'actual': json_results.ResultType.Pass,
            'artifacts': {
                'artifact_name': ['some_artifact', 'another_artifact'],
            },
        })
        retval = rsr.report_individual_test_result(
                'test_name_prefix.', results, ARTIFACT_DIR,
                CreateTestExpectations())
        self.assertEqual(retval, 2)

        test_result = GetTestResultFromPostedJson(rsr._post.args[0])
        expected_result = CreateExpectedTestResult(
                artifacts={
                    'artifact_name-file0': {
                        'filePath': self._host.join(self._host.getcwd(),
                                                    ARTIFACT_DIR,
                                                    'some_artifact'),
                    },
                    'artifact_name-file1': {
                        'filePath': self._host.join(self._host.getcwd(),
                                                    ARTIFACT_DIR,
                                                    'another_artifact'),
                    },
                })
        self.assertEqual(test_result, expected_result)

    def testReportResultEarlyReturnIfNotSupported(self):
        self.setLuciContextWithContent({})
        rsr = result_sink.ResultSinkReporter(self._host)
        result_sink._create_json_test_result = lambda: 1/0
        self.assertEqual(rsr._report_result(
                'test_id', json_results.ResultType.Pass, True, {}, {},
                '<pre>summary</pre>', 1), 0)

    def testCreateJsonTestResultInvalidStatus(self):
        with self.assertRaises(AssertionError):
            result_sink._create_json_test_result(
                'test_id', 'InvalidStatus', False, {}, {}, '', 1)

    def testCreateJsonTestResultInvalidSummary(self):
        with self.assertRaises(AssertionError):
            result_sink._create_json_test_result(
                'test_id', json_results.ResultType.Pass, True, {}, {},
                'a' * 4097, 1)

    def testCreateJsonTestResultBasic(self):
        retval = result_sink._create_json_test_result(
            'test_id', json_results.ResultType.Pass, True,
            {'artifact': {'filePath': 'somepath'}},
            [('tag_key', 'tag_value')], '<pre>summary</pre>', 1)
        self.assertEqual(retval, {
            'testId': 'test_id',
            'status': json_results.ResultType.Pass,
            'expected': True,
            'duration': '1.000000000s',
            'summaryHtml': '<pre>summary</pre>',
            'artifacts': {
                'artifact': {
                    'filePath': 'somepath',
                },
            },
            'tags': [
                {
                    'key': 'tag_key',
                    'value': 'tag_value',
                },
            ],
        })

    def testCreateJsonWithVerySmallDuration(self):
        retval = result_sink._create_json_test_result(
            'test_id', json_results.ResultType.Pass, True,
            {'artifact': {'filePath': 'somepath'}},
            [('tag_key', 'tag_value')], '<pre>summary</pre>', 1e-10)
        self.assertEqual(retval['duration'], '0.000000000s')

    def testCreateJsonFormatsWithVeryLongDuration(self):
        retval = result_sink._create_json_test_result(
            'test_id', json_results.ResultType.Pass, True,
            {'artifact': {'filePath': 'somepath'}},
            [('tag_key', 'tag_value')], '<pre>summary</pre>', 1e+16)
        self.assertEqual(retval['duration'], '10000000000000000.000000000s')
