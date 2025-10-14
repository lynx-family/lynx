// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_MACROS_H_
#define CLAY_UI_COMPONENT_LIST_MACROS_H_

#define LIST_DEBUG_TAG "[Clay]-[LIST] "
#if (DEBUG_LIST)
#define LIST_LOG FML_LOG(ERROR) << LIST_DEBUG_TAG
#else
#define LIST_LOG FML_EAT_STREAM_PARAMETERS(true)
#endif

#endif  // CLAY_UI_COMPONENT_LIST_MACROS_H_
