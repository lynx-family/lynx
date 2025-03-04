#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Usage: collect_idls_into_json.py path_file.txt json_file.json
This script collects and organizes interface information and that information dumps into json file.
"""

import json
import os
import sys

from blinkpy.common import path_finder
path_finder.add_bindings_scripts_dir_to_sys_path()

import utilities
from blink_idl_parser import parse_file, BlinkIDLParser

_INTERFACE = 'Interface'
_IMPLEMENT = 'Implements'
_PARTIAL = 'Partial'
_NAME = 'Name'
_TYPE = 'Type'
_UNIONTYPE = 'UnionType'
_ARRAY = 'Array'
_ANY = 'Any'
_SEQUENCE = 'Sequence'
_PROP_VALUE = 'VALUE'
_VALUE = 'Value'
_PARENT = 'Parent'
_FILEPATH = 'FilePath'
_PROP_FILENAME = 'FILENAME'
_PROP_READONLY = 'READONLY'
_READONLY = 'Readonly'
_PROP_STATIC = 'STATIC'
_STATIC = 'Static'
_CONSTS = 'Consts'
_CONST = 'Const'
_ATTRIBUTES = 'Attributes'
_ATTRIBUTE = 'Attribute'
_OPERATIONS = 'Operations'
_OPERATION = 'Operation'
_PROP_GETTER = 'GETTER'
_NAMED_GETTER = '__getter__'
_PROP_SETTER = 'SETTER'
_NAMED_SETTER = '__setter__'
_PROP_DELETER = 'DELETER'
_NAMED_DELETER = '__deleter__'
_ARGUMENTS = 'Arguments'
_ARGUMENT = 'Argument'
_EXTATTRIBUTES = 'ExtAttributes'
_EXTATTRIBUTE = 'ExtAttribute'
_INHERIT = 'Inherit'
_PROP_REFERENCE = 'REFERENCE'
_PARTIAL_FILEPATH = 'Partial_FilePaths'
_MEMBERS = [_CONSTS, _ATTRIBUTES, _OPERATIONS]


def get_definitions(paths):
    """Returns a generator of IDL node.
    Args:
      paths: list of IDL file path
    Returns:
      a generator which yields IDL node objects
    """
    parser = BlinkIDLParser()
    for path in paths:
        definitions = parse_file(parser, path)
        for definition in definitions.GetChildren():
            yield definition


def is_implements(definition):
    """Returns True if class of |definition| is Implements, otherwise False.
    Args:
      definition: IDL node
    Returns:
      True if class of |definition| is Implements, otherwise False.
    """
    return definition.GetClass() == _IMPLEMENT


def is_partial(definition):
    """Returns True if |definition| is 'partial interface' class, otherwise False.
    Args:
      definition: IDL node
    Return:
      True if |definition| is 'partial interface' class, otherwise False.
    """
    return definition.GetClass() == _INTERFACE and definition.GetProperty(
        _PARTIAL)


def get_filepath(interface_node):
    """Returns relative path to the IDL in which |interface_node| is defined.
    Args:
      interface_node: IDL interface
    Returns:
      str which is |interface_node|'s file path
    """
    filename = interface_node.GetProperty(_PROP_FILENAME)
    return os.path.relpath(filename)


def get_const_node_list(interface_node):
    """Returns a list of Const node.
    Args:
      interface_node: interface node
    Returns:
      A list of const node
    """
    return interface_node.GetListOf(_CONST)


def get_const_type(const_node):
    """Returns const's type.
    Args:
      const_node: const node
    Returns:
      str which is constant type.
    """
    return const_node.GetChildren()[0].GetName()


def get_const_value(const_node):
    """Returns const's value.
    This function only supports primitive types.

    Args:
      const_node: const node
    Returns:
      str which is name of constant's value.
    """
    if const_node.GetChildren()[1].GetName():
        return const_node.GetChildren()[1].GetName()
    else:
        for const_child in const_node.GetChildren():
            if const_child.GetClass() == _VALUE and not const_child.GetName():
                return const_child.GetProperty(_PROP_VALUE)
        raise Exception('Constant value is empty')


def const_node_to_dict(const_node):
    """Returns dictionary of const's information.
    Args:
      const_node: const node
    Returns:
      dictionary of const's information
    """
    return {
        _NAME:
        const_node.GetName(),
        _TYPE:
        get_const_type(const_node),
        _VALUE:
        get_const_value(const_node),
        _EXTATTRIBUTES: [
            extattr_node_to_dict(extattr)
            for extattr in get_extattribute_node_list(const_node)
        ],
    }


def get_attribute_node_list(interface_node):
    """Returns list of Attribute if the interface have one.
    Args:
      interface_node: interface node
    Returns:
      list of attribute node
    """
    return interface_node.GetListOf(_ATTRIBUTE)


def get_attribute_type(attribute_node):
    """Returns type of attribute.
    Args:
      attribute_node: attribute node
    Returns:
      name of attribute's type
    """
    attr_type = attribute_node.GetOneOf(_TYPE).GetChildren()[0]
    type_list = []
    if attr_type.GetClass() == _UNIONTYPE:
        union_member_list = attr_type.GetListOf(_TYPE)
        for union_member in union_member_list:
            for type_component in union_member.GetChildren():
                if type_component.GetClass() == _ARRAY:
                    type_list[-1] += '[]'
                elif type_component.GetClass() == _SEQUENCE:
                    for seq_type in type_component.GetOneOf(
                            _TYPE).GetChildren():
                        type_list.append('<' + seq_type.GetName() + '>')
                else:
                    type_list.append(type_component.GetName())
        return type_list
    elif attr_type.GetClass() == _SEQUENCE:
        union_member_types = []
        if attr_type.GetOneOf(_TYPE).GetChildren()[0].GetClass() == _UNIONTYPE:
            for union_member in attr_type.GetOneOf(_TYPE).GetOneOf(
                    _UNIONTYPE).GetListOf(_TYPE):
                if len(union_member.GetChildren()) != 1:
                    raise Exception(
                        'Complex type in a union in a sequence is not yet supported'
                    )
                type_component = union_member.GetChildren()[0]
                union_member_types.append(type_component.GetName())
            return '<' + str(union_member_types) + '>'
        else:
            for type_component in attr_type.GetOneOf(_TYPE).GetChildren():
                if type_component.GetClass() == _SEQUENCE:
                    raise Exception(
                        'Sequence in another sequence is not yet supported')
                else:
                    if type_component.GetClass() == _ARRAY:
                        type_list[-1] += []
                    else:
                        type_list.append(type_component.GetName())
            return '<' + type_list[0] + '>'
    elif attr_type.GetClass() == _ANY:
        return _ANY
    else:
        for type_component in attribute_node.GetOneOf(_TYPE).GetChildren():
            if type_component.GetClass() == _ARRAY:
                type_list[-1] += '[]'
            else:
                type_list.append(type_component.GetName())
        return type_list[0]


get_operation_type = get_attribute_type
get_argument_type = get_attribute_type


def attribute_node_to_dict(attribute_node):
    """Returns dictioary of attribute's information.
    Args:
      attribute_node: attribute node
    Returns:
      dictionary of attribute's information
    """
    return {
        _NAME:
        attribute_node.GetName(),
        _TYPE:
        get_attribute_type(attribute_node),
        _EXTATTRIBUTES: [
            extattr_node_to_dict(extattr)
            for extattr in get_extattribute_node_list(attribute_node)
        ],
        _READONLY:
        attribute_node.GetProperty(_PROP_READONLY, default=False),
        _STATIC:
        attribute_node.GetProperty(_PROP_STATIC, default=False),
    }


def get_operation_node_list(interface_node):
    """Returns operations node list.
    Args:
      interface_node: interface node
    Returns:
      list of oparation node
    """
    return interface_node.GetListOf(_OPERATION)


def get_argument_node_list(operation_node):
    """Returns list of argument.
    Args:
      operation_node: operation node
    Returns:
      list of argument node
    """
    return operation_node.GetOneOf(_ARGUMENTS).GetListOf(_ARGUMENT)


def argument_node_to_dict(argument_node):
    """Returns dictionary of argument's information.
    Args:
      argument_node: argument node
    Returns:
      dictionary of argument's information
    """
    return {
        _NAME: argument_node.GetName(),
        _TYPE: get_argument_type(argument_node),
    }


def get_operation_name(operation_node):
    """Returns openration's name.
    Args:
      operation_node: operation node
    Returns:
      name of operation
    """
    if operation_node.GetProperty(_PROP_GETTER):
        return _NAMED_GETTER
    elif operation_node.GetProperty(_PROP_SETTER):
        return _NAMED_SETTER
    elif operation_node.GetProperty(_PROP_DELETER):
        return _NAMED_DELETER
    else:
        return operation_node.GetName()


def operation_node_to_dict(operation_node):
    """Returns dictionary of operation's information.
    Args:
      operation_node: operation node
    Returns:
      dictionary of operation's informantion
    """
    return {
        _NAME:
        get_operation_name(operation_node),
        _ARGUMENTS: [
            argument_node_to_dict(argument)
            for argument in get_argument_node_list(operation_node)
            if argument_node_to_dict(argument)
        ],
        _TYPE:
        get_operation_type(operation_node),
        _EXTATTRIBUTES: [
            extattr_node_to_dict(extattr)
            for extattr in get_extattribute_node_list(operation_node)
        ],
        _STATIC:
        operation_node.GetProperty(_PROP_STATIC, default=False),
    }


def get_extattribute_node_list(node):
    """Returns list of ExtAttribute.
    Args:
      node: IDL node
    Returns:
      list of ExtAttrbute
    """
    if node.GetOneOf(_EXTATTRIBUTES):
        return node.GetOneOf(_EXTATTRIBUTES).GetListOf(_EXTATTRIBUTE)
    else:
        return []


def extattr_node_to_dict(extattr):
    """Returns dictionary of ExtAttribute's information.
    Args:
      extattr: ExtAttribute node
    Returns:
      dictionary of ExtAttribute's information
    """
    return {
        _NAME: extattr.GetName(),
    }


def inherit_node_to_dict(interface_node):
    """Returns a dictionary of inheritance information.
    Args:
      interface_node: interface node
    Returns:
      A dictioanry of inheritance information.
    """
    inherit = interface_node.GetOneOf(_INHERIT)
    if inherit:
        return {_PARENT: inherit.GetName()}
    else:
        return {_PARENT: None}


def interface_node_to_dict(interface_node):
    """Returns a dictioary of interface information.
    Args:
      interface_node: interface node
    Returns:
      A dictionary of the interface information.
    """
    return {
        _NAME:
        interface_node.GetName(),
        _FILEPATH:
        get_filepath(interface_node),
        _CONSTS: [
            const_node_to_dict(const)
            for const in get_const_node_list(interface_node)
        ],
        _ATTRIBUTES: [
            attribute_node_to_dict(attr)
            for attr in get_attribute_node_list(interface_node) if attr
        ],
        _OPERATIONS: [
            operation_node_to_dict(operation)
            for operation in get_operation_node_list(interface_node)
            if operation
        ],
        _EXTATTRIBUTES: [
            extattr_node_to_dict(extattr)
            for extattr in get_extattribute_node_list(interface_node)
        ],
        _INHERIT:
        inherit_node_to_dict(interface_node)
    }


def merge_partial_dicts(interfaces_dict, partials_dict):
    """Merges partial interface into non-partial interface.
    Args:
      interfaces_dict: A dict of the non-partial interfaces.
      partial_dict: A dict of partial interfaces.
    Returns:
      A merged dictionary of |interface_dict| with |partial_dict|.
    """
    for interface_name, partial in partials_dict.iteritems():
        interface = interfaces_dict.get(interface_name)
        if not interface:
            raise Exception(
                'There is a partial interface, but the corresponding non-partial interface was not found.'
            )
        for member in _MEMBERS:
            interface[member].extend(partial.get(member))
            interface.setdefault(_PARTIAL_FILEPATH,
                                 []).append(partial[_FILEPATH])
    return interfaces_dict


def merge_implement_nodes(interfaces_dict, implement_node_list):
    """Combines a dict of interface information with referenced interface information.
    Args:
      interfaces_dict: dict of interface information
      implement_nodes: list of implemented interface node
    Returns:
      A dict of interface information combined with implements nodes.
    """
    for implement in implement_node_list:
        reference = implement.GetProperty(_PROP_REFERENCE)
        implement = implement.GetName()
        if (reference not in interfaces_dict.keys()
                or implement not in interfaces_dict.keys()):
            raise Exception(
                'There is not corresponding implement or reference interface.')
        for member in _MEMBERS:
            interfaces_dict[implement][member].extend(
                interfaces_dict[reference].get(member))
    return interfaces_dict


def export_to_jsonfile(dictionary, json_file):
    """Writes a Python dict into a JSON file.
    Args:
      dictioary: interface dictionary
      json_file: json file for output
    """
    with open(json_file, 'w') as f:
        json.dump(dictionary, f, sort_keys=True)


def usage():
    sys.stdout.write(
        'Usage: collect_idls_into_json.py <path_file.txt> <output_file.json>\n'
    )


def main(args):
    if len(args) != 2:
        usage()
        exit(1)
    path_file = args[0]
    json_file = args[1]
    path_list = utilities.read_file_to_list(path_file)
    implement_node_list = [
        definition for definition in get_definitions(path_list)
        if is_implements(definition)
    ]
    interfaces_dict = {
        definition.GetName(): interface_node_to_dict(definition)
        for definition in get_definitions(path_list)
        if not is_partial(definition)
    }
    partials_dict = {
        definition.GetName(): interface_node_to_dict(definition)
        for definition in get_definitions(path_list) if is_partial(definition)
    }
    dictionary = merge_partial_dicts(interfaces_dict, partials_dict)
    interfaces_dict = merge_implement_nodes(interfaces_dict,
                                            implement_node_list)
    export_to_jsonfile(dictionary, json_file)


if __name__ == '__main__':
    main(sys.argv[1:])
