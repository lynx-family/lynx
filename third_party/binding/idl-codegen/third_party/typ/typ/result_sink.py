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

"""Functionality for interacting with ResultDB's ResultSink.

ResultSink is a process that accepts test results via HTTP requests for
ingesting into ResultDB.

See go/resultdb and go/resultsink for more details.
"""

import base64
import cgi
import json
import os
import sys

import requests

from typ import json_results
from typ import expectations_parser

# Valid status taken from the "TestStatus" enum in
# https://source.chromium.org/chromium/infra/infra/+/master:go/src/go.chromium.org/luci/resultdb/proto/v1/test_result.proto
VALID_STATUSES = {
    'PASS',
    'FAIL',
    'CRASH',
    'ABORT',
    'SKIP',
}
# The maximum allowed size is 4096 bytes as per
# https://source.chromium.org/chromium/infra/infra/+/master:go/src/go.chromium.org/luci/resultdb/proto/v1/test_result.proto;drc=ca12b9f52b27f064b0fa47c39baa3b011ffa5790;l=96
MAX_HTML_SUMMARY_LENGTH = 4096
TRUNCATED_SUMMARY_KEY = 'Test Log'
TRUNCATED_SUMMARY_MESSAGE = ('...Full output in "%s" artifact.</pre>' %
                             TRUNCATED_SUMMARY_KEY)


class ResultSinkReporter(object):
    def __init__(self, host, disable=False):
        """Class for interacting with ResultDB's ResultSink.

        Args:
            host: A host.Host or host_fake.FakeHost instance.
            disable: Whether to explicitly disable ResultSink integration.
        """
        self._host = host
        self._sink = None
        if disable:
            return

        luci_context_file = self._host.getenv('LUCI_CONTEXT')
        if not luci_context_file:
            return
        self._sink = json.loads(
                self._host.read_text_file(luci_context_file)).get('result_sink')
        if not self._sink:
            return

        self._url = ('http://%s/prpc/luci.resultsink.v1.Sink/ReportTestResults'
                     % self._sink['address'])
        self._headers = {
                'Content-Type': 'application/json',
                'Accept': 'application/json',
                'Authorization': 'ResultSink %s' % self._sink['auth_token']
        }
        self._session = requests.Session()

    @property
    def resultdb_supported(self):
        return self._sink is not None

    def report_individual_test_result(
            self, test_name_prefix, result, artifact_output_dir, expectations):
        """Reports typ results for a single test to ResultSink.

        Inputs are typically similar to what is passed to
        json_results.make_full_results(), but for a single test/Result instead
        of multiple tests/a ResultSet.

        Args:
            test_name_prefix: A string containing the prefix that will be added
                    to the test name.
            results: A json_results.Results instance containing the results to
                    report.
            artifact_output_dir: The path to the directory where artifacts test
                    artifacts are saved on disk. If a relative path, will be
                    automatically joined with the cwd. Use '.' instead of '' to
                    point to the cwd.
            expectations: An expectations_parser.TestExpectations instance, or
                    None if one is not available.

        Returns:
            0 if the result was reported successfully or ResultDB is not
            supported, otherwise 1.
        """
        if not self.resultdb_supported:
            return 0

        expectation_tags = expectations.tags if expectations else []

        test_id = test_name_prefix + result.name
        raw_typ_expected_results = (
                expectations.expectations_for(result.name).raw_results
                if expectations
                else [expectations_parser.RESULT_TAGS[
                        json_results.ResultType.Pass]])
        result_is_expected = result.actual in result.expected

        tag_list = [
            ('test_name', test_id),
        ]
        for expectation in result.expected:
            tag_list.append(('typ_expectation', expectation))
        for expectation in raw_typ_expected_results:
            tag_list.append(('raw_typ_expectation', expectation))
        if expectation_tags:
            for tag in expectation_tags:
                tag_list.append(('typ_tag', tag))

        artifacts = {}
        original_artifacts = result.artifacts or {}
        assert TRUNCATED_SUMMARY_KEY not in original_artifacts
        if original_artifacts:
            assert artifact_output_dir
            if not os.path.isabs(artifact_output_dir):
                artifact_output_dir = self._host.join(
                        self._host.getcwd(), artifact_output_dir)

        for artifact_name, artifact_filepaths in original_artifacts.items():
            # The typ artifact implementation supports multiple artifacts for
            # a single artifact name due to retries, but ResultDB does not.
            if len(artifact_filepaths) > 1:
                for index, filepath in enumerate(artifact_filepaths):
                    artifacts[artifact_name + '-file%d' % index] = {
                        'filePath': self._host.join(
                                artifact_output_dir, filepath),
                    }
            else:
                artifacts[artifact_name] = {
                    'filePath': self._host.join(
                            artifact_output_dir, artifact_filepaths[0]),
                }

        summary_content = 'stdout: %s\nstderr: %s' % (
                cgi.escape(result.out), cgi.escape(result.err))
        summary_content = summary_content.encode('utf-8')
        html_summary = '<pre>%s</pre>' % summary_content
        truncated_summary = None
        if len(html_summary) > MAX_HTML_SUMMARY_LENGTH:
            truncated_summary = (html_summary[:MAX_HTML_SUMMARY_LENGTH
                                              - len(TRUNCATED_SUMMARY_MESSAGE)]
                                 + TRUNCATED_SUMMARY_MESSAGE)
            artifacts[TRUNCATED_SUMMARY_KEY] = {
                'contents': base64.b64encode(summary_content)
            }
        html_summary = truncated_summary or html_summary

        return self._report_result(
                test_id, result.actual, result_is_expected, artifacts,
                tag_list, html_summary, result.took)


    def _report_result(
            self, test_id, status, expected, artifacts, tag_list, html_summary,
            duration):
        """Reports a single test result to ResultSink.

        Args:
            test_id: A string containing the unique identifier of the test.
            status: A string containing the status of the test. Must be in
                    |VALID_STATUSES|.
            expected: A boolean denoting whether |status| is expected or not.
            artifacts: A dict of artifact names (strings) to dicts, specifying
                    either a filepath or base64-encoded artifact content.
            tag_list: A list of tuples of (str, str), each element being a
                    key/value pair to add as tags to the reported result.
            html_summary: A string containing HTML summarizing the test run.
                    Must be <= |MAX_HTML_SUMMARY_LENGTH|.
            duration: How long the test took in seconds.

        Returns:
            0 if the result was reported successfully or ResultDB is not
            supported, otherwise 1.
        """
        if not self.resultdb_supported:
            return 0

        # TODO(crbug.com/1104252): Handle testLocation key so that ResultDB can
        # look up the correct component for bug filing.
        test_result = _create_json_test_result(
                test_id, status, expected, artifacts, tag_list, html_summary,
                duration)

        return self._post(json.dumps({'testResults': [test_result]}))

    def _post(self, content):
        """POST to ResultSink.

        Args:
            content: A string containing the content to send in the body of the
                    POST request.

        Returns:
            0 if the POST succeeded, otherwise 1.
        """
        res = self._session.post(
            url=self._url,
            headers=self._headers,
            data=content)
        return 0 if res.ok else 1


