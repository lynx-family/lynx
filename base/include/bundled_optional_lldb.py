# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import lldb


class BundledOptionalsSyntheticProvider:
    """
    Synthetic children provider for the BundledOptionals class.
    It reconstructs the view of optional members stored in a single memory block.
    """
    def __init__(self, val_obj, internal_dict):
        self.val_obj = val_obj
        self.update()

    def update(self):
        """
        Fetches all necessary information from the debugged process.
        This is called when the provider is first created or when the state changes.
        """
        self.target = self.val_obj.GetTarget()
        self.process = self.target.GetProcess()
        self.ptr_size = self.target.GetAddressByteSize()
        self.UINT8_MAX = 255

        data_ptr_member = self.val_obj.GetChildMemberWithName('bundled_data_')
        if data_ptr_member and data_ptr_member.IsValid():
            self.data_block_addr = data_ptr_member.GetValueAsUnsigned(0)
        else:
            self.data_block_addr = 0 # Indicate failure

        # --- Read the offsets_ array ---
        self.offsets = []
        self.num_fields = self.val_obj.GetType().GetNumberOfTemplateArguments()
        offsets_member = self.val_obj.GetChildMemberWithName('offsets_')
        if offsets_member.IsValid():
            # offsets_ is an array inside a struct, get its first element
            array_start = offsets_member.GetChildAtIndex(0)
            for i in range(self.num_fields):
                offset_val = array_start.GetChildAtIndex(i).GetValueAsUnsigned(0)
                self.offsets.append(offset_val)
        # --- Get template argument types ---
        self.def_types = []
        self.real_types = []
        for i in range(self.num_fields):
            def_type = self.val_obj.GetType().GetTemplateArgumentType(i)
            self.def_types.append(def_type)
            # Find the inner 'Type' from the definition struct
            real_type_field = def_type.FindDirectNestedType('Type')
            self.real_types.append(real_type_field.GetTypedefedType())

    def num_children(self):
        return self.num_fields + 2
    
    def get_child_at_index(self, index):
        if index == 0:
            return self.val_obj.GetChildMemberWithName('offsets_')
        elif index == 1:
            return self.val_obj.GetChildMemberWithName('bundled_data_')

        index -= 2
        if index >= self.num_fields:
            return None
        offset = self.offsets[index]
        def_type = self.def_types[index]
        child_name = f'[{index}] {def_type.GetName()}'
        if offset == self.UINT8_MAX or self.data_block_addr == 0:
            # Member does not exist or data block is invalid
            return self.val_obj.CreateValueFromData(child_name,
                                                   lldb.SBData.CreateDataFromCString(self.process.GetByteOrder(), self.ptr_size, "[not set]"),
                                                   self.target.GetBasicType(lldb.eBasicTypeChar).GetArrayType(9))
        else:
            # Member exists, calculate its address and create a typed value
            real_type = self.real_types[index]
            child_addr = self.data_block_addr + (offset * self.ptr_size)
            return self.val_obj.CreateValueFromAddress(child_name, child_addr, real_type)
        
    def has_children(self):
        return True
    
def __lldb_init_module(debugger, internal_dict):
    class_name = 'lynx::base::BundledOptionals'
    regex = f"^{class_name}<.*>$"
    debugger.HandleCommand(f'type synthetic add --python-class {__name__}.BundledOptionalsSyntheticProvider -x "{regex}" -w liblynx')
    debugger.HandleCommand('type category enable liblynx')