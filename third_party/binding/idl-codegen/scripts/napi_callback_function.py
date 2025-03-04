# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Generate template values for a callback function.

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""

from napi_globals import includes
import napi_types

CALLBACK_FUNCTION_H_INCLUDES = frozenset([
    'third_party/binding/napi/napi_bridge.h',
    "third_party/binding/napi/callback_helper.h",
])
CALLBACK_FUNCTION_CPP_INCLUDES = frozenset([
    # 'base/stl_util.h',
    # 'bindings/core/v8/generated_code_helper.h',
    # 'bindings/core/v8/native_value_traits_impl.h',
    # 'bindings/core/v8/to_v8_for_core.h',
    # 'bindings/core/v8/v8_binding_for_core.h',
    # 'core/execution_context/execution_context.h',
    # 'platform/bindings/exception_messages.h',
    # 'platform/bindings/exception_state.h',
    # 'platform/bindings/script_forbidden_scope.h',
])


def callback_function_context(callback_function):
    includes.clear()
    includes.update(CALLBACK_FUNCTION_CPP_INCLUDES)
    idl_type = callback_function.idl_type
    idl_type_str = str(idl_type)

    for argument in callback_function.arguments:
        argument.idl_type.add_includes_for_type(
            callback_function.extended_attributes)
        
    holder_storage_namespace = ''
    if 'HolderStorageNamespace' in callback_function.extended_attributes:
        holder_storage_namespace = '%s::' % callback_function.extended_attributes.get('HolderStorageNamespace')

    context = {
        # While both |callback_function_name| and |cpp_class| are identical at
        # the moment, the two are being defined because their values may change
        # in the future (e.g. if we support [ImplementedAs=] in callback
        # functions).
        'callback_function_name':
            callback_function.name,
        'cpp_class':
            'Napi%s' % callback_function.name,
        'cpp_includes':
            sorted(includes),
        # 'forward_declarations':
        # sorted(forward_declarations(callback_function)),
        'header_includes':
            sorted(CALLBACK_FUNCTION_H_INCLUDES),
        'idl_type':
            idl_type_str,
        'is_treat_non_object_as_null':
            'LegacyTreatNonObjectAsNull' in callback_function.extended_attributes,
        # 'native_value_traits_tag':
        # napi_types.idl_type_to_native_value_traits_tag(idl_type),
        'return_cpp_type':
            idl_type.cpp_type,
        'enable_interval':
            'EnableInterval' in callback_function.extended_attributes,
        'holder_storage_namespace':
            holder_storage_namespace,
    }

    context.update(arguments_context(callback_function.arguments, context))
    return context


def forward_declarations(callback_function):
    def find_forward_declaration(idl_type):
        if idl_type.is_interface_type or idl_type.is_dictionary:
            return idl_type.implemented_as
        elif idl_type.is_array_or_sequence_type:
            return find_forward_declaration(idl_type.element_type)
        return None

    declarations = set()
    for argument in callback_function.arguments:
        name = find_forward_declaration(argument.idl_type)
        if name:
            declarations.add(name)
    return declarations


def arguments_context(arguments, context):
    def argument_context(argument, index, context):
        idl_type = argument.idl_type
        this_cpp_type = idl_type.cpp_type_args(
            extended_attributes=argument.extended_attributes,
            raw_type=False,
            used_as_rvalue_type=True,
            used_as_variadic_argument=argument.is_variadic)

        impl_as_std_optional = False
        if idl_type.is_string_type:
            this_cpp_type = 'std::string'
        if idl_type.is_wrapper_type:
            this_cpp_type = 'std::unique_ptr<%s>' % (this_cpp_type[:-1])
        elif idl_type.is_nullable or argument.is_optional:
            this_cpp_type = 'std::optional<%s>' % (this_cpp_type)
            impl_as_std_optional = True
            context['header_includes'].insert(0, '<optional>')

        return {
            'cpp_value_to_v8_value':
                idl_type.cpp_value_to_v8_value(
                    argument.name,
                    isolate='GetIsolate()',
                    creation_context='argument_creation_context'),
            'declaration_name':
                '%s arg%s%s' % (this_cpp_type, index, ' = {}' if argument.is_optional else ''),
            'definition_name':
                '%s arg%s%s' % (this_cpp_type, index, '_optional' if impl_as_std_optional else ''),
            'call_name' :
                'arg%s_%s' %(index,argument.name),
            'enum_type':
                idl_type.enum_type,
            'enum_values':
                idl_type.enum_values,
            'is_variadic':
                argument.is_variadic,
            'name':
                argument.name,
            'napi_name':
                'napi_%s' % argument.name,
            'cpp_type':  this_cpp_type,
            # Dictionary is special-cased, but arrays and sequences shouldn't be
            'idl_type':
                idl_type.base_type,
            'idl_type_object':
                idl_type,
            'index':
                index,
            'is_dictionary':
                idl_type.is_dictionary,
            'is_explicit_nullable':
                idl_type.is_explicit_nullable,
            'is_nullable':
                idl_type.is_nullable,
            'is_optional':
                argument.is_optional,
            'is_boolean_type':
                idl_type.base_type == 'boolean',
            'is_numeric_type':
                idl_type.is_numeric_type,
            'is_string_type':
                idl_type.is_string_type,
            'is_typed_array':
                idl_type.is_typed_array,
            'is_wrapper_type':
                idl_type.is_wrapper_type,
        }

    def argument_cpp_type(argument):
        if argument.idl_type.is_dictionary:
            return 'const %s*' % argument.idl_type.implemented_as

        return argument.idl_type.cpp_type_args(
            extended_attributes=argument.extended_attributes,
            raw_type=False,
            used_as_rvalue_type=True,
            used_as_variadic_argument=argument.is_variadic)

    argument_declarations = [
        'bindings::V8ValueOrScriptWrappableAdapter callback_this_value'
    ]
    argument_declarations.extend(
        '%s %s' % (argument_cpp_type(argument), argument.name)
        for argument in arguments)
    return {
        'argument_declarations': argument_declarations,
        'arguments': [argument_context(argument,index,context) for index,argument in enumerate(arguments)],
        'number_of_arguments': len(arguments),
    }

