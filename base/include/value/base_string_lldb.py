# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb

def get_string_summary(valobj, internal_dict):
    """Get summary for lynx::base::String.
    
    Args:
        valobj: The value object of lynx::base::String
        internal_dict: Internal dictionary
        
    Returns:
        A string containing the string content
    """
    try:
        ref_impl = valobj.GetChildMemberWithName('ref_impl_')
        if not ref_impl:
            return '<invalid>'
        ptr_value = ref_impl.GetValueAsUnsigned()
        is_tagged = (ptr_value & 1) == 1
        tag_info = f"[ref_impl_=0x{ptr_value:x}, tagged={is_tagged}] "
        expr = valobj.EvaluateExpression('((lynx::base::RefCountedStringImpl*)((uintptr_t)ref_impl_ & ~1))->str_')
        str_value = expr.GetSummary() if expr.IsValid() else None
        if str_value and str_value != '""':
            if str_value.startswith('"') and str_value.endswith('"'):
                str_value = str_value[1:-1]
            return tag_info + str_value
        else:
            return tag_info + "<empty>"
    except Exception as e:
        return f'<error: {str(e)}>'

def __lldb_init_module(debugger, internal_dict):
    """Initialize the LLDB module.
    
    Args:
        debugger: The LLDB debugger
        internal_dict: Internal dictionary
    """
    # Add type summary for lynx::base::String
    debugger.HandleCommand(
        'type summary add -F base_string_lldb.get_string_summary -x "^lynx::base::String$" -w liblynx'
    )
    debugger.HandleCommand('type category enable liblynx')