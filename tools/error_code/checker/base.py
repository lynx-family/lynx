# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys

METADATA_TYPE_STR = "string"
METADATA_TYPE_NUMBER = "number"
METADATA_TYPE_BOOL = "bool"
METADATA_TYPE_ENUM = "enum"

# error message template
KEY_MISSING_TEMPLATE = "The key \'{0}\' is missing{1}"
NON_INTEGER_TEMPLATE = "The \'{0}\' must be a int number{1}"
NON_EMPTY_STR_TEMPLATE = "The \'{0}\' must be a non-empty string{1}"
EXCEED_CODE_RANGE_TEMPLATE = "The \'{0}\' must be in [0, 99]{1}"
SMALLER_THAN_PREVIOUS_TEMPLATE = "The \'{0}\' must be greater than the previous one{1}"
USELESS_KEYWORD_TEMPLATE = "The keyword \'{0}\' is useless{1}"
UNDECLARED_ENUM_VALUE_TEMPLATE = '''\
The value \'{0}\' for the keyword \'{1}\' is not \
declared in the \'{2}\' of metadata \'{3}\'{4}'''

g_has_error = False

def _abort(msg):
    print("\033[91m[Internal Error] " + msg + "\033[0m")
    sys.exit(1)

def _print_error(msg):
    print("\033[91m[Error] " + msg + "\033[0m")
        
def _print_warn(msg):
    print("\033[93m[Warn] " + msg + "\033[0m")
    

class ErrorChecker:
    def has_error(self):
        return g_has_error

    def _printError(self, msg):
        global g_has_error
        _print_error(msg)
        if not g_has_error:
            g_has_error = True
    
    def _printWarn(self, msg):
        _print_warn(msg)

    def _fatal(self, msg):
        _abort(msg)
