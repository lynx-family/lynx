// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOAD_META_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOAD_META_CAPI_H_

#include "lynx_export.h"
#include "lynx_template_bundle_capi.h"
#include "lynx_template_data_capi.h"

LYNX_EXTERN_C_BEGIN

// LynxLoadMeta is the metadata structure used by LynxView to load templates. It
// will be used as the input parameter of loadTemplate. Users can use
// LynxLoadMeta to specify many parameters of loading templates, such as
// specifying template content, template initial data, and other additional
// configuration information. url, binary_data, bundle are the required
// parameters for template loading. You can pass only one or all of them. The
// priority relationship is as follows: bundle > binary_data > url.
typedef struct lynx_load_meta_t lynx_load_meta_t;

LYNX_CAPI_EXPORT lynx_load_meta_t* lynx_load_meta_create();

// The file path of the template.
LYNX_CAPI_EXPORT void lynx_load_meta_set_url(lynx_load_meta_t*, const char*);

// The binary file data of the template. You can also
// provide a custom destructor function to handle the cleanup of the content
// memory, and pass an opaque pointer for additional context.
LYNX_CAPI_EXPORT void lynx_load_meta_set_binary_data(
    lynx_load_meta_t*, uint8_t* content, size_t length,
    void (*dtor)(uint8_t*, size_t, void*), void* opaque);

// TemplateBundle object parsed in advance by the template's binary file data
LYNX_CAPI_EXPORT void lynx_load_meta_set_template_bundle(
    lynx_load_meta_t*, lynx_template_bundle_t*);

// Set the initial data for the template. Initial data specified by the client
// during the first screen loading process
LYNX_CAPI_EXPORT void lynx_load_meta_set_initial_data(lynx_load_meta_t*,
                                                      lynx_template_data_t*);

// Set the global props data for the template.
LYNX_CAPI_EXPORT void lynx_load_meta_set_global_props(lynx_load_meta_t*,
                                                      lynx_template_data_t*);

// Release the load meta.
LYNX_CAPI_EXPORT void lynx_load_meta_release(lynx_load_meta_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_LOAD_META_CAPI_H_
