# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from common import *
from checker.base import *

class MetadataChecker(ErrorChecker):
    ATTR_FOR_METADATA = [KEY_NAME, KEY_TYPE, KEY_KEYWORD, 
                         KEY_DEFAULT, KEY_VALUES, KEY_MULTI_SELECTION]
    VALUE_OF_METADATA_TYPE = [METADATA_TYPE_STR, METADATA_TYPE_NUMBER, 
                              METADATA_TYPE_BOOL, METADATA_TYPE_ENUM]

    def __init__(self, metadata_list):
        super().__init__()
        self._metadata_list = metadata_list
        self._declared_metadata_keywords = []
        self._declared_metadata_name = []
        self._keyword_to_metadata = {}

    def check(self):
        if self._metadata_list == None:
            self._printWarn("Cannot find \'metadata\' list")
            return False
        self._traverse_metadata_list()
        return not self.has_error()

    def declared_metadata_keywords(self):
        return self._declared_metadata_keywords
    
    def keyword_to_metadata(self):
        return self._keyword_to_metadata

    def _traverse_metadata_list(self):
        if type(self._metadata_list)!= list:
            self._printError("The \'metadata\' must be a list")
            return
        for i, m in enumerate(self._metadata_list):
            self._check_metadata(i, m)


    def _check_metadata(self, index, metadata):
        name = metadata.get(KEY_NAME, "")
        m_type = metadata.get(KEY_TYPE, None)
        self._check_metadata_name(index, metadata)
        is_type_valid = self._check_metadata_type(index, metadata)
        self._check_metadata_keyword(index, metadata)
        if is_type_valid:
            self._check_metadata_default_and_values(index, metadata)
        for k, v in metadata.items():
            if k in self.ATTR_FOR_METADATA:
                continue
            if k == KEY_VALUES and m_type != METADATA_TYPE_ENUM:
                self._printWarn(
                    "The key \'{0}\' of the metadata \'{1}\' is useless"
                    .format(k, name))
        

    def _check_metadata_name(self, index, metadata):
        name = metadata.get(KEY_NAME, None)
        if name == None:
            self._printError(
                "The {0}th metadata must have key \'name\'".format(index))
        elif type(name) != str:
            self._printError("The \'name\' for the {0}th metadata must be a string".format(index))
        elif name == "":
            self._printError("The \'name\' for the {0}th metadata must not be empty".format(index))
        elif not is_pascal_case(name):
            self._printError("The \'name\' for the {0}th metadata must be pascal case".format(index))
        elif name in self._declared_metadata_name:
            self._printError("The name \'{0}\' of the metadata is duplicated".format(name))
        else:
            self._declared_metadata_name.append(name)

    def _check_metadata_type(self, index, metadata):
        metadata_type = metadata.get(KEY_TYPE, None)
        if metadata_type == None:
            self._printError("The {0}th metadata must have a type".format(index))
        elif metadata_type not in self.VALUE_OF_METADATA_TYPE:
            self._printError(
                "The \'type\' for the {0}th metadata must be one of "
                .format(index) + str(self.VALUE_OF_METADATA_TYPE))
        else:
            return True
        return False

    def _check_metadata_keyword(self, index, metadata):
        keyword = metadata.get(KEY_KEYWORD, None)
        pattern = re.compile('[^a-z-]')
        if keyword == None:
            self._printError(
                "The {0}th metadata must have a \'{1}\'"
                .format(index, KEY_KEYWORD))
        elif type(keyword)!= str:
            self._printError(
                "The \'{1}\' for the {0}th metadata must be a string"
                .format(index, KEY_KEYWORD))
        elif keyword == "":
            self._printError(
                "The \'{1}\' for the {0}th metadata must not be empty"
                .format(index, KEY_KEYWORD))
        elif pattern.search(keyword)!= None:
            self._printError(
                "The \'{1}\' for the {0}th metadata must contains only '-' and lower case letter"
                .format(index, KEY_KEYWORD))
        elif keyword in self._declared_metadata_keywords:
            self._printError(
                "The keyword \'{1}\' of the {0}th metadata is duplicated"
                .format(index, keyword))
        else:
            self._declared_metadata_keywords.append(keyword)
            self._keyword_to_metadata[keyword] = metadata
    
    def _check_metadata_default_and_values(self, index, metadata):
        default = metadata.get(KEY_DEFAULT, None)
        metadata_type = metadata.get(KEY_TYPE, None)
        values = metadata.get(KEY_VALUES, None)
        if metadata_type == None:
            self._printError(
                "The {0}th metadata must have a type"
                .format(index))
        if default == None:
            self._printError(
                "The {0}th metadata must have a \'{1}\' value"
                .format(index, KEY_DEFAULT))
        elif metadata_type == METADATA_TYPE_STR:
            if type(default)!= str:
                self._printError(
                    "The \'{1}\' for the {0}th metadata must be a string"
                    .format(index, KEY_DEFAULT))
        elif metadata_type == METADATA_TYPE_NUMBER:
            if type(default)!= int:
                self._printError(
                    "The \'{1}\' for the {0}th metadata must be a int number"
                    .format(index, KEY_DEFAULT))
        elif metadata_type == METADATA_TYPE_BOOL:
            if type(default)!= bool:
                self._printError(
                    "The \'{1}\' the {0}th metadata must be a bool"
                    .format(index, KEY_DEFAULT))
        elif metadata_type == METADATA_TYPE_ENUM:
            if values == None or type(values)!= list:
                self._printError(
                    "The \'{1}\' for the {0}th metadata must be a list of string"
                    .format(index, KEY_VALUES))
                return
            for v in values:
                if type(v)!= str or v == "":
                    self._printError(
                        "The \'{1}\' for the {0}th metadata must be a list of string"
                        .format(index, KEY_VALUES))
            if type(default) == str:
                if default not in values:
                    self._printError(
                        "The \'{1}\' for the {0}th metadata must be in \'{2}\'"
                        .format(index, KEY_DEFAULT, KEY_VALUES))
                return
            if type(default) != list:
                self._printError(
                    "The \'{1}\' for the {0}th metadata must be a list or string"
                    .format(index, KEY_DEFAULT))
                return
            for d in default:
                if d not in values:
                    self._printError(
                        "The element in \'{1}\' for the {0}th metadata must have been declared in \'{2}\'"
                        .format(index, KEY_DEFAULT, KEY_VALUES))
                    return
