#!/usr/bin/env python

import os
import unittest

from blinkpy.bindings import collect_idls_into_json
import utilities

from blink_idl_parser import parse_file, BlinkIDLParser

testdata_path = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), 'testdata')
_FILE = os.path.join(testdata_path, 'test_filepath.txt')
_KEY_SET = set([
    'Operations', 'Name', 'FilePath', 'Inherit', 'Consts', 'ExtAttributes',
    'Attributes'
])
_PARTIAL = {
    'Node': {
        'Operations': [],
        'Name':
        'Node',
        'FilePath':
        'Source/core/timing/WorkerGlobalScopePerformance.idl',
        'Inherit': [],
        'Consts': [],
        'ExtAttributes': [],
        'Attributes': [{
            'Static': False,
            'Readonly': True,
            'Type': 'WorkerPerformance',
            'Name': 'performance',
            'ExtAttributes': []
        }]
    }
}


class TestFunctions(unittest.TestCase):
    def setUp(self):
        parser = BlinkIDLParser()
        path = os.path.join(testdata_path,
                            utilities.read_file_to_list(_FILE)[0])
        definitions = parse_file(parser, path)
        self.definition = definitions.GetChildren()[0]

    def test_get_definitions(self):
        pathfile = utilities.read_file_to_list(_FILE)
        pathfile = [
            os.path.join(testdata_path, filename) for filename in pathfile
        ]
        for actual in collect_idls_into_json.get_definitions(pathfile):
            self.assertEqual(actual.GetName(), self.definition.GetName())

    def test_is_partial(self):
        if (self.definition.GetClass() == 'Interface'
                and self.definition.GetProperty('Partial')):
            self.assertTrue(collect_idls_into_json.is_partial(self.definition))
        else:
            self.assertFalse(
                collect_idls_into_json.is_partial(self.definition))

    def test_get_filepaths(self):
        filepath = collect_idls_into_json.get_filepath(self.definition)
        self.assertTrue(filepath.endswith('.idl'))

    def test_const_node_to_dict(self):
        const_member = set(['Name', 'Type', 'Value', 'ExtAttributes'])
        for const in collect_idls_into_json.get_const_node_list(
                self.definition):
            if const:
                self.assertEqual(const.GetClass(), 'Const')
                self.assertEqual(
                    collect_idls_into_json.get_const_type(const),
                    'unsigned short')
                self.assertEqual(
                    collect_idls_into_json.get_const_value(const), '1')
                self.assertTrue(
                    const_member.issuperset(
                        collect_idls_into_json.const_node_to_dict(const).
                        keys()))
            else:
                self.assertEqual(const, None)

    def test_attribute_node_to_dict(self):
        attribute_member = set(
            ['Name', 'Type', 'ExtAttributes', 'Readonly', 'Static'])
        for attribute in collect_idls_into_json.get_attribute_node_list(
                self.definition):
            if attribute:
                self.assertEqual(attribute.GetClass(), 'Attribute')
                self.assertEqual(attribute.GetName(), 'parentNode')
                self.assertEqual(
                    collect_idls_into_json.get_attribute_type(attribute),
                    'Node')
                self.assertTrue(
                    attribute_member.issuperset(
                        collect_idls_into_json.attribute_node_to_dict(
                            attribute).keys()))
            else:
                self.assertEqual(attribute, None)

    def test_operation_node_to_dict(self):
        operate_member = set(
            ['Static', 'ExtAttributes', 'Type', 'Name', 'Arguments'])
        argument_member = set(['Name', 'Type'])
        for operation in collect_idls_into_json.get_operation_node_list(
                self.definition):
            if operation:
                self.assertEqual(operation.GetClass(), 'Operation')
                self.assertEqual(operation.GetName(), 'appendChild')
                self.assertEqual(
                    collect_idls_into_json.get_operation_type(operation),
                    'Node')
                self.assertTrue(
                    operate_member.issuperset(
                        collect_idls_into_json.operation_node_to_dict(
                            operation).keys()))
                for argument in collect_idls_into_json.get_argument_node_list(
                        operation):
                    if argument:
                        self.assertEqual(argument.GetClass(), 'Argument')
                        self.assertEqual(argument.GetName(), 'newChild')
                        self.assertEqual(
                            collect_idls_into_json.get_argument_type(argument),
                            'Node')
                        self.assertTrue(
                            argument_member.issuperset(
                                collect_idls_into_json.argument_node_to_dict(
                                    argument).keys()))
                    else:
                        self.assertEqual(argument, None)
            else:
                self.assertEqual(operation, None)

    def test_extattribute_node_to_dict(self):
        for extattr in collect_idls_into_json.get_extattribute_node_list(
                self.definition):
            if extattr:
                self.assertEqual(extattr.GetClass(), 'ExtAttribute')
                self.assertEqual(extattr.GetName(), 'CustomToV8')
                self.assertEqual(
                    collect_idls_into_json.extattr_node_to_dict(extattr).
                    keys(), ['Name'])
                self.assertEqual(
                    collect_idls_into_json.extattr_node_to_dict(extattr).
                    values(), ['CustomToV8'])
            else:
                self.assertEqual(extattr, None)

    def test_inherit_node_to_dict(self):
        inherit = collect_idls_into_json.inherit_node_to_dict(self.definition)
        if inherit:
            self.assertEqual(inherit.keys(), ['Parent'])
            self.assertEqual(inherit.values(), ['EventTarget'])
        else:
            self.assertEqual(inherit, [])

    def test_interface_node_to_dict(self):
        self.assertTrue(
            _KEY_SET.issuperset(
                collect_idls_into_json.interface_node_to_dict(
                    self.definition)))

    def test_merge_partial_dicts(self):
        key_name = self.definition.GetName()
        merged = collect_idls_into_json.merge_partial_dicts({
            key_name:
            collect_idls_into_json.interface_node_to_dict(self.definition)
        }, _PARTIAL)[key_name]['Partial_FilePaths']
        expected = [
            'Source/core/timing/WorkerGlobalScopePerformance.idl',
            'Source/core/timing/WorkerGlobalScopePerformance.idl',
            'Source/core/timing/WorkerGlobalScopePerformance.idl',
        ]
        self.assertEqual(merged, expected)


if __name__ == '__main__':
    unittest.main()
