// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#if TARGET_OS_IOS || TARGET_OS_TV
#import <UIKit/UIKit.h>
#elif TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif

#if TARGET_OS_IOS || TARGET_OS_TV
typedef UIColor COLOR_CLASS;
typedef UITextView TEXTVIEW_CLASS;
typedef UIView VIEW_CLASS;
typedef UIImage IMAGE_CLASS;
#elif TARGET_OS_OSX
typedef NSColor COLOR_CLASS;
typedef NSTextView TEXTVIEW_CLASS;
typedef NSView VIEW_CLASS;
typedef NSImage IMAGE_CLASS;
#endif
