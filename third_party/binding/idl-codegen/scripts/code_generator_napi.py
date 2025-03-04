# Copyright (C) 2013 Google Inc. All rights reserved.
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

# pylint: disable=import-error,print-statement,relative-import
"""Generate Blink V8 bindings (.h and .cpp files).

If run itself, caches Jinja templates (and creates dummy file for build,
since cache filenames are unpredictable and opaque).

This module is *not* concurrency-safe without care: bytecode caching creates
a race condition on cache *write* (crashes if one process tries to read a
partially-written cache). However, if you pre-cache the templates (by running
the module itself), then you can parallelize compiling individual files, since
cache *reading* is safe.

Input: An object of class IdlDefinitions, containing an IDL interface X
Output: V8X.h and V8X.cpp

Design doc: http://www.chromium.org/developers/design-documents/idl-compiler
"""

import os
import posixpath

from blinkbuild.name_style_converter import NameStyleConverter
from code_generator import CodeGeneratorBase
from idl_definitions import Visitor
from idl_types import IdlType
import napi_callback_function
# import v8_callback_interface
import napi_dictionary
from napi_globals import includes
import napi_interface
# import v8_types
# import v8_union
from napi_utilities import build_basename, cpp_name, cpp_name_from_interfaces_info
from utilities import idl_filename_to_component, is_testing_target, shorten_union_name, to_header_guard, to_snake_case

# Make sure extension is .py, not .pyc or .pyo, so doesn't depend on caching
MODULE_PYNAME = os.path.splitext(os.path.basename(__file__))[0] + '.py'


def depending_union_type(idl_type):
    """Returns the union type name if the given idl_type depends on a
    union type.
    """

    def find_base_type(current_type):
        if current_type.is_array_or_sequence_type:
            return find_base_type(current_type.element_type)
        if current_type.is_record_type:
            # IdlRecordType.key_type is always a string type, so we only need
            # to looking into value_type.
            return find_base_type(current_type.value_type)
        if current_type.is_nullable:
            return find_base_type(current_type.inner_type)
        return current_type

    base_type = find_base_type(idl_type)
    if base_type.is_union_type:
        return base_type
    return None


class TypedefResolver(Visitor):
    def __init__(self, info_provider):
        self.info_provider = info_provider
        self.additional_header_includes = set()
        self.typedefs = {}

    def resolve(self, definitions, definition_name):
        """Traverse definitions and resolves typedefs with the actual types."""
        self.typedefs = {}
        for name, typedef in self.info_provider.typedefs.items():
            self.typedefs[name] = typedef.idl_type
        self.additional_header_includes = set()
        definitions.accept(self)
        self._update_dependencies_include_paths(definition_name)

    def _update_dependencies_include_paths(self, definition_name):
        if definition_name not in self.info_provider.interfaces_info:
            return
        interface_info = self.info_provider.interfaces_info[definition_name]
        interface_info['additional_header_includes'] = set(
            self.additional_header_includes)

    def _resolve_typedefs(self, typed_object):
        """Resolve typedefs to actual types in the object."""
        for attribute_name in typed_object.idl_type_attributes:
            try:
                idl_type = getattr(typed_object, attribute_name)
            except AttributeError:
                continue
            if not idl_type:
                continue
            resolved_idl_type = idl_type.resolve_typedefs(self.typedefs)
            # TODO(bashi): Dependency resolution shouldn't happen here.
            # Move this into includes_for_type() families.
            # union_type = depending_union_type(resolved_idl_type)
            # if union_type:
            #     self.additional_header_includes.add(
            #         self.info_provider.include_path_for_union_types(
            #             union_type))
            # Need to re-assign the attribute, not just mutate idl_type, since
            # type(idl_type) may change.
            setattr(typed_object, attribute_name, resolved_idl_type)

    def visit_typed_object(self, typed_object):
        self._resolve_typedefs(typed_object)


