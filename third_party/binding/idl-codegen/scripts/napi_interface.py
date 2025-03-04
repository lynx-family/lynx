# Copyright (C) 2013 Google Inc. All rights reserved.
# coding=utf-8
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# pylint: disable=relative-import
"""Generate template values for an interface.

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""
import os
import sys
from operator import or_

import copy

sys.path.append(
    os.path.join(os.path.dirname(__file__), 'base'))
from blinkbuild.name_style_converter import NameStyleConverter
from idl_definitions import IdlAttribute, IdlOperation, IdlArgument
from idl_types import IdlType, inherits_interface
from overload_set_algorithm import effective_overload_set_by_length
from overload_set_algorithm import method_overloads_by_name

import napi_attributes
from napi_globals import includes
import napi_methods
import napi_types
import napi_utilities
from napi_utilities import (
    binding_header_filename, context_enabled_feature_name, cpp_name_or_partial,
    cpp_name, has_extended_attribute_value, runtime_enabled_feature_name)

import re

INTERFACE_H_INCLUDES = frozenset([
    'third_party/binding/napi/napi_bridge.h',
    'third_party/binding/napi/native_value_traits.h',
    # 'bindings/core/v8/generated_code_helper.h',
    # 'bindings/core/v8/native_value_traits.h',
    # 'platform/bindings/script_wrappable.h',
    # 'bindings/core/v8/to_v8_for_core.h',
    # 'bindings/core/v8/v8_binding_for_core.h',
    # 'platform/bindings/v8_dom_wrapper.h',
    # 'platform/bindings/wrapper_type_info.h',
    # 'platform/heap/handle.h',
])
INTERFACE_CPP_INCLUDES = frozenset([
    # 'base/memory/scoped_refptr.h',
    # 'bindings/core/v8/native_value_traits_impl.h',
    # 'bindings/core/v8/v8_dom_configuration.h',
    # 'core/execution_context/execution_context.h',
    # 'platform/scheduler/public/cooperative_scheduling_manager.h',
    # 'platform/bindings/exception_messages.h',
    # 'platform/bindings/exception_state.h',
    # 'platform/bindings/v8_object_constructor.h',
    # 'platform/wtf/get_ptr.h',
])


def filter_has_constant_configuration(constants):
    return [
        constant for constant in constants
        if not constant['measure_as'] and not constant['deprecate_as']
        and not constant['runtime_enabled_feature_name']
        and not constant['origin_trial_feature_name']
    ]


def filter_has_special_getter(constants):
    return [
        constant for constant in constants
        if constant['measure_as'] or constant['deprecate_as']
    ]


def filter_runtime_enabled(constants):
    return [
        constant for constant in constants
        if constant['runtime_enabled_feature_name']
    ]


def filter_origin_trial_enabled(constants):
    return [
        constant for constant in constants
        if constant['origin_trial_feature_name']
    ]


def constant_filters():
    return {
        'has_constant_configuration': filter_has_constant_configuration,
        'has_special_getter': filter_has_special_getter,
        'runtime_enabled_constants': filter_runtime_enabled,
        'origin_trial_enabled_constants': filter_origin_trial_enabled
    }


def origin_trial_features(interface, constants, attributes, methods):
    """ Returns a list of the origin trial features used in this interface.

    Each element is a dictionary with keys 'name' and 'needs_instance'.
    'needs_instance' is true if any member associated with the interface needs
    to be installed on every instance of the interface. This list is the union
    of the sets of features used for constants, attributes and methods.
    """
    KEY = 'origin_trial_feature_name'  # pylint: disable=invalid-name

    def member_filter(members):
        return sorted([member for member in members if member.get(KEY)])

    def member_filter_by_name(members, name):
        return [member for member in members if member[KEY] == name]

    # Collect all members visible on this interface with a defined origin trial
    origin_trial_constants = member_filter(constants)
    origin_trial_attributes = member_filter(attributes)
    origin_trial_methods = member_filter([
        method for method in methods
        if v8_methods.method_is_visible(method, interface.is_partial)
        and not v8_methods.custom_registration(method)
    ])

    feature_names = set([
        member[KEY] for member in origin_trial_constants +
        origin_trial_attributes + origin_trial_methods
    ])

    # Construct the list of dictionaries. 'needs_instance' will be true if any
    # member for the feature has 'on_instance' defined as true.
    features = [{
        'name':
        name,
        'constants':
        member_filter_by_name(origin_trial_constants, name),
        'attributes':
        member_filter_by_name(origin_trial_attributes, name),
        'methods':
        member_filter_by_name(origin_trial_methods, name)
    } for name in feature_names]
    for feature in features:
        members = feature['constants'] + feature['attributes'] + feature[
            'methods']
        feature['needs_instance'] = any(
            member.get('on_instance', False) for member in members)
        # TODO(chasej): Need to handle method overloads? e.g.
        # (method['overloads']['secure_context_test_all'] if 'overloads' in method else method['secure_context_test'])
        feature['needs_secure_context'] = any(
            member.get('secure_context_test', False) for member in members)
        feature['needs_context'] = feature['needs_secure_context'] or any(
            member.get('exposed_test', False) for member in members)

    if features:
        includes.add('platform/bindings/script_state.h')
        includes.add('platform/runtime_enabled_features.h')
        includes.add('core/execution_context/execution_context.h')

    return features


def context_enabled_features(attributes):
    """ Returns a list of context-enabled features from a set of attributes.

    Each element is a dictionary with the feature's |name| and lists of
    |attributes| associated with the feature.
    """
    KEY = 'context_enabled_feature_name'  # pylint: disable=invalid-name

    def member_filter(members):
        return sorted([
            member for member in members
            if member.get(KEY) and not member.get('exposed_test')
        ], key=lambda item: item['name'])

    def member_filter_by_name(members, name):
        return [member for member in members if member[KEY] == name]

    # Collect all members visible on this interface with a defined origin trial
    context_enabled_attributes = member_filter(attributes)
    feature_names = set([member[KEY] for member in context_enabled_attributes])
    features = [{
        'name':
        name,
        'attributes':
        member_filter_by_name(context_enabled_attributes, name),
        'needs_instance':
        False
    } for name in feature_names]
    if features:
        includes.add('platform/bindings/script_state.h')
    return features


def runtime_call_stats_context(interface):
    counter_prefix = 'Blink_' + v8_utilities.cpp_name(interface) + '_'
    return {
        'constructor_counter':
        counter_prefix + 'Constructor',
        'cross_origin_named_getter_counter':
        counter_prefix + 'CrossOriginNamedGetter',
        'cross_origin_named_setter_counter':
        counter_prefix + 'CrossOriginNamedSetter',
        'indexed_property_getter_counter':
        counter_prefix + 'IndexedPropertyGetter',
        'named_property_getter_counter':
        counter_prefix + 'NamedPropertyGetter',
        'named_property_query_counter':
        counter_prefix + 'NamedPropertyQuery',
        'named_property_setter_counter':
        counter_prefix + 'NamedPropertySetter',
    }


def interface_context(interface, interfaces, component_info, interfaces_info):
    """Creates a Jinja template context for an interface.

    Args:
        interface: An interface to create the context for
        interfaces: A dict which maps an interface name to the definition
            which can be referred if needed
        component_info: A dict containing component wide information

    Returns:
        A Jinja template context for |interface|
    """

    includes.clear()
    includes.update(INTERFACE_CPP_INCLUDES)
    header_includes = set(INTERFACE_H_INCLUDES)

    if interface.is_partial:
        # A partial interface definition cannot specify that the interface
        # inherits from another interface. Inheritance must be specified on
        # the original interface definition.
        parent_interface = None
        is_event_target = False
        # partial interface needs the definition of its original interface.
        includes.add(
            'bindings/core/v8/%s' % binding_header_filename(interface.name))
        raise ValueError('interface.is_partial')
    else:
        parent_interface = interface.parent
        if parent_interface:
            header_includes.update(
                napi_types.includes_for_interface(parent_interface))
        is_event_target = inherits_interface(interface.name, 'EventTarget')

    extended_attributes = interface.extended_attributes

    napi_class_name = napi_utilities.napi_class_name(interface)
    cpp_class_name = cpp_name(interface)
    cpp_class_name_or_partial = cpp_name_or_partial(interface)
    napi_class_name_or_partial = napi_utilities.napi_class_name_or_partial(interface)

    runtime_features = component_info['runtime_enabled_features']

    context = {
        'cpp_class':
        cpp_class_name,
        'cpp_class_or_partial':
        cpp_class_name_or_partial,
        'header_includes':
        header_includes,
        'interface_name':
        interface.name,
        'internal_namespace':
        internal_namespace(interface),
        'pass_cpp_type':
        cpp_name(interface) + '*',
        'snake_case_napi_class':
        NameStyleConverter(napi_class_name).to_snake_case(),
        'napi_class':
        napi_class_name,
        'napi_class_or_partial':
        napi_class_name_or_partial,
        'parent_interface':
        parent_interface,
        'descendants':
        getattr(interface, 'descendants', []),
        'sharing_impl':
        interface.sharing_impl
    }

    # Constructors
    constructors = [
        constructor_context(interface, constructor)
        for constructor in interface.constructors
        # FIXME: shouldn't put named constructors with constructors
        # (currently needed for Perl compatibility)
        # Handle named constructors separately
        if constructor.name == 'Constructor'
    ]
    if len(constructors) > 1:
        context['constructor_overloads'] = overloads_context(
            interface, constructors)

    context['dispose_on_destruction'] = ('DisposeOnDestruction' in extended_attributes)
    context['buffer_commands'] = ('BufferCommands' in extended_attributes)
    context['enable_trace'] = ('EnableTrace' in extended_attributes)
    context['omit_constants'] = ('OmitConstants' in extended_attributes)
    context['impl_namespace'] = extended_attributes.get('ImplNamespace')
    context['exported'] = ('Exported' in extended_attributes)

    context['log_on_destruction'] = ('LogOnDestruction' in extended_attributes)

    # ToImplUnsafe method needs BASE_EXPORT
    context['export_impl_getter'] = ('ExportImplGetter' in extended_attributes)


    # Constants
    context.update({
        'constants': [
            constant_context(constant, interface, component_info)
            for constant in interface.constants
        ],
        'do_not_check_constants':
        'DoNotCheckConstants' in extended_attributes,
    })

    command_buffer_context['constants'].extend(context['constants'])

    if interface.async_created:
        command_buffer_context['async_classes'].append(interface.name)

    # Attributes
    attributes = attributes_context(interface, interfaces, component_info)

    context.update({
        'constructors':
        constructors,
        'attributes':
        attributes,
        # Elements in attributes are broken in following members.
        # 'accessors':
        # napi_attributes.filter_accessors(attributes),
        # 'data_attributes':
        # napi_attributes.filter_data_attributes(attributes),
        # 'runtime_enabled_attributes':
        # napi_attributes.filter_runtime_enabled(attributes),
    })

    # Methods
    context.update(methods_context(interface, component_info, interfaces_info, interfaces))
    methods = context['methods']

    # context.update({
    #     'indexed_property_getter':
    #     property_getter(interface.indexed_property_getter, ['index']),
    #     'indexed_property_setter':
    #     property_setter(interface.indexed_property_setter, interface),
    #     'indexed_property_deleter':
    #     property_deleter(interface.indexed_property_deleter),
    #     'named_property_getter':
    #     property_getter(interface.named_property_getter, ['name']),
    #     'named_property_setter':
    #     property_setter(interface.named_property_setter, interface),
    #     'named_property_deleter':
    #     property_deleter(interface.named_property_deleter),
    # })

    return context


def attributes_context(interface, interfaces, component_info):
    """Creates a list of Jinja template contexts for attributes of an interface.

    Args:
        interface: An interface to create contexts for
        interfaces: A dict which maps an interface name to the definition
            which can be referred if needed

    Returns:
        A list of attribute contexts
    """

    attributes = [
        napi_attributes.attribute_context(interface, attribute, interfaces,
                                        component_info)
        for attribute in interface.attributes
    ]

    has_conditional_attributes = any(
        attribute['exposed_test'] for attribute in attributes)
    if has_conditional_attributes and interface.is_partial:
        raise Exception(
            'Conditional attributes between partial interfaces in modules '
            'and the original interfaces(%s) in core are not allowed.' %
            interface.name)

    return attributes


k_command_buffer_init = {
    'method_index': 1,
    'remote_method_index': 1,
    'remote_type_id': 3001,
    'methods': [],
    'remote_methods': [],
    'constants': [],
    'interfaces': [],
    'async_classes': [],
    'slow_if_buffered_methods': [],
    'buffering_disabled_methods': [],
    'flushed_within_frame_methods': [],
}
command_buffer_context = copy.deepcopy(k_command_buffer_init)


def methods_context(interface, component_info, interfaces_info, interfaces):
    """Creates a list of Jinja template contexts for methods of an interface.

    Args:
        interface: An interface to create contexts for
        component_info: A dict containing component wide information

    Returns:
        A dictionary with 3 keys:
        'iterator_method': An iterator context if available or None.
        'iterator_method_alias': A string that can also be used to refer to the
                                 @@iterator symbol or None.
        'methods': A list of method contexts.
    """

    methods = []

    methods.extend([
        napi_methods.method_context(interface, method, component_info, interfaces_info)
        for method in interface.operations if method.name
    ])  # Skip anonymous special operations (methods)

    compute_method_overloads_context(interface, methods)

    for method in methods:
        # The value of the Function object’s “length” property is a Number
        # determined as follows:
        # 1. Let S be the effective overload set for regular operations (if the
        # operation is a regular operation) or for static operations (if the
        # operation is a static operation) with identifier id on interface I and
        # with argument count 0.
        # 2. Return the length of the shortest argument list of the entries in S.
        # FIXME: This calculation doesn't take into account whether runtime
        # enabled overloads are actually enabled, so length may be incorrect.
        # E.g., [RuntimeEnabled=Foo] void f(); void f(long x);
        # should have length 1 if Foo is not enabled, but length 0 if it is.
        method['length'] = (method['overloads']['length']
                            if 'overloads' in method else
                            method['number_of_required_arguments'])

    global command_buffer_context
    command_buffer_class_name = 'Napi' + NameStyleConverter(command_buffer_context['component']).to_upper_camel_case() + 'CommandBuffer'
    overloads_child_only = {}
    buffer_commands = 'BufferCommands' in interface.extended_attributes
    buffer_commands_for_remote = command_buffer_context['generates_remote'] and not 'DataContainer' in interface.extended_attributes
    if buffer_commands_for_remote:
        for regex in command_buffer_context['remote_excludes']:
            if re.search(regex, interface.name):
                buffer_commands_for_remote = False
                break

    deferred_setters = buffer_commands_for_remote and 'DeferredSetters' in interface.extended_attributes
    remote_only = buffer_commands_for_remote and not buffer_commands
    if buffer_commands or buffer_commands_for_remote:
        ptr_name = NameStyleConverter(interface.name).to_snake_case()

        # Account for remote constructors/destructors.
        remote_destructor_id = 0
        constructors = []
        if buffer_commands_for_remote and not interface.shared_impl:
            constructors = [
                constructor_context(interface, constructor)
                for constructor in interface.constructors
                if constructor.name == 'Constructor'
            ]
            overload_index = 1
            for constructor in constructors:
                constructor.update({'name': 'constructor', 'from_shared': False, 'camel_case_name': 'Constructor'})
                if len(constructors) > 1:
                    constructor.update({'overload_index': overload_index})
                    overload_index += 1
                add_buffered_method(interface, constructor, ptr_name, command_buffer_context, overloads_child_only, True)
            remote_destructor_id = command_buffer_context['remote_method_index']
            add_buffered_method(interface, {'name': 'destructor', 'from_shared': False}, ptr_name, command_buffer_context, overloads_child_only, True)

        # Convert remote attributes to setter/getters.
        attributes = [
            napi_attributes.attribute_context(interface, attribute, interfaces,
                                            component_info)
            for attribute in interface.attributes
        ]
        for attribute in attributes:
            if attribute['does_generate_setter']:
                attribute_copy = copy.deepcopy(attribute)
                attribute_copy.update({'is_setter': True, 'camel_case_name': 'Set' + attribute_copy['camel_case_name'], 'arguments': [attribute], 'idl_type': 'void', 'is_async': deferred_setters})
                add_buffered_method(interface, attribute_copy, ptr_name, command_buffer_context, overloads_child_only, True)
            attribute.update({'is_getter': True, 'camel_case_name': 'Get' + attribute['camel_case_name']})
            add_buffered_method(interface, attribute, ptr_name, command_buffer_context, overloads_child_only, True)

        for method in methods:
            method_is_remote_only = remote_only
            if buffer_commands:
                for argument in method['arguments']:
                    # For now sequence elements can only be numbers or strings.
                    if argument['sequence_element_type'] and argument['sequence_element_type'] != 'Number' and argument['sequence_element_type'] != 'String':
                        raise Exception("For now sequence elements can only be numbers or strings")

                    if argument['is_string_type'] or argument['sequence_element_type'] == 'String':
                        method['slow_if_buffered'] = True
                        break

                    if argument['idl_type'] == 'ArrayBufferView' or argument['idl_type'] == 'ArrayBuffer' or argument['is_typed_array']:
                        method['keep_binding_version'] = True

                if method['name'] in command_buffer_context['slow_if_buffered_methods']:
                    method['slow_if_buffered'] = True

                if method.get('slow_if_buffered', False):
                    method['keep_binding_version'] = True

                if method['name'] in command_buffer_context['buffering_disabled_methods']:
                    method['skip_buffering'] = True
                    method['keep_binding_version'] = False

                # TODO(yuyifei): Maybe remove the last part to migrate remaining binding methods.
                method['do_not_buffer_for_single_thread'] = method.get('skip_buffering', False) or (method['idl_type'] != 'void' and not method['handles_async'])
                if method.get('do_not_buffer_for_single_thread', False):
                    method['keep_binding_version'] = False
                method_is_remote_only = method_is_remote_only or method['do_not_buffer_for_single_thread']

            add_buffered_method(interface, method, ptr_name, command_buffer_context, overloads_child_only, method_is_remote_only)

        command_buffer_context['interfaces'].append({
            'type_name': interface.name,
            'parent': interface.parent,
            'shared_impl': interface.shared_impl,
            'ptr_name': ptr_name,
            'overloads_child_only': overloads_child_only,
            'remote_only': remote_only,
            'remote_type_id': command_buffer_context['remote_type_id'],
            'remote_constructor_num': len(constructors),
            'remote_destructor_id': remote_destructor_id,
        })
        command_buffer_context['remote_type_id'] += 1

    for method in methods:
        if method.get('overloads', False):
              for length, tests_methods in method['overloads']['length_tests_methods']:
                  for test, overload in tests_methods:
                      if overload.get('keep_binding_version', False):
                          method['keep_binding_resolver'] = True
                      if overload.get('slow_if_buffered', False):
                          method['slow_if_buffered'] = True

        # 'slow_if_buffered' are those methods that a buffered version is generated, but only used in tests:
        # 1. Methods that may have time consuming argument serialization: strings.
        # 2. Temporary workarounds.
        # 'keep_binding_version' indicates if we should keep a binding impl for this method in single-thread mode, which includes:
        # 1. 'slow_if_buffered' methods.
        # 2. Methods that may overflow the command buffer: strings/typedarray/arraybuffer/arraybufferview.
        # 'keep_binding_resolver' stresses that the flag is valid for the overload resolver, clarifying the method itself may not need
        # binding, since the resolver is the last overload which is randomly chosen.
        # Only buffered methods have 'keep_binding_version' and 'slow_if_buffered',
        # but all methods have 'has_binding_version'.
        # The above flags are only valid for single-thread mode.
        method['has_binding_version'] = not buffer_commands or method.get('keep_binding_version', False) or method['do_not_buffer_for_single_thread']
        method['has_binding_resolver'] = method.get('overloads', False) and (not buffer_commands or method.get('keep_binding_resolver', False) or method['do_not_buffer_for_single_thread'])

    return {
        'methods': methods,
        'command_buffer_class': command_buffer_class_name,
    }

def add_buffered_method(interface, method, ptr_name, command_buffer_context, overloads_child_only, remote_only):
    # Keep the base impl only
    if interface.sharing_impl and method['from_shared']:
        return
    if not remote_only:
        method['buffered_method_id'] = command_buffer_context['method_index']
        command_buffer_context['method_index'] += 1
    method['remote_method_id'] = command_buffer_context['remote_method_index']
    command_buffer_context['remote_method_index'] += 1

    method['ptr_name'] = ptr_name
    method['class_name'] = NameStyleConverter(interface.name).to_upper_camel_case()
    # |is_sync| determines the method is actually called sync/async (Call/Flush).
    if not remote_only:
        method['is_sync'] = method['idl_type'] != 'void' and not method['return_async'] or method['forced_sync']
        command_buffer_context['methods'].append(method)
    command_buffer_context['remote_methods'].append(method)
    # Record methods that only are overloaded in children, i.e. overload 2 is in children, but overload 1 not.
    if not method.get('from_shared', False) and method.get('overload_index', 0) == 1:
        overloads_child_only[method['name']] = False
    if not method.get('from_shared', False) and method.get('overload_index', 0) == 2 and not method['name'] in overloads_child_only:
        overloads_child_only[method['name']] = True

def init_command_buffer_context(component, metadata):
    global command_buffer_context
    command_buffer_context = copy.deepcopy(k_command_buffer_init)
    command_buffer_context['component'] = component
    command_buffer_context['generates_remote'] = metadata.get('has_remote', False)
    command_buffer_context['remote_excludes'] = metadata.get('remote_excluding_regexps', [])
    command_buffer_context['slow_if_buffered_methods'] = metadata.get('slow_if_buffered_methods', [])
    command_buffer_context['buffering_disabled_methods'] = metadata.get('buffering_disabled_methods', [])
    command_buffer_context['flushed_within_frame_methods'] = metadata.get('flushed_within_frame_methods', [])


def finish_command_buffer():
    global command_buffer_context
    if not command_buffer_context['methods'] and not command_buffer_context['remote_methods'] or not command_buffer_context['component']:
        return
    del command_buffer_context['method_index']
    del command_buffer_context['remote_method_index']
    return command_buffer_context


def reflected_name(constant_name):
    """Returns the name to use for the matching constant name in blink code.

    Given an all-uppercase 'CONSTANT_NAME', returns a camel-case
    'kConstantName'.
    """
    # Check for SHOUTY_CASE constants
    if constant_name.upper() != constant_name:
        return constant_name
    return 'k' + ''.join(part.title() for part in constant_name.split('_'))


# [DeprecateAs], [Reflect], [RuntimeEnabled]
def constant_context(constant, interface, component_info):
    extended_attributes = constant.extended_attributes
    runtime_features = component_info['runtime_enabled_features']

    return {
        'class_name':
        interface.name,
        'camel_case_name':
        NameStyleConverter(constant.name).to_upper_camel_case(),
        'cpp_class':
        extended_attributes.get('PartialInterfaceImplementedAs'),
        'cpp_type':
        constant.idl_type.cpp_type,
        # 'deprecate_as':
        # v8_utilities.deprecate_as(constant),  # [DeprecateAs]
        'idl_type':
        constant.idl_type.name,
        # 'measure_as':
        # v8_utilities.measure_as(constant, interface),  # [MeasureAs]
        # 'high_entropy':
        # v8_utilities.high_entropy(constant),  # [HighEntropy]
        'name':
        constant.name,
        # # [RuntimeEnabled] for origin trial
        # 'origin_trial_feature_name':
        # v8_utilities.origin_trial_feature_name(constant, runtime_features),
        # FIXME: use 'reflected_name' as correct 'name'
        # 'rcs_counter':
        # 'Blink_' + v8_utilities.cpp_name(interface) + '_' + constant.name +
        # '_ConstantGetter',
        # 'reflected_name':
        # extended_attributes.get('Reflect', reflected_name(constant.name)),
        # # [RuntimeEnabled] if not in origin trial
        # 'runtime_enabled_feature_name':
        # runtime_enabled_feature_name(constant, runtime_features),
        'value':
        constant.value,
    }


################################################################################
# Overloads
################################################################################


def compute_method_overloads_context(interface, methods):
    # Regular methods
    compute_method_overloads_context_by_type(
        interface, [method for method in methods if not method['is_static']])
    # Static methods
    compute_method_overloads_context_by_type(
        interface, [method for method in methods if method['is_static']])


def compute_method_overloads_context_by_type(interface, methods):
    """Computes |method.overload*| template values.

    Called separately for static and non-static (regular) methods,
    as these are overloaded separately.
    Modifies |method| in place for |method| in |methods|.
    Doesn't change the |methods| list itself (only the values, i.e. individual
    methods), so ok to treat these separately.
    """
    # Add overload information only to overloaded methods, so template code can
    # easily verify if a function is overloaded
    for name, overloads in method_overloads_by_name(methods):
        # Resolution function is generated after last overloaded function;
        # package necessary information into |method.overloads| for that method.
        overloads[-1]['overloads'] = overloads_context(interface, overloads)
        overloads[-1]['overloads']['name'] = name
        overloads[-1]['overloads']['camel_case_name'] = NameStyleConverter(
            name).to_upper_camel_case()


def overloads_context(interface, overloads):
    """Returns |overloads| template values for a single name.

    Sets |method.overload_index| in place for |method| in |overloads|
    and returns dict of overall overload template values.
    """
    assert len(overloads) > 1  # only apply to overloaded names
    # Make sure inherited methods have smaller indexes.
    index = 1
    last_method = None
    for method in filter(lambda method: 'from_shared' in method and method['from_shared'], overloads):
        method['overload_index'] = index
        last_method = method
        index += 1
    # If there is only 1 inherited overload, need to keep the original function name (without 'Overload1').
    if index == 2:
        last_method['only_one_inherited'] = True
    for method in filter(lambda method: 'from_shared' not in method or not method['from_shared'], overloads):
        method['overload_index'] = index
        index += 1

    # [RuntimeEnabled]
    if any(method.get('origin_trial_feature_name') for method in overloads):
        raise Exception(
            '[RuntimeEnabled] for origin trial cannot be specified on '
            'overloaded methods: %s.%s' % (interface.name,
                                           overloads[0]['name']))

    effective_overloads_by_length = effective_overload_set_by_length(overloads)
    lengths = [length for length, _ in effective_overloads_by_length]
    name = overloads[0].get('name', '<constructor>')
    camel_case_name = NameStyleConverter(name).to_upper_camel_case()

    runtime_determined_lengths = None
    function_length = lengths[0]
    runtime_determined_maxargs = None
    maxarg = lengths[-1]

    # The special case handling below is not needed if all overloads are
    # runtime enabled by the same feature.
    if not common_value(overloads, 'runtime_enabled_feature_name'):
        # Check if all overloads with the shortest acceptable arguments list are
        # runtime enabled, in which case we need to have a runtime determined
        # Function.length.
        shortest_overloads = effective_overloads_by_length[0][1]
        if (all(
                method.get('runtime_enabled_feature_name')
                for method, _, _ in shortest_overloads)):
            # Generate a list of (length, runtime_enabled_feature_names) tuples.
            runtime_determined_lengths = []
            for length, effective_overloads in effective_overloads_by_length:
                runtime_enabled_feature_names = set(
                    method['runtime_enabled_feature_name']
                    for method, _, _ in effective_overloads)
                if None in runtime_enabled_feature_names:
                    # This "length" is unconditionally enabled, so stop here.
                    runtime_determined_lengths.append((length, [None]))
                    break
                runtime_determined_lengths.append(
                    (length, sorted(runtime_enabled_feature_names)))
            function_length = ('%s::%sMethodLength()' % (
                internal_namespace(interface), camel_case_name))

        # Check if all overloads with the longest required arguments list are
        # runtime enabled, in which case we need to have a runtime determined
        # maximum distinguishing argument index.
        longest_overloads = effective_overloads_by_length[-1][1]
        if (not common_value(overloads, 'runtime_enabled_feature_name')
                and all(
                    method.get('runtime_enabled_feature_name')
                    for method, _, _ in longest_overloads)):
            # Generate a list of (length, runtime_enabled_feature_name) tuples.
            runtime_determined_maxargs = []
            for length, effective_overloads in reversed(
                    effective_overloads_by_length):
                runtime_enabled_feature_names = set(
                    method['runtime_enabled_feature_name']
                    for method, _, _ in effective_overloads
                    if method.get('runtime_enabled_feature_name'))
                if not runtime_enabled_feature_names:
                    # This "length" is unconditionally enabled, so stop here.
                    runtime_determined_maxargs.append((length, [None]))
                    break
                runtime_determined_maxargs.append(
                    (length, sorted(runtime_enabled_feature_names)))
            maxarg = ('%s::%sMethodMaxArg()' % (internal_namespace(interface),
                                                camel_case_name))

    # Check and fail if overloads disagree about whether the return type
    # is a Promise or not.
    promise_overload_count = sum(
        1 for method in overloads if method.get('returns_promise'))
    if promise_overload_count not in (0, len(overloads)):
        raise ValueError(
            'Overloads of %s have conflicting Promise/non-Promise types' %
            (name))

    has_overload_visible = False
    has_overload_not_visible = False
    for overload in overloads:
        if overload.get('visible', True):
            # If there exists an overload which is visible, need to generate
            # overload_resolution, i.e. overlods_visible should be True.
            has_overload_visible = True
        else:
            has_overload_not_visible = True

    # If some overloads are not visible and others are visible,
    # the method is overloaded between core and modules.
    has_partial_overloads = has_overload_visible and has_overload_not_visible

    return {
        'deprecate_all_as':
        common_value(overloads, 'deprecate_as'),  # [DeprecateAs]
        'exposed_test_all':
        common_value(overloads, 'exposed_test'),  # [Exposed]
        'length':
        function_length,
        'length_tests_methods':
        length_tests_methods(effective_overloads_by_length),
        # 1. Let maxarg be the length of the longest type list of the
        # entries in S.
        'maxarg':
        maxarg,
        'measure_all_as':
        common_value(overloads, 'measure_as'),  # [MeasureAs]
        'returns_promise_all':
        promise_overload_count > 0,
        'runtime_determined_lengths':
        runtime_determined_lengths,
        'runtime_determined_maxargs':
        runtime_determined_maxargs,
        # [RuntimeEnabled]
        'runtime_enabled_all':
        common_value(overloads, 'runtime_enabled_feature_name'),
        # [SecureContext]
        'secure_context_test_all':
        common_value(overloads, 'secure_context_test'),
        'valid_arities': (
            lengths
            # Only need to report valid arities if there is a gap in the
            # sequence of possible lengths, otherwise invalid length means
            # "not enough arguments".
            if lengths[-1] - lengths[0] != len(lengths) - 1 else None),
        'visible':
        has_overload_visible,
        'has_partial_overloads':
        has_partial_overloads,
    }


def distinguishing_argument_index(entries):
    """Returns the distinguishing argument index for a sequence of entries.

    Entries are elements of the effective overload set with the same number
    of arguments (formally, same type list length), each a 3-tuple of the form
    (callable, type list, optionality list).

    Spec: http://heycam.github.io/webidl/#dfn-distinguishing-argument-index

    If there is more than one entry in an effective overload set that has a
    given type list length, then for those entries there must be an index i
    such that for each pair of entries the types at index i are
    distinguishable.
    The lowest such index is termed the distinguishing argument index for the
    entries of the effective overload set with the given type list length.
    """
    # Only applicable “If there is more than one entry”
    assert len(entries) > 1

    def typename_without_nullable(idl_type):
        if idl_type.is_nullable:
            return idl_type.inner_type.name
        return idl_type.name

    type_lists = [
        tuple(typename_without_nullable(idl_type) for idl_type in entry[1])
        for entry in entries
    ]
    type_list_length = len(type_lists[0])
    # Only applicable for entries that “[have] a given type list length”
    assert all(len(type_list) == type_list_length for type_list in type_lists)
    name = entries[0][0].get('name', 'Constructor')  # for error reporting

    # The spec defines the distinguishing argument index by conditions it must
    # satisfy, but does not give an algorithm.
    #
    # We compute the distinguishing argument index by first computing the
    # minimum index where not all types are the same, and then checking that
    # all types in this position are distinguishable (and the optionality lists
    # up to this point are identical), since "minimum index where not all types
    # are the same" is a *necessary* condition, and more direct to check than
    # distinguishability.
    types_by_index = (set(types) for types in zip(*type_lists))
    try:
        # “In addition, for each index j, where j is less than the
        #  distinguishing argument index for a given type list length, the types
        #  at index j in all of the entries’ type lists must be the same”
        index = next(
            i for i, types in enumerate(types_by_index) if len(types) > 1)
    except StopIteration:
        raise ValueError('No distinguishing index found for %s, length %s:\n'
                         'All entries have the same type list:\n'
                         '%s' % (name, type_list_length, type_lists[0]))
    # Check optionality
    # “and the booleans in the corresponding list indicating argument
    #  optionality must be the same.”
    # FIXME: spec typo: optionality value is no longer a boolean
    # https://www.w3.org/Bugs/Public/show_bug.cgi?id=25628
    initial_optionality_lists = set(entry[2][:index] for entry in entries)
    if len(initial_optionality_lists) > 1:
        raise ValueError(
            'Invalid optionality lists for %s, length %s:\n'
            'Optionality lists differ below distinguishing argument index %s:\n'
            '%s' % (name, type_list_length, index,
                    set(initial_optionality_lists)))

    # Check distinguishability
    # http://heycam.github.io/webidl/#dfn-distinguishable
    # Use names to check for distinct types, since objects are distinct
    # FIXME: check distinguishability more precisely, for validation
    distinguishing_argument_type_names = [
        type_list[index] for type_list in type_lists
    ]
    if (len(set(distinguishing_argument_type_names)) !=
            len(distinguishing_argument_type_names)):
        raise ValueError('Types in distinguishing argument are not distinct:\n'
                         '%s' % distinguishing_argument_type_names)

    return index


def length_tests_methods(effective_overloads_by_length):
    """Returns sorted list of resolution tests and associated methods, by length.

    This builds the main data structure for the overload resolution loop.
    For a given argument length, bindings test argument at distinguishing
    argument index, in order given by spec: if it is compatible with
    (optionality or) type required by an overloaded method, resolve to that
    method.

    Returns:
        [(length, [(test, method)])]
    """
    return [(length, list(resolution_tests_methods(effective_overloads)))
            for length, effective_overloads in effective_overloads_by_length]


def resolution_tests_methods(effective_overloads):
    """Yields resolution test and associated method, in resolution order, for effective overloads of a given length.

    This is the heart of the resolution algorithm.
    https://heycam.github.io/webidl/#dfn-overload-resolution-algorithm

    Note that a given method can be listed multiple times, with different tests!
    This is to handle implicit type conversion.

    Returns:
        [(test, method)]
    """
    methods = [
        effective_overload[0] for effective_overload in effective_overloads
    ]
    if len(methods) == 1:
        # If only one method with a given length, no test needed
        yield 'true', methods[0]
        return

    # 8. If there is more than one entry in S, then set d to be the
    # distinguishing argument index for the entries of S.
    index = distinguishing_argument_index(effective_overloads)

    # (11. is for handling |undefined| values for optional arguments
    #  before the distinguishing argument (as “missing”).)
    # TODO(peria): We have to handle this step. Also in 15.4.2.

    # 12. If i = d, then:
    # 12.1. Let V be args[i].
    cpp_value = 'info[%s]' % index

    # Extract argument and IDL type to simplify accessing these in each loop.
    arguments = [method['arguments'][index] for method in methods]
    arguments_methods = list(zip(arguments, methods))
    idl_types = [argument['idl_type_object'] for argument in arguments]
    idl_types_methods = list(zip(idl_types, methods))

    # We can’t do a single loop through all methods or simply sort them, because
    # a method may be listed in multiple steps of the resolution algorithm, and
    # which test to apply differs depending on the step.
    #
    # Instead, we need to go through all methods at each step, either finding
    # first match (if only one test is allowed) or filtering to matches (if
    # multiple tests are allowed), and generating an appropriate tests.
    #
    # In listing types, we put ellipsis (...) for shorthand nullable type(s),
    # annotated type(s), and a (nullable/annotated) union type, which extend
    # listed types.
    # TODO(peria): Support handling general union types. https://crbug.com/838787

    # 12.2. If V is undefined, and there is an entry in S whose list of
    # optionality values has “optional” at index i, then remove from S all
    # other entries.
    try:
        method = next(method for argument, method in arguments_methods
                      if argument['is_optional'])
        test = '%s.IsUndefined()' % cpp_value
        yield test, method
    except StopIteration:
        pass

    # 12.3. Otherwise: if V is null or undefined, and there is an entry in S that
    # has one of the following types at position i of its type list,
    # • a nullable type
    # • a dictionary type
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.is_nullable or idl_type.is_dictionary)
        test = '%s.IsUndefined() || %s.IsNull()' % (cpp_value, cpp_value)
        yield test, method
    except StopIteration:
        pass

    # 12.4. Otherwise: if V is a platform object, and there is an entry in S that
    # has one of the following types at position i of its type list,
    # • an interface type that V implements
    # ...
    for idl_type, method in idl_types_methods:
        if idl_type.is_wrapper_type and not idl_type.is_array_buffer_or_view:
            test = 'binding::SafeUnwrap<Napi{idl_type}>({cpp_value})'.format(
                idl_type=idl_type.base_type, cpp_value=cpp_value)
            yield test, method

    # 12.5. Otherwise: if V is a DOMException platform object and there is an entry
    # in S that has one of the following types at position i of its type list,
    # • DOMException
    # • Error
    # ...
    # (DOMException is handled in 12.4, and we don't support Error type.)

    # 12.6. Otherwise: if Type(V) is Object, V has an [[ErrorData]] internal slot,
    # and there is an entry in S that has one of the following types at position
    # i of its type list,
    # • Error
    # ...
    # (We don't support Error type.)

    # 12.7. Otherwise: if Type(V) is Object, V has an [[ArrayBufferData]] internal
    # slot, and there is an entry in S that has one of the following types at
    # position i of its type list,
    # • ArrayBuffer
    # ...
    for idl_type, method in idl_types_methods:
        if idl_type.base_type == 'ArrayBufferView':
            test = '{cpp_value}.IsTypedArray() || {cpp_value}.IsDataView()'.format(
                idl_type=idl_type.base_type, cpp_value=cpp_value)
            yield test, method
        elif idl_type.is_array_buffer_or_view or idl_type.is_typed_array:
            test = '{cpp_value}.Is{idl_type}()'.format(
                idl_type=idl_type.base_type, cpp_value=cpp_value)
            yield test, method

    # 12.8. Otherwise: if Type(V) is Object, V has a [[DataView]] internal slot,
    # and there is an entry in S that has one of the following types at position
    # i of its type list,
    # • DataView
    # ...
    # (DataView is included in 12.7.)

    # 12.9. Otherwise: if Type(V) is Object, V has a [[TypedArrayName]] internal
    # slot, and there is an entry in S that has one of the following types at
    # position i of its type list,
    # • a typed array type whose name is equal to the value of V’s
    #   [[TypedArrayName]] internal slot
    # ...
    # (TypedArrays are included in 12.7.)

    # 12.10. Otherwise: if IsCallable(V) is true, and there is an entry in S that
    # has one of the following types at position i of its type list,
    # • a callback function type
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.is_callback_function)
        test = '%s.IsFunction()' % cpp_value
        yield test, method
    except StopIteration:
        pass

    # 12.11. Otherwise: if Type(V) is Object and there is an entry in S that has
    # one of the following types at position i of its type list,
    # • a sequence type
    # • a frozen array type
    # ...
    # and after performing the following steps,
    # 12.11.1. Let method be ? GetMethod(V, @@iterator).
    # method is not undefined, then remove from S all other entries.
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.native_array_element_type)
        # Either condition should be fulfilled to call this |method|.
        test = '%s.IsArray()' % cpp_value
        yield test, method
        # test = 'HasCallableIteratorSymbol(info.GetIsolate(), %s, exception_state)' % cpp_value
        # yield test, method
    except StopIteration:
        pass

    # 12.12. Otherwise: if Type(V) is Object and there is an entry in S that has
    # one of the following types at position i of its type list,
    # • a callback interface type
    # • a dictionary type
    # • a record type
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.is_callback_interface
                      or idl_type.is_dictionary or idl_type.is_record_type)
        test = '%s.IsObject()' % cpp_value
        yield test, method
    except StopIteration:
        pass

    # 12.13. Otherwise: if Type(V) is Boolean and there is an entry in S that has
    # one of the following types at position i of its type list,
    # • boolean
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.name == 'Boolean')
        test = '%s.IsBoolean()' % cpp_value
        yield test, method
    except StopIteration:
        pass

    # 12.14. Otherwise: if Type(V) is Number and there is an entry in S that has
    # one of the following types at position i of its type list,
    # • a numeric type
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.is_numeric_type)
        test = '%s.IsNumber()' % cpp_value
        yield test, method
    except StopIteration:
        pass

    # 12.15. Otherwise: if there is an entry in S that has one of the following
    # types at position i of its type list,
    # • a string type
    # ...
    try:
        method = next(
            method for idl_type, method in idl_types_methods
            if idl_type.is_string_type or idl_type.is_enum or (
                idl_type.is_union_type and idl_type.string_member_type))
        yield 'true', method
    except StopIteration:
        pass

    # 12.16. Otherwise: if there is an entry in S that has one of the following
    # types at position i of its type list,
    # • a numeric type
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.is_numeric_type)
        yield 'true', method
    except StopIteration:
        pass

    # 12.17. Otherwise: if there is an entry in S that has one of the following
    # types at position i of its type list,
    # • boolean
    # ...
    try:
        method = next(method for idl_type, method in idl_types_methods
                      if idl_type.name == 'Boolean')
        yield 'true', method
    except StopIteration:
        pass


################################################################################
# Utility functions
################################################################################


def common(dicts, f):
    """Returns common result of f across an iterable of dicts, or None.

    Call f for each dict and return its result if the same across all dicts.
    """
    values = (f(d) for d in dicts)
    first_value = next(values)
    if all(value == first_value for value in values):
        return first_value
    return None


def common_key(dicts, key):
    """Returns common presence of a key across an iterable of dicts, or None.

    True if all dicts have the key, False if none of the dicts have the key,
    and None if some but not all dicts have the key.
    """
    return common(dicts, lambda d: key in d)


def common_value(dicts, key):
    """Returns common value of a key across an iterable of dicts, or None.

    Auxiliary function for overloads, so can consolidate an extended attribute
    that appears with the same value on all items in an overload set.
    """
    return common(dicts, lambda d: d.get(key))


def internal_namespace(interface):
    return (napi_utilities.to_snake_case(cpp_name_or_partial(interface)) +
            '_napi_internal')


################################################################################
# Constructors
################################################################################


# [Constructor]
def constructor_context(interface, constructor):
    # [RaisesException=Constructor]
    is_constructor_raises_exception = \
        interface.extended_attributes.get('RaisesException') == 'Constructor'
    if constructor.is_raises_exception :
        includes.add(
            'third_party/binding/napi/exception_state.h'
        )

    argument_contexts = [
        napi_methods.argument_context(interface, constructor, argument, index)
        for index, argument in enumerate(constructor.arguments)
    ]

    return {
        'arguments':
        argument_contexts,
        'cpp_type':
        cpp_name(interface) + '*',
        'cpp_value':
        napi_methods.cpp_value(interface, constructor,
                             len(constructor.arguments)),
        'has_exception_state':
        is_constructor_raises_exception
        or any(argument for argument in constructor.arguments
               if argument.idl_type.v8_conversion_needs_exception_state),
        'has_optional_argument_without_default_value':
        any(True for argument_context in argument_contexts
            if argument_context['is_optional_without_default_value']),
        'is_constructor':
        True,
        'is_named_constructor':
        False,
        'is_raises_exception':
        constructor.is_raises_exception,
        'number_of_required_arguments':
        number_of_required_arguments(constructor),
        'rcs_counter':
        'Blink_' + napi_utilities.cpp_name(interface) + '_ConstructorCallback'
    }


# [NamedConstructor]
def named_constructor_context(interface):
    extended_attributes = interface.extended_attributes
    if 'NamedConstructor' not in extended_attributes:
        return None
    # FIXME: parser should return named constructor separately;
    # included in constructors (and only name stored in extended attribute)
    # for Perl compatibility
    idl_constructor = interface.constructors[-1]
    assert idl_constructor.name == 'NamedConstructor'
    context = constructor_context(interface, idl_constructor)
    context.update({
        'name': extended_attributes['NamedConstructor'],
        'is_named_constructor': True,
    })
    return context


def number_of_required_arguments(constructor):
    return len([
        argument for argument in constructor.arguments
        if not (argument.is_optional or argument.is_variadic)
    ])


def interface_length(constructors):
    # Docs: http://heycam.github.io/webidl/#es-interface-call
    if not constructors:
        return 0
    return min(constructor['number_of_required_arguments']
               for constructor in constructors)


################################################################################
# Special operations (methods)
# http://heycam.github.io/webidl/#idl-special-operations
################################################################################


def property_getter(getter, cpp_arguments):
    if not getter:
        return None

    def is_null_expression(idl_type):
        if idl_type.use_output_parameter_for_result or idl_type.is_string_type:
            return 'result.IsNull()'
        if idl_type.is_interface_type:
            return '!result'
        if idl_type.base_type in ('any', 'object'):
            return 'result.IsEmpty()'
        return ''

    extended_attributes = getter.extended_attributes
    has_no_side_effect = v8_utilities.has_extended_attribute_value(
        getter, 'Affects', 'Nothing')
    idl_type = getter.idl_type
    idl_type.add_includes_for_type(extended_attributes)
    is_call_with_script_state = v8_utilities.has_extended_attribute_value(
        getter, 'CallWith', 'ScriptState')
    is_raises_exception = 'RaisesException' in extended_attributes
    use_output_parameter_for_result = idl_type.use_output_parameter_for_result

    # FIXME: make more generic, so can use v8_methods.cpp_value
    cpp_method_name = 'impl->%s' % cpp_name(getter)

    if is_call_with_script_state:
        cpp_arguments.insert(0, 'script_state')
    if is_raises_exception:
        cpp_arguments.append('exception_state')
    if use_output_parameter_for_result:
        cpp_arguments.append('result')

    cpp_value = '%s(%s)' % (cpp_method_name, ', '.join(cpp_arguments))

    return {
        'cpp_type':
        idl_type.cpp_type,
        'cpp_value':
        cpp_value,
        'has_no_side_effect':
        has_no_side_effect,
        'is_call_with_script_state':
        is_call_with_script_state,
        'is_cross_origin':
        'CrossOrigin' in extended_attributes,
        'is_custom':
        'Custom' in extended_attributes and
        (not extended_attributes['Custom']
         or has_extended_attribute_value(getter, 'Custom', 'PropertyGetter')),
        'is_custom_property_enumerator':
        has_extended_attribute_value(getter, 'Custom', 'PropertyEnumerator'),
        'is_custom_property_query':
        has_extended_attribute_value(getter, 'Custom', 'PropertyQuery'),
        # TODO(rakuco): [NotEnumerable] does not make sense here and is only
        # used in non-standard IDL operations. We need to get rid of them.
        'is_enumerable':
        'NotEnumerable' not in extended_attributes,
        'is_null_expression':
        is_null_expression(idl_type),
        'is_raises_exception':
        is_raises_exception,
        'name':
        cpp_name(getter),
        'use_output_parameter_for_result':
        use_output_parameter_for_result,
        'v8_set_return_value':
        idl_type.v8_set_return_value(
            'result',
            extended_attributes=extended_attributes,
            script_wrappable='impl'),
    }


def property_setter(setter, interface):
    if not setter:
        return None

    extended_attributes = setter.extended_attributes
    idl_type = setter.arguments[1].idl_type
    idl_type.add_includes_for_type(extended_attributes)
    is_call_with_script_state = v8_utilities.has_extended_attribute_value(
        setter, 'CallWith', 'ScriptState')
    is_raises_exception = 'RaisesException' in extended_attributes
    is_ce_reactions = 'CEReactions' in extended_attributes

    has_type_checking_interface = idl_type.is_wrapper_type

    return {
        'has_exception_state':
        (is_raises_exception or idl_type.v8_conversion_needs_exception_state),
        'has_type_checking_interface':
        has_type_checking_interface,
        'idl_type':
        idl_type.base_type,
        'is_call_with_script_state':
        is_call_with_script_state,
        'is_ce_reactions':
        is_ce_reactions,
        'is_custom':
        'Custom' in extended_attributes,
        'is_nullable':
        idl_type.is_nullable,
        'is_raises_exception':
        is_raises_exception,
        'name':
        cpp_name(setter),
        'v8_value_to_local_cpp_value':
        idl_type.v8_value_to_local_cpp_value(extended_attributes, 'v8_value',
                                             'property_value'),
    }


def property_deleter(deleter):
    if not deleter:
        return None

    extended_attributes = deleter.extended_attributes
    is_call_with_script_state = v8_utilities.has_extended_attribute_value(
        deleter, 'CallWith', 'ScriptState')
    is_ce_reactions = 'CEReactions' in extended_attributes
    return {
        'is_call_with_script_state': is_call_with_script_state,
        'is_ce_reactions': is_ce_reactions,
        'is_custom': 'Custom' in extended_attributes,
        'is_raises_exception': 'RaisesException' in extended_attributes,
        'name': cpp_name(deleter),
    }
