# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb
from enum import IntEnum


class LynxValueType(IntEnum):
    NULL = 0
    UNDEFINED = 1
    BOOL = 2
    DOUBLE = 3
    INT32 = 4
    UINT32 = 5
    INT64 = 6
    UINT64 = 7
    NAN = 8
    STRING = 9
    ARRAY = 10
    DICTIONARY = 11
    ARRAY_BUFFER = 12
    OBJECT = 13
    FUNCTION = 14
    EXTERNAL = 15
    EXTENDED = 16

class CSSValueSyntheticProvider:
  def __init__(self, val_obj, internal_dict):
    self.val_obj = val_obj

  def update(self):
    try:
      # get type field in lynx_value
      self.type_field = self.val_obj.GetChildMemberWithName('type_')
      self.type_value = self.type_field.GetValueAsUnsigned()
      
      # get val_ptr field in lynx_value
      self.val_ptr_field = self.val_obj.GetChildMemberWithName('val_ptr')
      
      self.child_count = 7
      self.load_addr = self.val_obj.GetLoadAddress()
    except:
      self.type_value = 0
      self.child_count = 0

  def num_children(self, max_children):
    return self.child_count

  def get_child_at_index(self,index):
    if index < 0 or index >= self.child_count:
      return None
    
    try:
      if index == 0:
        # If val_ptr is a RefCountedStringImpl raw pointer
        if self.type_value == LynxValueType.STRING:
          string_type = self.val_obj.GetTarget().FindFirstType(
              'lynx::base::RefCountedStringImpl').GetPointerType()
          return self.val_ptr_field.Cast(string_type)
        # If val_ptr is a CArray raw pointer
        elif self.type_value == LynxValueType.ARRAY:
          array_type = self.val_obj.GetTarget().FindFirstType(
              'lynx::lepus::CArray').GetPointerType()
          return self.val_ptr_field.Cast(array_type)
        # If val_ptr is a Dictionary raw pointer
        elif self.type_value == LynxValueType.DICTIONARY:
          dict_type = self.val_obj.GetTarget().FindFirstType(
              'lynx::lepus::Dictionary').GetPointerType()
          return self.val_ptr_field.Cast(dict_type)
        else:
          return self.get_union_value()

      if index == 1:
        return self.val_obj.GetChildMemberWithName('__attr__')
      if index == 2:
        return self.type_field
      if index == 3:
        return self.val_obj.GetChildMemberWithName('pattern_')
      if index == 4:
        return self.val_obj.GetChildMemberWithName('is_variable_')
      if index == 5:
        return self.val_obj.GetChildMemberWithName('needs_variable_resolution_')
      if index == 6:
        return self.val_obj.GetChildMemberWithName('optionals_')
    except:
      return None

  def has_children(self):
    return True

  def get_union_value(self):
    try:
      if self.type_value in [LynxValueType.BOOL, LynxValueType.NAN]:
        return self.val_obj.GetChildMemberWithName('val_bool')
      elif self.type_value == LynxValueType.DOUBLE:
        return self.val_obj.GetChildMemberWithName('val_double')
      else:
        return self.val_obj.GetChildMemberWithName('val_int64')
    except:
      return None

def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand(
      'type synthetic add -x "^lynx::tasm::CSSValue$" -l css_value_lldb.CSSValueSyntheticProvider -w liblynx'
    )
    debugger.HandleCommand('type category enable liblynx')