class CodeGeneratorNapiBase(CodeGeneratorBase):
    """Base class for v8 bindings generator and IDL dictionary impl generator"""

    def __init__(self, info_provider, cache_dir, output_dir):
        CodeGeneratorBase.__init__(self, MODULE_PYNAME, info_provider,
                                   cache_dir, output_dir)
        self.typedef_resolver = TypedefResolver(info_provider)

    def generate_code(self, definitions, definition_name):
        """Returns .h/.cpp code as ((path, content)...)."""
        # Set local type info
        if not self.should_generate_code(definitions):
            return set()

        # Resolve typedefs
        self.typedef_resolver.resolve(definitions, definition_name)
        return self.generate_code_internal(definitions, definition_name)

    def generate_code_internal(self, definitions, definition_name):
        # This should be implemented in subclasses.
        raise NotImplementedError()

    def get_output_filename(self, definition_name, ext, prefix=None):
        return build_basename(definition_name, prefix=prefix) + ext


class CodeGeneratorNapi(CodeGeneratorNapiBase):
    def __init__(self, info_provider, cache_dir, output_dir, hardcoded_includes={}):
        CodeGeneratorNapiBase.__init__(self, info_provider, cache_dir,
                                     output_dir)
        self.hardcoded_includes = hardcoded_includes

    def output_paths(self, definition_name, prefix='napi_'):
        header_path = posixpath.join(
            self.output_dir,
            self.get_output_filename(definition_name, '.h', prefix=prefix))
        cpp_path = posixpath.join(
            self.output_dir,
            self.get_output_filename(definition_name, '.cc', prefix=prefix))
        return header_path, cpp_path

    def js_output_path(self, definition_name):
        js_path = posixpath.join(
            self.output_dir,
            NameStyleConverter(definition_name).to_lower_camel_case() +'.ts')
        return js_path

    def command_buffer_output_path(self):
        header_path = posixpath.join(
            self.output_dir,
            self.get_output_filename('CommandBuffer', '.h', prefix='napi_'))
        cpp_path = posixpath.join(
            self.output_dir,
            self.get_output_filename('CommandBuffer', '.cc', prefix='napi_'))
        return header_path, cpp_path

    def generate_code_internal(self, definitions, definition_name):
        if definition_name in definitions.interfaces:
            return self.generate_interface_code(
                definitions, definition_name,
                definitions.interfaces[definition_name])
        if definition_name in definitions.dictionaries:
            return self.generate_dictionary_code(
                definitions, definition_name,
                definitions.dictionaries[definition_name])
        raise ValueError('%s is not in IDL definitions' % definition_name)

    def init_command_buffer(self, component):
        napi_interface.init_command_buffer_context(component, self.hardcoded_includes.get(component, {}))

    def finish_command_buffer(self):
        template_context = napi_interface.finish_command_buffer()
        if not template_context:
            return
        header_template_filename = 'napi_command_buffer.h.tmpl'
        cpp_template_filename = 'napi_command_buffer.cc.tmpl'
        header_template = self.jinja_env.get_template(header_template_filename)
        cpp_template = self.jinja_env.get_template(cpp_template_filename)
        name = NameStyleConverter(template_context['component'] + 'CommandBuffer').to_upper_camel_case()
        header_path, _ = self.output_paths(name)
        _, cpp_path = self.output_paths(name + 'Gen')
        _, remote_cpp_path = self.output_paths(name + 'Gen', prefix='remote_')
        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        template_context['header_includes'] = []
        template_context['napi_class'] = 'Napi' + name
        template_context['hardcoded_includes'] = self.hardcoded_includes.get(template_context['component'], {}).get(name, [])
        template_context['macro_prefix'] = self.hardcoded_includes.get(template_context['component'], {}).get('macro_prefix', '')
        include_paths = []
        napi_disabled = self.hardcoded_includes.get(template_context['component'], {}).get('napi_disabled', False)
        if napi_disabled:
            out = []
        else:
            _, cpp_text = self.render_templates(
                include_paths, header_template, cpp_template, template_context,
                template_context['component'])
            out = [
                (cpp_path, cpp_text),
            ]
        generates_remote = self.hardcoded_includes.get(template_context['component'], {}).get('has_remote', False)
        if generates_remote:
            try:
                remote_cpp_template_filename = 'remote_command_buffer.cc.tmpl'
                remote_cpp_template = self.jinja_env.get_template(remote_cpp_template_filename)
                _, remote_cpp_text = self.render_templates(
                    include_paths, remote_cpp_template, remote_cpp_template, template_context,
                    template_context['component'])
                out.append((remote_cpp_path, remote_cpp_text))
            except:
                pass
        for js_class in template_context.get('interfaces', []):
            if js_class['shared_impl']:
                continue
            class_name = js_class['type_name']
            parent_name = js_class['parent']
            class_context = {
                'interface_name': class_name,
                'parent_name': parent_name,
                # For methods we need to keep the shared base impl to have consistent index with native side.
                'methods': filter(
                lambda method: method['class_name'] == class_name or method['class_name'] == parent_name, template_context['methods']),
                'remote_methods': filter(
                lambda method: method['class_name'] == class_name or method['class_name'] == parent_name, template_context['remote_methods']),
                # For constants just use the inherited for simplicity.
                'constants': filter(lambda constant: constant['class_name'] == class_name, template_context['constants']),
                'overloads_child_only': js_class['overloads_child_only'],
                'async_classes': template_context.get('async_classes', []),
                'partial_data_opt_prefix': self.hardcoded_includes.get(template_context['component'], {}).get('partial_data_opt_prefix', '0'),
                'overload_resolvers_prefix': self.hardcoded_includes.get(template_context['component'], {}).get('overload_resolvers_prefix', '0'),
                'remote_type_id': js_class['remote_type_id'],
                'remote_constructor_num': js_class['remote_constructor_num'],
                'remote_destructor_id': js_class['remote_destructor_id'],
            }
            js_template = self.jinja_env.get_template('opts.js.tmpl')
            js_path = self.js_output_path(js_class['type_name'])
            from code_generator import render_template
            if not js_class['remote_only']:
                js_text = render_template(js_template, class_context)
                out.append((js_path, js_text))
            if generates_remote:
                try:
                    remote_js_template = self.jinja_env.get_template('remote.js.tmpl')
                    remote_js_path = self.js_output_path('remote' + js_class['type_name'])
                    remote_js_text = render_template(remote_js_template, class_context)
                    out.append((remote_js_path, remote_js_text))
                except:
                    pass

        return out

    def generate_interface_code(self, definitions, interface_name, interface):
        interface_info = self.info_provider.interfaces_info[interface_name]
        full_path = interface_info.get('full_path')
        component = idl_filename_to_component(full_path)
        include_paths = interface_info.get('dependencies_include_paths')

        # Select appropriate Jinja template and contents function
        if interface.is_callback:
            # header_template_filename = 'callback_interface.h.tmpl'
            # cpp_template_filename = 'callback_interface.cc.tmpl'
            # interface_context = v8_callback_interface.callback_interface_context
            return
        elif interface.is_partial:
            interface_context = v8_interface.interface_context
            header_template_filename = 'partial_interface.h.tmpl'
            cpp_template_filename = 'partial_interface.cc.tmpl'
            interface_name += 'Partial'
            assert component == 'core'
            component = 'modules'
            include_paths = interface_info.get(
                'dependencies_other_component_include_paths')
            raise ValueError('interface.is_partial')
        elif interface.is_mixin:
            if interface.shared_impl:
                header_template_filename = 'napi_shared_impl.h.tmpl'
                cpp_template_filename = 'napi_shared_impl.cc.tmpl'
                interface_context = napi_interface.interface_context
            else:
                return
        else:
            header_template_filename = 'napi_interface.h.tmpl'
            cpp_template_filename = 'napi_interface.cc.tmpl'
            interface_context = napi_interface.interface_context

        component_info = self.info_provider.component_info
        template_context = interface_context(interface, definitions.interfaces,
                                             component_info, self.info_provider.interfaces_info)
        # Skip generating async interfaces.
        if interface.async_created:
            return ()
        template_context['component'] = component
        template_context['attach_native_info'] = component_info.get('has_command_buffer', False)
        includes.update(
            interface_info.get('cpp_includes', {}).get(component, set()))
        # if not interface.is_partial and not is_testing_target(full_path):
            # template_context['header_includes'].add(
            #     self.info_provider.include_path_for_export)
            # template_context['exported'] = \
            #     self.info_provider.specifier_for_export
        # Add the include for interface itself
        if IdlType(interface_name).is_typed_array:
            template_context['header_includes'].add(
                'core/typed_arrays/dom_typed_array.h')
        elif interface.is_callback:
            pass
        else:
            includes.update(set({interface_info['include_path']}))
        # template_context['header_includes'].update(
        #     interface_info.get('additional_header_includes', []))
        header_path, cpp_path = self.output_paths(interface_name)
        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        template_context['hardcoded_includes'] = self.hardcoded_includes.get(component, {}).get(interface_name, [])
        if 'third_party/binding/napi/exception_state.h' in includes :
            template_context['hardcoded_includes'].append('third_party/binding/napi/exception_state.h')
        template_context['hardcoded_includes'].append('third_party/binding/napi/exception_message.h')
        template_context['hardcoded_includes'].append('third_party/binding/napi/napi_base_wrap.h')
        if template_context['export_impl_getter']:
            template_context['header_includes'].add('base/include/base_export.h')
        # add common headers to component
        common_headers = self.hardcoded_includes.get(component, {}).get('common_headers', [])
        for header in common_headers:
            template_context['hardcoded_includes'].append(header)
        # Add header for tracing
        tracing_headers = self.hardcoded_includes.get(component, {}).get('tracing_headers', [])
        if template_context['enable_trace']:
            template_context['hardcoded_includes']+= tracing_headers
        template_context['hardcoded_includes'].sort()
        template_context['macro_prefix'] = self.hardcoded_includes.get(component, {}).get('macro_prefix', '')
        header_template = self.jinja_env.get_template(header_template_filename)
        cpp_template = self.jinja_env.get_template(cpp_template_filename)
        if template_context['parent_interface']:
            parent_name = template_context['parent_interface']
            template_context['parent_cpp_class'] = cpp_name_from_interfaces_info(parent_name, self.info_provider.interfaces_info)
        template_context['has_remote'] = self.hardcoded_includes.get(component, {}).get('has_remote', False)
        napi_disabled = self.hardcoded_includes.get(template_context['component'], {}).get('napi_disabled', False)
        if napi_disabled:
            return ()
        header_text, cpp_text = self.render_templates(
            include_paths, header_template, cpp_template, template_context,
            component)
        return (
            (header_path, header_text),
            (cpp_path, cpp_text),
        )

    def generate_dictionary_code(self, definitions, dictionary_name,
                                 dictionary):
        # pylint: disable=unused-argument
        interfaces_info = self.info_provider.interfaces_info
        header_template = self.jinja_env.get_template('napi_dictionary.h.tmpl')
        cpp_template = self.jinja_env.get_template('napi_dictionary.cc.tmpl')
        interface_info = interfaces_info[dictionary_name]
        component_info = self.info_provider.component_info
        template_context = napi_dictionary.dictionary_context(
            dictionary, interfaces_info, component_info)
        full_path = interface_info.get('full_path')
        component = idl_filename_to_component(full_path)
        template_context['component'] = component
        include_paths = interface_info.get('dependencies_include_paths')
        # Add the include for interface itself
        # template_context['header_includes'].add(interface_info['include_path'])
        # if not is_testing_target(interface_info.get('full_path')):
        #     template_context['header_includes'].add(
        #         self.info_provider.include_path_for_export)
        #     template_context['exported'] = \
        #         self.info_provider.specifier_for_export
        header_path, cpp_path = self.output_paths(dictionary_name)
        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        template_context['hardcoded_includes'] = self.hardcoded_includes.get(component, {}).get(dictionary_name, [])
        template_context['hardcoded_includes'].append('third_party/binding/napi/exception_message.h')

        # add common headers to component
        common_headers = self.hardcoded_includes.get(component, {}).get('common_headers', [])
        for header in common_headers:
            template_context['hardcoded_includes'].append(header)
        template_context['hardcoded_includes'].sort()
        out = []
        generates_remote = self.hardcoded_includes.get(component, {}).get('has_remote', False)
        if generates_remote:
            excluding_regexps = self.hardcoded_includes.get(component, {}).get('remote_excluding_regexps', [])
            skip_generation = False
            for regex in excluding_regexps:
                import re
                if re.search(regex, dictionary_name):
                    skip_generation = True
                    break
            if not skip_generation:
                try:
                    template_context['has_remote'] = True
                    remote_cpp_template_filename = 'remote_dictionary.cc.tmpl'
                    remote_cpp_template = self.jinja_env.get_template(remote_cpp_template_filename)
                    _, remote_cpp_text = self.render_templates(
                        include_paths, remote_cpp_template, remote_cpp_template, template_context,
                        component)
                    _, remote_cpp_path = self.output_paths(dictionary_name, prefix='remote_')
                    out.append((remote_cpp_path, remote_cpp_text))

                    remote_js_template = self.jinja_env.get_template('remote_dictionary.js.tmpl')
                    _, remote_js_text = self.render_templates(
                        include_paths, remote_js_template, remote_js_template, template_context,
                        component)
                    remote_js_path = self.js_output_path('remote' + dictionary_name)
                    out.append((remote_js_path, remote_js_text))
                except Exception as e:
                    pass
        header_text, cpp_text = self.render_templates(
            include_paths, header_template, cpp_template, template_context)
        out.append((header_path, header_text))
        out.append((cpp_path, cpp_text))
        return out


