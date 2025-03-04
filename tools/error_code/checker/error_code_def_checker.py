# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from checker.base import *

__all__ = [ "ErrorCodeDefChecker" ]

RESERVED_NAMES_WITH_ZERO_CODE = ["Success", "Default"]

RESERVED_NAMES_WITHOUT_DESCRIPTION = [ "Default" ]

# node types
NODE_TYPE_ROOT = "root"
NODE_TYPE_SECTION = "section"
NODE_TYPE_BEHAVIOR = "behavior"
NODE_TYPE_SUB_CODE = "code"

# global variables
g_declared_metadata_keywords = []
g_keyword_to_metadata = {}

def _create_node_checker(node, parent_node, node_index, key_in_parent):
    if key_in_parent == KEY_SECTIONS:
        return SectionChecker(node, parent_node, node_index)
    if key_in_parent == KEY_BEHAVIORS:
        return BehaviorChecker(node, parent_node, node_index)
    if key_in_parent == KEY_CODES:
        return SubCodeChecker(node, parent_node, node_index)

class ErrorCodeTreeNodeChecker(ErrorChecker):
    def __init__(self,
                 node, 
                 parent_node,
                 node_index,
                 builtin_keywords):
        super().__init__()
        self._type = "unknown"
        self._node = node
        self._parent = parent_node
        self._node_index = node_index
        self._builtin_keywords = builtin_keywords
        self._extended_keywords = g_declared_metadata_keywords
        self._checked_child_names = []
        self._err_pos_cache = None
        self._key_for_code = None
        self._key_for_children = None

    def check(self):
        if self._node == None:
            self._fatal(
                "The node for ErrorCodeTreeNodeChecker is None") 
        self._check_name()
        self._check_code()
        self._check_description()
        self._check_extended_keywords()
        self._check_child()
        return not self.has_error()

    def name(self):
        if self._node != None and self._type != NODE_TYPE_ROOT:
            name = self._node.get(KEY_NAME, "")
            return name if name != None else ""
        return ""

    def child_at(self, index):
        if self._key_for_children == None:
            return None
        children = self._node.get(self._key_for_children, None)
        if children == None:
            return None
        if index >= len(children) or index < 0:
            self._fatal("invalid index")
            return None
        return children[index]

    def record_child_name(self, name):
        if name in self._checked_child_names:
            return False
        self._checked_child_names.append(name)
        return True

    def _check_name(self):
        if self._type == NODE_TYPE_ROOT:
            return
        name = self._node.get(KEY_NAME, None)
        if name == None:
            self._printError(
                "The key \'{0}\' is missing, the node index in parent is {1}{2}"
                .format(KEY_NAME, self._node_index, self._err_pos()))
        elif type(name) != str or name == "":
            self._printError(
                "The \'{0}\' must be a non-empty string{1}".format(KEY_NAME, self._err_pos()))
        elif is_pascal_case(name) == False:
            self._printError(
                "The \'{0}\' must be pascal case{1}".format(KEY_NAME, self._err_pos()))
        elif self._parent != None:
            if not self._parent.record_child_name(name):
                self._printError(
                    "Duplicated name \'{0}\'{1}".format(name, self._err_pos()))

    def _check_code(self):
        if self._type == NODE_TYPE_ROOT:
            return
        if self._key_for_code == None:
            self._fatal(
                "The key for code is None, the node type is {0}"
                .format(self._type))
        code = self._node.get(self._key_for_code, None)
        name = self._node.get(KEY_NAME, "")
        if code == None:
            self._printError(
                KEY_MISSING_TEMPLATE
                .format(self._key_for_code, self._err_pos()))
        elif type(code) != int:
            self._printError(
                NON_INTEGER_TEMPLATE
                .format(self._key_for_code, self._err_pos()))
        elif code < 0 or code > 99:
            self._printError(
                EXCEED_CODE_RANGE_TEMPLATE
                .format(self._key_for_code, self._err_pos()))
        elif code == 0:
            if name not in RESERVED_NAMES_WITH_ZERO_CODE:
                self._printError("Only names like {0} can have code 0{1}"
                                 .format(str(RESERVED_NAMES_WITH_ZERO_CODE), self._err_pos()))
        elif name in RESERVED_NAMES_WITH_ZERO_CODE:
            self._printWarn("when the name is in the list {0}, the code should be 0{1}"
                           .format(str(RESERVED_NAMES_WITH_ZERO_CODE), self._err_pos()))
        else:
            if self._node_index <= 0 or self._parent == None:
                return
            previous_node = self._parent.child_at(self._node_index - 1)
            if type(previous_node) != int:
                return
            previous_code = previous_node.get(self._key_for_code, None)
            if previous_code != None and code <= previous_code:
                self._printError(
                    SMALLER_THAN_PREVIOUS_TEMPLATE
                    .format(self._key_for_code, self._err_pos()))

    def _check_description(self):
        if self._type == NODE_TYPE_ROOT:
            return
        desc = self._node.get(KEY_DESC, None)
        if desc == None:
            self._printError(
                KEY_MISSING_TEMPLATE.format(KEY_DESC, self._err_pos()))
        elif type(desc) != str:
            self._printError(
                NON_EMPTY_STR_TEMPLATE
                .format(KEY_DESC, self._err_pos()))
        elif desc == "":
            name = self._node.get(KEY_NAME, "")
            if name not in RESERVED_NAMES_WITHOUT_DESCRIPTION:
                self._printError(
                    NON_EMPTY_STR_TEMPLATE
                    .format(KEY_DESC, self._err_pos()))

    def _check_extended_keywords(self):
        for k, v in self._node.items():
            if k in self._builtin_keywords:
                continue
            if k in self._extended_keywords:
                self._check_metadata(k, v)
            else:
                self._printWarn(
                    USELESS_KEYWORD_TEMPLATE.format(k, self._err_pos()))

    def _check_metadata(self, keyword, value):
        metadata = g_keyword_to_metadata.get(keyword, None)
        if metadata == None:
            self._printError(
                "No metadata with the keyword of \'{0}\' was found."
                .format(keyword))
            return
        # We have already checked metadata in the
        # MetadataChecker, so we can use it safely now.
        metadata_type = metadata[KEY_TYPE]
        metadata_name = metadata[KEY_NAME]
        metadata_value_is_list = metadata.get(KEY_MULTI_SELECTION, False)
        if metadata_type == METADATA_TYPE_STR :
            if type(value) != str or value == "":
                self._printError(
                    "The value must be a string" + self._err_pos())
            return
        elif metadata_type == METADATA_TYPE_NUMBER:
            if type(value) != int:
                self._printError(
                    "The value must be a int number" + self._err_pos())
            return
        elif metadata_type == METADATA_TYPE_BOOL:
            if type(value) != bool:
                self._printError(
                    "The value must be a bool" + self._err_pos())
            return
        # handle the situation where metadata type is enum
        values = metadata[KEY_VALUES]
        error_msg = (
            UNDECLARED_ENUM_VALUE_TEMPLATE
            .format(value, keyword, KEY_VALUES, metadata_name, self._err_pos()))
        if type(value) == str:
            if value not in values:
                self._printError(error_msg)
            return
        if type(value) != list:
            self._printError(
                "The value must be a list or string" + self._err_pos())
            return
        if not metadata_value_is_list:
            self._printError(
                "The value for \'{0}\' should be a string{1}"
                .format(keyword, self._err_pos()))
            return
        # handle the situation where the value for keyword is a list
        for v in value:
            if v in values:
                continue
            self._printError(
                UNDECLARED_ENUM_VALUE_TEMPLATE
                .format(v, keyword, KEY_VALUES, metadata_name, self._err_pos()))

    def _check_child(self):
        if self._key_for_children == None:
            return
        children = self._node.get(self._key_for_children, None)
        if type(children) != list:
            self._printError(
                "The \'{0}\' must be a list{1}"
                .format(self._key_for_children, self._err_pos()))
            return
        for i, c in enumerate(children):
            child = _create_node_checker(
                c, self, i, self._key_for_children)
            child.check()
    
    def _err_pos(self):
        if self._err_pos_cache != None:
            return self._err_pos_cache
        pos_list = []
        p = self
        while p != None and p._parent != None:
            if p != self:
                pos_list.append(",")
            pos_list.append(
                " {0} \'{1}\'".format(p._type, p.name()))
            p = p._parent
        if len(pos_list) == 0:
            pos_list.append(" root node")
        pos_list.append(" (Error position:")
        pos_list.reverse()
        pos_list.append(")")
        self._err_pos_cache = "".join(pos_list)
        return self._err_pos_cache

