// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_NET_MACROS_H_
#define CLAY_NET_MACROS_H_

#if (DEBUG_NET)
#define NET_LOG FML_LOG(ERROR) << "[CLAY]-[NET]"
#else
#define NET_LOG FML_EAT_STREAM_PARAMETERS(true)
#endif

#endif  // CLAY_NET_MACROS_H_
