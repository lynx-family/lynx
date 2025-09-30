// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_UPDATE_META_CAPI_H_
#define PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_UPDATE_META_CAPI_H_

#include "lynx_export.h"
#include "lynx_template_bundle_capi.h"
#include "lynx_template_data_capi.h"

LYNX_EXTERN_C_BEGIN

// LynxUpdateMeta is the metadata structure used by LynxView to update template
// data. It will serve as the input parameter for UpdateData. Users can specify
// various parameters required for the update through LynxUpdateMeta, such as
// template data and globalProps, among others.
typedef struct lynx_update_meta_t lynx_update_meta_t;

LYNX_CAPI_EXPORT lynx_update_meta_t* lynx_update_meta_create();
// The content of the data to update the template.
LYNX_CAPI_EXPORT void lynx_update_meta_set_update_data(
    lynx_update_meta_t*, lynx_template_data_t* data);

// The globalProps content for updating the template;
LYNX_CAPI_EXPORT void lynx_update_meta_set_global_props(
    lynx_update_meta_t*, lynx_template_data_t* data);

// Release the update meta.
LYNX_CAPI_EXPORT void lynx_update_meta_release(lynx_update_meta_t*);

LYNX_EXTERN_C_END

#endif  // PLATFORM_EMBEDDER_PUBLIC_CAPI_LYNX_UPDATE_META_CAPI_H_
