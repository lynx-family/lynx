#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""generate_idl_diff.py is a script that generates a diff of two given IDL files.
Usage: generate_idl_diff.py old_file.json new_file.json diff_file.json
    old_file.json: An input json file including idl data of old Chrome version
    new_file.json: An input json file including idl data of new Chrome version
    diff_file.json: An output json file expressing a diff between old_file.json
        and new_file.json
"""

import json
import sys

# pylint: disable=W0105
"""Data structure of input files of this script.
The format of the json files is as follows. Each json file contains multiple
"interface"s. Each "interface" contains 'ExtAttributes', 'Consts', 'Attributes'
and 'Operations'. Each item in them are called a "member".
    {'InterfaceName': {
            'ExtAttributes': [{'Name': '...'},
                               ...,
                             ],
            'Consts': [{'Type': '...',
                        'Name': '...',
                        'Value': '...'
                       },
                       ...,
                      ],
            'Attributes': [{'Type': '...',
                            'Name': '...',
                            'ExtAttributes':[{'Name': '...'},
                                              ...,
                                            ]
                           },
                           ...,
                          ],
            'Operations': [{'Type': '...',
                            'Name': '...',
                            'ExtAttributes': [{'Name': '...'},
                                               ...,
                                             ]
                            'Arguments': [{'Type': '...',
                                           'Name': '...'},
                                           ...,
                                         ]
                           },
                           ...,
                          ],
            'Name': '...'
        },
        ...,
    }
"""

EXTATTRIBUTES_AND_MEMBER_TYPES = [
    'ExtAttributes', 'Consts', 'Attributes', 'Operations'
]
DIFF_INSENSITIVE_FIELDS = ['Name']
DIFF_TAG = 'diff_tag'
DIFF_TAG_ADDED = 'added'
DIFF_TAG_DELETED = 'deleted'


def load_json_file(filepath):
    """Load a json file into a dictionary.
    Args:
        filepath: A json file path of a json file that we want to load
    Returns:
        An "interfaces" object loaded from the json file
    """
    with open(filepath, 'r') as f:
        return json.load(f)


def members_diff(old_interface, new_interface):
    """Create a diff between two "interface" objects by adding annotations to
    "member" objects that are not common in them.
    Args:
        old_interface: An "interface" object
        new_interface: An "interface" object
    Returns:
        (annotated, is_changed) where
        annotated: An annotated "interface" object
        is_changed: True if two interfaces are not identical, otherwise False
    """
    annotated = {}
    is_changed = False
    for member_type in EXTATTRIBUTES_AND_MEMBER_TYPES:
        annotated_members = []
        unannotated_members = []
        for member in new_interface[member_type]:
            if member in old_interface[member_type]:
                unannotated_members.append(member)
                old_interface[member_type].remove(member)
            else:
                is_changed = True
                member[DIFF_TAG] = DIFF_TAG_ADDED
                annotated_members.append(member)
        annotated[member_type] = annotated_members
        annotated[member_type].extend(unannotated_members)
    for member_type in EXTATTRIBUTES_AND_MEMBER_TYPES:
        for member in old_interface[member_type]:
            is_changed = True
            member[DIFF_TAG] = DIFF_TAG_DELETED
        annotated[member_type].extend(old_interface[member_type])
    for field in DIFF_INSENSITIVE_FIELDS:
        annotated[field] = old_interface[field]
    return (annotated, is_changed)


def annotate_all_members(interface, diff_tag):
    """Add annotations to all "member" objects of |interface|.
    Args:
        interface: An "interface" object whose members should be annotated with
            |diff_tag|.
        diff_tag: DIFF_TAG_ADDED or DIFF_TAG_DELETED
    Returns:
        Annotated "interface" object
    """
    for member_type in EXTATTRIBUTES_AND_MEMBER_TYPES:
        for member in interface[member_type]:
            member[DIFF_TAG] = diff_tag
    return interface


def interfaces_diff(old_interfaces, new_interfaces):
    """Compare two "interfaces" objects and create a diff between them by
    adding annotations (DIFF_TAG_ADDED or DIFF_TAG_DELETED) to each member
    and/or interface.
    Args:
        old_interfaces: An "interfaces" object
        new_interfaces: An "interfaces" object
    Returns:
        An "interfaces" object representing diff between |old_interfaces| and
        |new_interfaces|
    """
    annotated = {}
    for interface_name, interface in new_interfaces.items():
        if interface_name in old_interfaces:
            annotated_interface, is_changed = members_diff(
                old_interfaces[interface_name], interface)
            if is_changed:
                annotated[interface_name] = annotated_interface
            del old_interfaces[interface_name]
        else:
            interface = annotate_all_members(interface, DIFF_TAG_ADDED)
            interface[DIFF_TAG] = DIFF_TAG_ADDED
            annotated[interface_name] = interface
    for interface_name, interface in old_interfaces.items():
        interface = annotate_all_members(interface, DIFF_TAG_DELETED)
        interface[DIFF_TAG] = DIFF_TAG_DELETED
    annotated.update(old_interfaces)
    return annotated


def write_diff(diff, filepath):
    """Write a diff dictionary to a json file.
    Args:
        diff: An "interfaces" object that represents a diff
        filepath: An output file path
    """
    with open(filepath, 'w') as f:
        json.dump(diff, f, indent=4)


def main(argv):
    if len(argv) != 3:
        sys.stdout.write('Usage: make_diff.py <old_file.json> <new_file.json> '
                         '<diff_file.json>\n')
        exit(1)
    old_json_file = argv[0]
    new_json_file = argv[1]
    output_file = argv[2]
    old_interfaces = load_json_file(old_json_file)
    new_interfaces = load_json_file(new_json_file)
    diff = interfaces_diff(old_interfaces, new_interfaces)
    write_diff(diff, output_file)


if __name__ == '__main__':
    main(sys.argv[1:])