class CodeGeneratorDictionaryImpl(CodeGeneratorNapiBase):
    def __init__(self, info_provider, cache_dir, output_dir, hardcoded_includes={}):
        CodeGeneratorV8Base.__init__(self, info_provider, cache_dir,
                                     output_dir)

    def output_paths(self, definition_name, interface_info):
        output_dir = posixpath.join(self.output_dir,
                                    interface_info['relative_dir'])
        header_path = posixpath.join(
            output_dir, self.get_output_filename(definition_name, '.h'))
        cpp_path = posixpath.join(
            output_dir, self.get_output_filename(definition_name, '.cc'))
        return header_path, cpp_path

    def generate_code_internal(self, definitions, definition_name):
        if not definition_name in definitions.dictionaries:
            raise ValueError('%s is not an IDL dictionary' % definition_name)
        interfaces_info = self.info_provider.interfaces_info
        dictionary = definitions.dictionaries[definition_name]
        interface_info = interfaces_info[definition_name]
        header_template = self.jinja_env.get_template('dictionary_impl.h.tmpl')
        cpp_template = self.jinja_env.get_template('dictionary_impl.cc.tmpl')
        template_context = napi_dictionary.dictionary_impl_context(
            dictionary, interfaces_info)
        include_paths = interface_info.get('dependencies_include_paths')
        if not is_testing_target(interface_info.get('full_path')):
            template_context['exported'] = \
                self.info_provider.specifier_for_export
            template_context['header_includes'].add(
                self.info_provider.include_path_for_export)
        template_context['header_includes'].update(
            interface_info.get('additional_header_includes', []))
        header_path, cpp_path = self.output_paths(definition_name,
                                                  interface_info)
        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        header_text, cpp_text = self.render_templates(
            include_paths, header_template, cpp_template, template_context)
        return (
            (header_path, header_text),
            (cpp_path, cpp_text),
        )


