// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxMemoryRecord.h>
#import <XCTest/XCTest.h>

@interface LynxMemoryRecordUnitTest : XCTestCase

@end

@implementation LynxMemoryRecordUnitTest

- (void)testCopyReturnsDetachedRecord {
  NSMutableDictionary<NSString *, NSString *> *detail = [@{@"url" : @"image-a"} mutableCopy];
  LynxMemoryRecord *record = [[LynxMemoryRecord alloc] initWithCategory:@"image"
                                                              sizeBytes:64
                                                                 detail:detail];
  record.instanceCount = 3;

  LynxMemoryRecord *recordCopy = [record copy];
  record.sizeBytes = 128;
  record.instanceCount = 6;
  detail[@"url"] = @"image-b";

  XCTAssertEqualObjects(recordCopy.category, @"image");
  XCTAssertEqual(recordCopy.sizeBytes, 64);
  XCTAssertEqual(recordCopy.instanceCount, 3);
  XCTAssertEqualObjects(recordCopy.detail[@"url"], @"image-a");
}

@end
