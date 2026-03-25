# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb

# LLDB formatter for Vector container.

def align_up(addr, alignment):
    """Replicates the C++ align_up function."""
    return (addr + alignment - 1) & ~(alignment - 1)

class linear_map_SynthProvider:

    def __init__(self, val_obj, dict):
        self.map_obj = val_obj
        self.update()

    def num_children(self):
        return self.count

    def get_child_at_index(self, index):
        if index < 0:
            return None
        if index >= self.num_children():
            return None
        try:
            offset = index * self.data_size
            child_name = '[' + str(index) + ']'
            if self.consecutive_key:
                extra_hash_value = self.container_obj.CreateValueFromAddress('', self.alloc_start + index * self.extra_bytes_per_element, self.key_type)
                child_name += '[' + extra_hash_value.GetValue() + ']'
            return self.start.CreateChildAtOffset(child_name,
                                                    offset, self.data_type)

        except:
            return None

    def num_capacity(self):
        return self.capacity

    def update(self):
        self.map_type = self.map_obj.GetType().GetDereferencedType()
        if self.map_type.IsPointerType():
            self.map_type = self.map_type.GetPointeeType()
        self.key_type = self.map_type.GetTemplateArgumentType(0)
        self.key_policy_type = self.map_type.GetTemplateArgumentType(2)
        self.consecutive_key = str(self.key_policy_type).find("lynx::base::MapKeyPolicyConsecutiveIntegers") >= 0
        self.container_obj = self.map_obj.GetChildMemberWithName("array_")
        self.container_target = self.container_obj.GetTarget()
        self.container_type = self.container_obj.GetType()
        if self.container_type.GetCanonicalType().GetName().find("InlineVector") < 0:
            # template Vector<class T, size_t ExtraBytesPerElement = 0, bool CountRealloc = false>
            self.extra_bytes_per_element = self.container_type.GetTemplateArgumentValue(self.container_target, 1).GetValueAsUnsigned()
        else:
            # template InlineVector<class T, size_t N, size_t ExtraBytesPerElement = 0, bool CountRealloc = false>
            self.extra_bytes_per_element = self.container_type.GetTemplateArgumentValue(self.container_target, 2).GetValueAsUnsigned()
        self.count = self.container_obj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        self.capacity = self.container_obj.GetChildMemberWithName(
            'capacity_').GetValueAsSigned()
        self.start = self.container_obj.GetChildMemberWithName('memory_')
        self.alloc_start = self.start.GetValueAsAddress() - align_up(self.extra_bytes_per_element * abs(self.capacity), self.container_target.GetAddressByteSize())
        self.data_type = self.start.GetType().GetPointeeType()
        self.data_size = self.data_type.GetByteSize()

    def has_children(self):
        return True

class vector_SynthProvider:

    def __init__(self, val_obj, dict):
        self.val_obj = val_obj
        self.count = self.val_obj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        self.capacity = self.val_obj.GetChildMemberWithName(
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
        self.count = self.val_obj.GetChildMemberWithName(
            'count_').GetValueAsUnsigned()
        self.capacity = self.val_obj.GetChildMemberWithName(
            'capacity_').GetValueAsSigned()
        self.start = self.val_obj.GetChildMemberWithName('memory_')
        self.data_type = self.start.GetType().GetPointeeType()
        self.data_size = self.data_type.GetByteSize()

    def has_children(self):
        return True


def __lldb_init_module(debugger, dict):
    debugger.HandleCommand(
        'type synthetic add -x "^lynx::base::LinearSearchMap<.*>$" -l vector_lldb.linear_map_SynthProvider -w liblynx'
    )
    debugger.HandleCommand(
        'type synthetic add -x "^lynx::base::Vector<.*>$" -l vector_lldb.vector_SynthProvider -w liblynx'
    )
    debugger.HandleCommand(
        'type summary add -x "^lynx::base::Vector<.*>$" -s "size=${var.count_}, capacity=${var.capacity_}" -w liblynx'
    )
    debugger.HandleCommand('type category enable liblynx')
