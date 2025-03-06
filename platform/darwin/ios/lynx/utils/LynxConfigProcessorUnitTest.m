// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxViewConfigProcessor.h>
#import <XCTest/XCTest.h>

@interface LynxConfigProcessorUnitTest : XCTestCase

@end

@implementation LynxConfigProcessorUnitTest

- (void)setUp {
  // Put setup code here. This method is called before the invocation of each test method in the
  // class.
}

- (void)tearDown {
  // Put teardown code here. This method is called after the invocation of each test method in the
  // class.
}

- (void)testRuntimeParse {
  NSMutableDictionary *params = [@{@"runtime_type" : @"jsc"} mutableCopy];
  LynxViewBuilder *builder = [[LynxViewBuilder alloc] init];
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertEqual(builder.backgroundJsRuntimeType, LynxBackgroundJsRuntimeTypeJSC);

  params[@"runtime_type"] = @"quickjs";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertEqual(builder.backgroundJsRuntimeType, LynxBackgroundJsRuntimeTypeQuickjs);

  params[@"runtime_type"] = @"v8";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  // currently we don't support v8, so use jsc instead.
  XCTAssertEqual(builder.backgroundJsRuntimeType, LynxBackgroundJsRuntimeTypeJSC);

  params[@"runtime_type"] = @"foo";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  // if params match nothing, use jsc default.
  XCTAssertEqual(builder.backgroundJsRuntimeType, LynxBackgroundJsRuntimeTypeJSC);
}

- (void)testEnableBytecodeParse {
  NSMutableDictionary *params = [@{@"enable_bytecode" : @"true"} mutableCopy];
  LynxViewBuilder *builder = [[LynxViewBuilder alloc] init];
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  params[@"enable_bytecode"] = @"True";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  params[@"enable_bytecode"] = @"Yes";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  params[@"enable_bytecode"] = @"yes";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  params[@"enable_bytecode"] = @"1";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  params[@"enable_bytecode"] = @(1);
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertTrue(builder.enableBytecode);
  builder.enableBytecode = NO;

  // set true for below test.
  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @"false";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);

  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @"False";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);

  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @"NO";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);

  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @"no";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);

  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @"0";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);

  builder.enableBytecode = YES;
  params[@"enable_bytecode"] = @(0);
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertFalse(builder.enableBytecode);
}

- (void)testBytecodeUrlParse {
  // not a string has no effect.
  NSMutableDictionary *params = [@{@"bytecode_url" : @(1)} mutableCopy];
  LynxViewBuilder *builder = [[LynxViewBuilder alloc] init];
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertEqual(builder.bytecodeUrl, nil);

  params[@"bytecode_url"] = @"foo";
  [LynxViewConfigProcessor processorMap:params lynxViewBuilder:builder];
  XCTAssertEqual(builder.bytecodeUrl, @"foo");
}

@end