class CodeGeneratorUnionType(CodeGeneratorBase):
    """Generates union type container classes.
    This generator is different from CodeGeneratorV8 and
    CodeGeneratorDictionaryImpl. It assumes that all union types are already
    collected. It doesn't process idl files directly.
    """

    def __init__(self, info_provider, cache_dir, output_dir, target_component):
        CodeGeneratorBase.__init__(self, MODULE_PYNAME, info_provider,
                                   cache_dir, output_dir)
        self.target_component = target_component
        # The code below duplicates parts of TypedefResolver. We do not use it
        # directly because IdlUnionType is not a type defined in
        # idl_definitions.py. What we do instead is to resolve typedefs in
        # _generate_container_code() whenever a new union file is generated.
        self.typedefs = {}
        for name, typedef in self.info_provider.typedefs.items():
            self.typedefs[name] = typedef.idl_type

    def _generate_container_code(self, union_type):
        includes.clear()
        union_type = union_type.resolve_typedefs(self.typedefs)
        header_template = self.jinja_env.get_template('union_container.h.tmpl')
        cpp_template = self.jinja_env.get_template('union_container.cc.tmpl')
        template_context = v8_union.container_context(union_type,
                                                      self.info_provider)
        template_context['header_includes'].append(
            self.info_provider.include_path_for_export)
        template_context['exported'] = self.info_provider.specifier_for_export
        snake_base_name = to_snake_case(shorten_union_name(union_type))
        header_path = posixpath.join(self.output_dir, '%s.h' % snake_base_name)
        cpp_path = posixpath.join(self.output_dir, '%s.cc' % snake_base_name)
        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        header_text, cpp_text = self.render_templates(
            [], header_template, cpp_template, template_context)
        return (
            (header_path, header_text),
            (cpp_path, cpp_text),
        )

    def _get_union_types_for_containers(self):
        union_types = self.info_provider.union_types
        if not union_types:
            return None
        # For container classes we strip nullable wrappers. For example,
        # both (A or B)? and (A? or B) will become AOrB. This should be OK
        # because container classes can handle null and it seems that
        # distinguishing (A or B)? and (A? or B) doesn't make sense.
        container_cpp_types = set()
        union_types_for_containers = set()
        for union_type in union_types:
            cpp_type = union_type.cpp_type
            if cpp_type not in container_cpp_types:
                union_types_for_containers.add(union_type)
                container_cpp_types.add(cpp_type)
        return union_types_for_containers

    def generate_code(self):
        union_types = self._get_union_types_for_containers()
        if not union_types:
            return ()
        outputs = set()
        for union_type in union_types:
            outputs.update(self._generate_container_code(union_type))
        return outputs