class ErrorCodeDefChecker(ErrorCodeTreeNodeChecker):
    def __init__(self, node):
        super().__init__(node, None, 0, [])
        self._type = NODE_TYPE_ROOT
        self._key_for_children = KEY_SECTIONS
        self._builtin_keywords = [KEY_SECTIONS, KEY_METADATA]
        self._extended_keywords = []

    def prepare_check(self, declared_metadata_keywords, keyword_to_metadata):
        global g_declared_metadata_keywords
        global g_keyword_to_metadata
        g_declared_metadata_keywords = declared_metadata_keywords
        g_keyword_to_metadata = keyword_to_metadata


class SectionChecker(ErrorCodeTreeNodeChecker):
    ATTR_FOR_SECTION = [KEY_NAME, KEY_HIGH_CODE, KEY_DESC, KEY_BEHAVIORS]
    def __init__(self,
                 node,
                 parent_node,
                 node_index):
        super().__init__(
            node,
            parent_node,
            node_index,
            self.ATTR_FOR_SECTION)
        self._type = NODE_TYPE_SECTION
        self._key_for_children = KEY_BEHAVIORS
        self._key_for_code = KEY_HIGH_CODE
        self._extended_keywords = []


class BehaviorChecker(ErrorCodeTreeNodeChecker):
    ATTR_FOR_BEHAVIOR = [KEY_NAME, KEY_MID_CODE, KEY_DESC, KEY_CODES]

    def __init__(self,
                 node,
                 parent_node,
                 node_index):
        super().__init__(
            node,
            parent_node,
            node_index,
            self.ATTR_FOR_BEHAVIOR)
        self._type = NODE_TYPE_BEHAVIOR
        self._key_for_children = KEY_CODES
        self._key_for_code = KEY_MID_CODE

class SubCodeChecker(ErrorCodeTreeNodeChecker):
    ATTR_FOR_SUB_CODE = [KEY_NAME, KEY_LOW_CODE, KEY_DESC]

    def __init__(self,
                 node,
                 parent_node,
                 node_index):
        super().__init__(
            node,
            parent_node,
            node_index,
            self.ATTR_FOR_SUB_CODE)
        self._type = NODE_TYPE_SUB_CODE
        self._key_for_code = KEY_LOW_CODE
