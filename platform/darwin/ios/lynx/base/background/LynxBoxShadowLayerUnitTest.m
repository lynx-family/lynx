// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBoxShadowLayer.h>
#import <Lynx/LynxUIView.h>
#import <XCTest/XCTest.h>

@interface LynxBoxShadowLayerUnitTest : XCTestCase {
  LynxBoxShadowLayer* layer;
}
@end

@implementation LynxBoxShadowLayerUnitTest

- (void)setUp {
  LynxUIView* ui = [[LynxUIView alloc] init];
  layer = [[LynxBoxShadowLayer alloc] initWithUi:ui];
}

/// Test the CF objects inside the layer will be release properly.
- (void)testCFObjectSetter {
  CGMutablePathRef mutablePath = CGPathCreateMutable();
  CGPathAddRect(mutablePath, nil, CGRectMake(0, 0, 100, 100));
  // Test setter & getter
  [layer setMaskPath:mutablePath];
  XCTAssertEqual(CFGetRetainCount(mutablePath), 2);
  XCTAssertTrue(CGPathEqualToPath(layer.maskPath, mutablePath));
  [layer setMaskPath:nil];
  XCTAssertEqual(layer.maskPath, NULL);

  [layer setCustomShadowPath:mutablePath];
  XCTAssertEqual(CFGetRetainCount(mutablePath), 2);
  XCTAssertTrue(CGPathEqualToPath(layer.customShadowPath, mutablePath));
  [layer setCustomShadowPath:nil];
  XCTAssertEqual(layer.customShadowPath, NULL);
  XCTAssertEqual(CFGetRetainCount(mutablePath), 1);

  [layer setCustomShadowPath:mutablePath];
  CGPathRelease(mutablePath);
  CGPathRef path = layer.customShadowPath;
  [layer setCustomShadowPath:path];
  XCTAssertEqual(CFGetRetainCount(path), 1);
}

@end
