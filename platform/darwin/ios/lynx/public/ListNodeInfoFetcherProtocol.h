// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

NS_ASSUME_NONNULL_BEGIN

@protocol ListNodeInfoFetcherProtocol <NSObject>

@required

- (void)scrollByListContainer:(int)containerSign
                            x:(float)x
                            y:(float)y
                    originalX:(float)originalX
                    originalY:(float)originalY;

- (void)scrollToPosition:(int)containerSign
                position:(int)position
                  offset:(float)offset
                   align:(int)align
                  smooth:(BOOL)smooth;

- (void)scrollStopped:(int)containerSign;

@end

NS_ASSUME_NONNULL_END
