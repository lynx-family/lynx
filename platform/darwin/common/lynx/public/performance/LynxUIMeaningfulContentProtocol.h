// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef enum {
  /// Represents a UI component that does not render meaningful content and is
  /// irrelevant to FSP calculation (e.g., a container view).
  kLynxUIMeaningfulContentStatusIrrelevant = 0,
  /// Represents a UI component that contains meaningful content but is still
  /// pending load or render (e.g., an Image is downloading).
  kLynxUIMeaningfulContentStatusPending = 1,
  /// Represents a UI component that has finished loading and has presented its
  /// meaningful content to the user.
  kLynxUIMeaningfulContentStatusPresented = 2,
} LynxUIMeaningfulContentStatus;

/// The LynxUIMeaningfulContentProtocol defines the interface for LynxUI to
/// report meaningful content loading status.
@protocol LynxUIMeaningfulContentProtocol <NSObject>

/// Gets the current meaningful content loading status of the UI component. See
/// `LynxUIMeaningfulContentStatus` for details.
- (LynxUIMeaningfulContentStatus)meaningfulContentStatus;

/// Optionally provides a custom rect to be considered as the "meaningful
/// content" for FSP tracing. If not provided, the view's bounds will be used.
- (CGRect)meaningfulContentRect;

/// * @abstract Optionally provides the timestamp in microseconds when the first
/// meaningful content was presented.
/// @discussion This method is optional. If not implemented, it defaults to -1,
/// indicating that no specific timestamp is provided.
/// @return The timestamp in microseconds, or -1 if not implemented.
- (int64_t)firstMeaningfulContentPresentedTimestampMicros;

@end

NS_ASSUME_NONNULL_END