class CodeGeneratorCallbackFunction(CodeGeneratorBase):
    def __init__(self, info_provider, cache_dir, output_dir, target_component, hardcoded_includes={}):
        CodeGeneratorBase.__init__(self, MODULE_PYNAME, info_provider,
                                   cache_dir, output_dir)
        self.target_component = target_component
        self.typedef_resolver = TypedefResolver(info_provider)
        self.hardcoded_includes = hardcoded_includes

    def generate_code_internal(self, callback_function, path):
        self.typedef_resolver.resolve(callback_function,
                                      callback_function.name)
        header_template = self.jinja_env.get_template(
            'napi_callback_function.h.tmpl')
        cpp_template = self.jinja_env.get_template('napi_callback_function.cc.tmpl')
        template_context = napi_callback_function.callback_function_context(
            callback_function)
        # if not is_testing_target(path):
        #     template_context['exported'] = \
        #         self.info_provider.specifier_for_export
        #     template_context['header_includes'].append(
        #         self.info_provider.include_path_for_export)

        # TODO(bashi): Dependency resolution shouldn't happen here.
        # Move this into includes_for_type() families.
        # for argument in callback_function.arguments:
        #     if argument.idl_type.is_union_type:
        #         template_context['header_includes'].append(
        #             self.info_provider.include_path_for_union_types(
        #                 argument.idl_type))

        snake_base_name = to_snake_case('Napi%s' % callback_function.name)
        header_path = posixpath.join(
            self.output_dir,build_basename(callback_function.name, prefix='napi_') + '.h')
        cpp_path = posixpath.join(
            self.output_dir,build_basename(callback_function.name, prefix='napi_') + '.cc')

        this_include_header_path = self.normalize_this_header_path(header_path)
        this_include_header_guard_path = self.normalize_this_header_guard_path(header_path)
        template_context['this_include_header_path'] = this_include_header_path
        template_context['header_guard'] = to_header_guard(
            this_include_header_guard_path)
        template_context['component'] = self.target_component
        template_context['hardcoded_includes'] = self.hardcoded_includes.get(self.target_component, {}).get(callback_function.name, [])

        # add common headers to component
        common_headers = self.hardcoded_includes.get(self.target_component, {}).get('common_headers', [])
        for header in common_headers:
            template_context['hardcoded_includes'].append(header)
        template_context['hardcoded_includes'].sort()
        header_text, cpp_text = self.render_templates(
            [], header_template, cpp_template, template_context)
        return (
            (header_path, header_text),
            (cpp_path, cpp_text),
        )

    # pylint: disable=W0221
    def generate_code(self):
        callback_functions = self.info_provider.callback_functions
        if not callback_functions:
            return ()
        outputs = set()
        for callback_function_dict in callback_functions.values():
            if callback_function_dict['component_dir'] != self.target_component:
                continue
            callback_function = callback_function_dict['callback_function']
            if 'Custom' in callback_function.extended_attributes:
                continue
            path = callback_function_dict['full_path']
            outputs.update(
                self.generate_code_internal(callback_function, path))
        return outputs
