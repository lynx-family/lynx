# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb

# LLDB formatter for Vector container.


class vector_SynthProvider:

    def __init__(self, valobj, dict):
        self.valobj = valobj
        self.count = self.valobj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        self.capacity = self.valobj.GetChildMemberWithName(
            'capacity_').GetValueAsSigned()

    def num_children(self):
        return self.count

    def get_child_at_index(self, index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        try:
            offset = index * self.data_size
            return self.start.CreateChildAtOffset('[' + str(index) + ']',
                                                  offset, self.data_type)
        except:
            return None

    def num_capacity(self):
        return self.capacity

    def update(self):
        self.count = self.valobj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        self.capacity = self.valobj.GetChildMemberWithName(
            'capacity_').GetValueAsSigned()
        self.start = self.valobj.GetChildMemberWithName('memory_')
        self.data_type = self.start.GetType().GetPointeeType()
        self.data_size = self.data_type.GetByteSize()

    def has_children(self):
        return True


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        'type synthetic add -x "^lynx::base::Vector<.*>$" -l vector_lldb.vector_SynthProvider -w liblynx'
    )
    debugger.HandleCommand(
        'type summary add -x "^lynx::base::Vector<.*>$" -s "size=${var.count_}, capacity=${var.capacity_}" -w liblynx'
    )
    debugger.HandleCommand('type category enable liblynx')
