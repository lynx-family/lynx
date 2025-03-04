# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb

# LLDB formatter for LinkedHashMap container.


class linked_hash_map_entry:

    def __init__(self, entry):
        self.entry = entry

    def _next_impl(self):
        return linked_hash_map_entry(self.entry.GetChildMemberWithName('next'))

    def _prev_impl(self):
        return linked_hash_map_entry(self.entry.GetChildMemberWithName('prev'))

    def _value_impl(self):
        return self.entry.GetValueAsUnsigned(0)

    def _isnull_impl(self):
        return self._value_impl() == 0

    def _sbvalue_impl(self):
        return self.entry

    next = property(_next_impl, None)
    value = property(_value_impl, None)
    is_null = property(_isnull_impl, None)
    sbvalue = property(_sbvalue_impl, None)


class linked_hash_map_iterator:

    def increment_node(self, node):
        if node.is_null:
            return None
        return node.next

    def __init__(self, node):
        self.node = linked_hash_map_entry(node)

    def value(self):
        return self.node.sbvalue  # and return the SBValue back on exit

    def next(self):
        node = self.increment_node(self.node)
        if node is not None and node.sbvalue.IsValid() and not (node.is_null):
            self.node = node
            return self.value()
        else:
            return None

    def advance(self, N):
        if N < 0:
            return None
        if N == 0:
            return self.value()
        if N == 1:
            return self.next()
        while N > 0:
            self.next()
            N = N - 1
        return self.value()


class linked_hash_map_SynthProvider:

    def __init__(self, valobj, dict):
        self.valobj = valobj
        self.count = self.valobj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()

    def next_node(self, node):
        return node.GetChildMemberWithName('next')

    def value(self, node):
        return node.GetValueAsUnsigned()

    def num_children(self):
        return self.count

    def get_child_index(self, name):
        try:
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        try:
            current = linked_hash_map_iterator(self.head)
            current = current.advance(index)
            obj = current.GetChildMemberWithName('value')
            return self.valobj.CreateValueFromData('[' + str(index) + ']',
                                                   obj.GetData(),
                                                   obj.GetType())
        except:
            return None

    def update(self):
        self.count = self.valobj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        end_node = self.valobj.GetChildMemberWithName('end_')
        self.head = end_node.GetChildMemberWithName('next')

    def has_children(self):
        return True


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        'type synthetic add -x "^lynx::base::LinkedHashMap<.*>$" -l linked_hash_map_lldb.linked_hash_map_SynthProvider -w liblynx'
    )
    debugger.HandleCommand(
        'type summary add -x "^lynx::base::LinkedHashMap<.*>$" -s "size=${var.count_}, pool_size=${var.pool_size_}, pool_cursor=${var.pool_cursor_}, is_perfect=${var.is_perfect_}, map=${var.map_}" -w liblynx'
    )
    debugger.HandleCommand('type category enable liblynx')
