# Copyright 2007 The JsonCpp Authors
# Distributed under MIT license, or public domain if desired and
# recognized in your jurisdiction.
# See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

from __future__ import print_function
from __future__ import unicode_literals
from io import open
from glob import glob
import sys
import os
import os.path
import optparse

VALGRIND_CMD = 'valgrind --tool=memcheck --leak-check=yes --undef-value-errors=yes '

def getStatusOutput(cmd):
    """
    Return int, unicode (for both Python 2 and 3).
    Note: os.popen().close() would return None for 0.
    """
    print(cmd, file=sys.stderr)
    pipe = os.popen(cmd)
    process_output = pipe.read()
    try:
        # We have been using os.popen(). When we read() the result
        # we get 'str' (bytes) in py2, and 'str' (unicode) in py3.
        # Ugh! There must be a better way to handle this.
        process_output = process_output.decode('utf-8')
    except AttributeError:
        pass  # python3
    status = pipe.close()
    return status, process_output
def compareOutputs(expected, actual, message):
    expected = expected.strip().replace('\r','').split('\n')
    actual = actual.strip().replace('\r','').split('\n')
    diff_line = 0
    max_line_to_compare = min(len(expected), len(actual))
    for index in range(0,max_line_to_compare):
        if expected[index].strip() != actual[index].strip():
            diff_line = index + 1
            break
    if diff_line == 0 and len(expected) != len(actual):
        diff_line = max_line_to_compare+1
    if diff_line == 0:
        return None
    def safeGetLine(lines, index):
        index += -1
        if index >= len(lines):
            return ''
        return lines[index].strip()
    return """  Difference in %s at line %d:
  Expected: '%s'
  Actual:   '%s'
""" % (message, diff_line,
       safeGetLine(expected,diff_line),
       safeGetLine(actual,diff_line))
        
def safeReadFile(path):
    try:
        return open(path, 'rt', encoding = 'utf-8').read()
    except IOError as e:
        return '<File "%s" is missing: %s>' % (path,e)

def runAllTests(jsontest_executable_path, input_dir = None,
                 use_valgrind=False, with_json_checker=False,
                 writerClass='StyledWriter'):
    if not input_dir:
        input_dir = os.path.join(os.getcwd(), 'data')
    tests = glob(os.path.join(input_dir, '*.json'))
    if with_json_checker:
        test_jsonchecker = glob(os.path.join(input_dir, '../jsonchecker', '*.json'))
    else:
        test_jsonchecker = []
    failed_tests = []
    valgrind_path = use_valgrind and VALGRIND_CMD or ''
    for input_path in tests + test_jsonchecker:
        expect_failure = os.path.basename(input_path).startswith('fail')
        is_json_checker_test = (input_path in test_jsonchecker) or expect_failure
        print('TESTING:', input_path, end=' ')
        options = is_json_checker_test and '--json-checker' or ''
        options += ' --json-writer %s'%writerClass
        cmd = '%s%s %s "%s"' % (            valgrind_path, jsontest_executable_path, options,
            input_path)
        status, process_output = getStatusOutput(cmd)
        if is_json_checker_test:
            if expect_failure:
                if not status:
                    print('FAILED')
                    failed_tests.append((input_path, 'Parsing should have failed:\n%s' %
                                          safeReadFile(input_path)))
                else:
                    print('OK')
            else:
                if status:
                    print('FAILED')
                    failed_tests.append((input_path, 'Parsing failed:\n' + process_output))
                else:
                    print('OK')
        else:
            base_path = os.path.splitext(input_path)[0]
            actual_output = safeReadFile(base_path + '.actual')
            actual_rewrite_output = safeReadFile(base_path + '.actual-rewrite')
            open(base_path + '.process-output', 'wt', encoding = 'utf-8').write(process_output)
            if status:
                print('parsing failed')
                failed_tests.append((input_path, 'Parsing failed:\n' + process_output))
            else:
                expected_output_path = os.path.splitext(input_path)[0] + '.expected'
                expected_output = open(expected_output_path, 'rt', encoding = 'utf-8').read()
                detail = (compareOutputs(expected_output, actual_output, 'input')
                            or compareOutputs(expected_output, actual_rewrite_output, 'rewrite'))
                if detail:
                    print('FAILED')
                    failed_tests.append((input_path, detail))
                else:
                    print('OK')

    if failed_tests:
        print()
        print('Failure details:')
        for failed_test in failed_tests:
            print('* Test', failed_test[0])
            print(failed_test[1])
            print()
        print('Test results: %d passed, %d failed.' % (len(tests)-len(failed_tests),
                                                       len(failed_tests)))
        return 1
    else:
        print('All %d tests passed.' % len(tests))
        return 0

def main():
    from optparse import OptionParser
    parser = OptionParser(usage="%prog [options] <path to jsontestrunner.exe> [test case directory]")
    parser.add_option("--valgrind",
                  action="store_true", dest="valgrind", default=False,
                  help="run all the tests using valgrind to detect memory leaks")
    parser.add_option("-c", "--with-json-checker",
                  action="store_true", dest="with_json_checker", default=False,
                  help="run all the tests from the official JSONChecker test suite of json.org")
    parser.enable_interspersed_args()
    options, args = parser.parse_args()

    if len(args) < 1 or len(args) > 2:
        parser.error('Must provides at least path to jsontestrunner executable.')
        sys.exit(1)

    jsontest_executable_path = os.path.normpath(os.path.abspath(args[0]))
    if len(args) > 1:
        input_path = os.path.normpath(os.path.abspath(args[1]))
    else:
        input_path = None
    status = runAllTests(jsontest_executable_path, input_path,
                         use_valgrind=options.valgrind,
                         with_json_checker=options.with_json_checker,
                         writerClass='StyledWriter')
    if status:
        sys.exit(status)
    status = runAllTests(jsontest_executable_path, input_path,
                         use_valgrind=options.valgrind,
                         with_json_checker=options.with_json_checker,
                         writerClass='StyledStreamWriter')
    if status:
        sys.exit(status)
    status = runAllTests(jsontest_executable_path, input_path,
                         use_valgrind=options.valgrind,
                         with_json_checker=options.with_json_checker,
                         writerClass='BuiltStyledStreamWriter')
    if status:
        sys.exit(status)

if __name__ == '__main__':
    main()
