# Copyright 2015 Google Inc. All rights reserved.
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

import io
import math
import os
import sys
import unittest

import json5


class TestLoads(unittest.TestCase):
    maxDiff = None

    def check(self, s, obj):
        self.assertEqual(json5.loads(s), obj)

    def check_fail(self, s, err=None):
        try:
            json5.loads(s)
            self.fail()  # pragma: no cover
        except ValueError as e:
            if err:
                self.assertEqual(err, str(e))

    def test_arrays(self):
        self.check('[]', [])
        self.check('[0]', [0])
        self.check('[0,1]', [0, 1])
        self.check('[ 0 , 1 ]', [0, 1])

    def test_bools(self):
        self.check('true', True)
        self.check('false', False)

    def test_cls_is_not_supported(self):
        self.assertRaises(AssertionError, json5.loads, '1', cls=lambda x: x)

    def test_empty_strings_are_errors(self):
        self.check_fail('', 'Empty strings are not legal JSON5')

    def test_encoding(self):
        if sys.version_info[0] < 3:
          s = '"\xf6"'
        else:
          s = b'"\xf6"'
        self.assertEqual(json5.loads(s, encoding='iso-8859-1'),
                         u"\xf6")


    def test_numbers(self):
        # decimal literals
        self.check('1', 1)
        self.check('-1', -1)
        self.check('+1', 1)

        # hex literals
        self.check('0xf', 15)
        self.check('0xfe', 254)
        self.check('0xfff', 4095)
        self.check('0XABCD', 43981)
        self.check('0x123456', 1193046)

        # floats
        self.check('1.5', 1.5)
        self.check('1.5e3', 1500.0)
        self.check('-0.5e-2', -0.005)

        # names
        self.check('Infinity', float('inf'))
        self.check('-Infinity', float('-inf'))
        self.assertTrue(math.isnan(json5.loads('NaN')))
        self.assertTrue(math.isnan(json5.loads('-NaN')))

        # syntax errors
        self.check_fail('14d', '<string>:1 Unexpected "d" at column 3')

    def test_identifiers(self):
        self.check('{a: 1}', {'a': 1})
        self.check('{$: 1}', {'$': 1})
        self.check('{_: 1}', {'_': 1})
        self.check('{a_b: 1}', {'a_b': 1})
        self.check('{a$: 1}', {'a$': 1})

        # This valid JavaScript but not valid JSON5; keys must be identifiers
        # or strings.
        self.check_fail('{1: 1}')

    def test_identifiers_unicode(self):
        self.check(u'{\xc3: 1}', {u'\xc3': 1})

    def test_null(self):
        self.check('null', None)

    def test_object_hook(self):
        hook = lambda d: [d]
        self.assertEqual(json5.loads('{foo: 1}', object_hook=hook),
                         [{"foo": 1}])

    def test_object_pairs_hook(self):
        hook = lambda pairs: pairs
        self.assertEqual(json5.loads('{foo: 1, bar: 2}',
                                     object_pairs_hook=hook),
                         [('foo', 1), ('bar', 2)])

    def test_objects(self):
        self.check('{}', {})
        self.check('{"foo": 0}', {"foo": 0})
        self.check('{"foo":0,"bar":1}', {"foo": 0, "bar": 1})
        self.check('{ "foo" : 0 , "bar" : 1 }', {"foo": 0, "bar": 1})

    def test_parse_constant(self):
        hook = lambda x: x
        self.assertEqual(json5.loads('-Infinity', parse_constant=hook),
                         '-Infinity')
        self.assertEqual(json5.loads('NaN', parse_constant=hook),
                         'NaN')

    def test_parse_float(self):
        hook = lambda x: x
        self.assertEqual(json5.loads('1.0', parse_float=hook), '1.0')

    def test_parse_int(self):
        hook = lambda x, base=10: x
        self.assertEqual(json5.loads('1', parse_int=hook), '1')

    def test_sample_file(self):
        path = os.path.join(os.path.dirname(__file__), '..', '..',
                            'sample.json5')
        with open(path) as fp:
            obj = json5.load(fp)
        self.assertEqual({
            u'oh': [
                u"we shouldn't forget",
                u"arrays can have",
                u"trailing commas too",
            ],
            u"this": u"is a multi-line string",
            u"delta": 10,
            u"hex": 3735928559,
            u"finally": "a trailing comma",
            u"here": "is another",
            u"to": float("inf"),
            u"while": True,
            u"half": 0.5,
            u"foo": u"bar"
            }, obj)

    def test_strings(self):
        self.check('"foo"', 'foo')
        self.check("'foo'", 'foo')

        # escape chars
        self.check("'\\b\\t\\f\\n\\r\\v\\\\'", '\b\t\f\n\r\v\\')
        self.check("'\\''", "'")
        self.check('"\\""', '"')

        # hex literals
        self.check('"\\x66oo"', 'foo')

        # unicode literals
        self.check('"\\u0066oo"', 'foo')

        # string literals w/ continuation markers at the end of the line.
        # These should not have spaces is the result.
        self.check('"foo\\\nbar"', 'foobar')
        self.check("'foo\\\nbar'", 'foobar')

        # unterminated string literals.
        self.check_fail('"\n')
        self.check_fail("'\n")

        # bad hex literals
        self.check_fail("'\\x0'")
        self.check_fail("'\\xj'")
        self.check_fail("'\\x0j'")

        # bad unicode literals
        self.check_fail("'\\u0'")
        self.check_fail("'\\u00'")
        self.check_fail("'\\u000'")
        self.check_fail("'\\u000j'")
        self.check_fail("'\\u00j0'")
        self.check_fail("'\\u0j00'")
        self.check_fail("'\\uj000'")

    def test_whitespace(self):
        self.check('\n1', 1)
        self.check('\r1', 1)
        self.check('\r\n1', 1)
        self.check('\t1', 1)
        self.check('\v1', 1)
        self.check(u'\uFEFF 1', 1)
        self.check(u'\u00A0 1', 1)
        self.check(u'\u2028 1', 1)
        self.check(u'\u2029 1', 1)


class TestDump(unittest.TestCase):
    def test_basic(self):
        sio = io.StringIO()
        json5.dump(True, sio)
        self.assertEqual('true', sio.getvalue())


class TestDumps(unittest.TestCase):
    maxDiff = None

    def check(self, obj, s):
        self.assertEqual(json5.dumps(obj, compact=True), s)

    def test_arrays(self):
        self.check([], '[]')
        self.check([1, 2, 3], '[1,2,3]')

    def test_bools(self):
        self.check(True, 'true')
        self.check(False, 'false')

    def test_numbers(self):
        self.check(15, '15')

    def test_null(self):
        self.check(None, 'null')

    def test_objects(self):
        self.check({'foo': 1}, '{foo:1}')
        self.check({'foo bar': 1}, '{"foo bar":1}')

    def test_strings(self):
        self.check("'single'", '"\'single\'"')
        self.check('"double"', "'\"double\"'")
        self.check("'single \\' and double \"'",
                   '"\'single \\\\\' and double \\"\'"')