def _create_json_test_result(
        test_id, status, expected, artifacts, tag_list, html_summary,
        duration):
    """Formats data to be suitable for sending to ResultSink.

    Args:
        test_id: A string containing the unique identifier of the test.
        status: A string containing the status of the test. Must be in
                |VALID_STATUSES|.
        expected: A boolean denoting whether |status| is expected or not.
        artifacts: A dict of artifact names (strings) to dicts, specifying
                either a filepath or base64-encoded artifact content.
        tag_list: A list of tuples of (str, str), each element being a
                    key/value pair to add as tags to the reported result.
        html_summary: A string containing HTML summarizing the test run. Must be
                <= |MAX_HTML_SUMMARY_LENGTH|.
        duration: How long the test took in seconds.

    Returns:
        A dict containing the provided data in a format that is ingestable by
        ResultSink.
    """
    assert status in VALID_STATUSES
    assert len(html_summary) <= MAX_HTML_SUMMARY_LENGTH
    # This is based off the protobuf in
    # https://source.chromium.org/chromium/infra/infra/+/master:go/src/go.chromium.org/luci/resultdb/sink/proto/v1/test_result.proto
    test_result = {
            'testId': test_id,
            'status': status,
            'expected': expected,
            # If the number is too large or small, python formats the number
            # in scientific notation, but google.protobuf.duration doesn't
            # accept an input formatted in scientific notation.
            #
            # .9fs because nanosecond is the smallest precision that
            # google.protobuf.duration supports.
            'duration': '%.9fs' % duration,
            'summaryHtml': html_summary,
            'artifacts': artifacts,
            'tags': [],
    }
    for (k, v) in tag_list:
        test_result['tags'].append({'key': k, 'value': v})

    return test_result


def result_sink_retcode_from_result_set(result_set):
    """Determines whether any interactions with ResultSink failed.

    Args:
        result_set: A json_results.ResultSet instance.

    Returns:
        1 if any Result in |result_set| failed to interact properly with
        ResultSink, otherwise 0.
    """
    return int(any(r.result_sink_retcode for r in result_set.results))
