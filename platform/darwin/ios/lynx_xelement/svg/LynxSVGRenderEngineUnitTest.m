// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <ServalSVG/SrSVG.h>
#import <XCTest/XCTest.h>
#import <XElement/LynxSVGRenderEngine.h>

@interface LynxSVGRenderEngineUnitTest : XCTestCase
@end

@implementation LynxSVGRenderEngineUnitTest

- (LynxSVGRenderEngine *)createEngineWithSrSVG:(SrSVG *)srSvg
                                         color:(NSString *)color
                                 asyncExecutor:(LynxSVGRenderAsyncExecutor)asyncExecutor {
  return [[LynxSVGRenderEngine alloc] initWithSrSVG:srSvg
                                              color:color
                                     genericFetcher:nil
                                       mediaFetcher:nil
                                      asyncExecutor:asyncExecutor
                                        reloadBlock:nil];
}

#pragma mark - testContentPreprocessingAndRender

- (void)testContentPreprocessingAndRender {
  SrSVG *srSvg = [[SrSVG alloc] init];
  NSString *simpleSVG = @"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" "
                        @"height=\"100\"><rect width=\"100\" height=\"100\" fill=\"red\"/></svg>";

  LynxSVGRenderAsyncExecutor syncExecutor =
      ^(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        completion(renderBlock());
      };

  LynxSVGRenderEngine *engine = [self createEngineWithSrSVG:srSvg
                                                      color:nil
                                              asyncExecutor:syncExecutor];
  engine.content = simpleSVG;

  __block UIImage *resultImage = nil;
  __block BOOL completionCalled = NO;

  [engine renderWithSize:CGSizeMake(100, 100)
      completion:^(UIImage *_Nullable image) {
        completionCalled = YES;
        resultImage = image;
      }
      errorHandler:^(LynxError *error) {
        XCTFail(@"errorHandler should not be called for valid SVG content");
      }];

  XCTAssertTrue(completionCalled, @"Completion should be called");
  XCTAssertNotNil(resultImage, @"Completion should receive a non-nil UIImage");
}

#pragma mark - testCancelPreventsCompletion

- (void)testCancelPreventsCompletion {
  SrSVG *srSvg = [[SrSVG alloc] init];
  NSString *simpleSVG = @"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" "
                        @"height=\"100\"><rect width=\"100\" height=\"100\" fill=\"red\"/></svg>";

  __block LynxSVGRenderBlock pendingRenderBlock = nil;
  __block LynxSVGRenderCompletion pendingCompletion = nil;

  LynxSVGRenderAsyncExecutor deferredExecutor =
      ^(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        pendingRenderBlock = renderBlock;
        pendingCompletion = completion;
      };

  LynxSVGRenderEngine *engine = [self createEngineWithSrSVG:srSvg
                                                      color:nil
                                              asyncExecutor:deferredExecutor];
  engine.content = simpleSVG;

  __block BOOL completionCalled = NO;

  [engine renderWithSize:CGSizeMake(100, 100)
              completion:^(UIImage *_Nullable image) {
                completionCalled = YES;
              }
            errorHandler:^(LynxError *error){
                // Not expected for this test
            }];

  // Completion should not be called yet because executor is deferred
  XCTAssertFalse(completionCalled);

  [engine cancel];

  // Now execute the deferred render
  if (pendingRenderBlock && pendingCompletion) {
    pendingCompletion(pendingRenderBlock());
  }

  XCTAssertFalse(completionCalled, @"Completion should NOT be called after cancel");
}

#pragma mark - testRenderWithEmptySrcAndContent

- (void)testRenderWithEmptySrcAndContent {
  SrSVG *srSvg = [[SrSVG alloc] init];

  LynxSVGRenderAsyncExecutor syncExecutor =
      ^(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        completion(renderBlock());
      };

  LynxSVGRenderEngine *engine = [self createEngineWithSrSVG:srSvg
                                                      color:nil
                                              asyncExecutor:syncExecutor];
  engine.src = nil;
  engine.content = nil;

  __block BOOL completionCalled = NO;
  __block UIImage *resultImage = nil;
  __block BOOL errorHandlerCalled = NO;

  [engine renderWithSize:CGSizeMake(100, 100)
      completion:^(UIImage *_Nullable image) {
        completionCalled = YES;
        resultImage = image;
      }
      errorHandler:^(LynxError *error) {
        errorHandlerCalled = YES;
      }];

  XCTAssertTrue(completionCalled, @"Completion should be called even with nil src and content");
  XCTAssertNil(resultImage, @"Result image should be nil when both src and content are nil");
  XCTAssertFalse(errorHandlerCalled, @"errorHandler should NOT be called");
}

#pragma mark - testRenderQueuesWhileRendering

- (void)testRenderQueuesWhileRendering {
  SrSVG *srSvg = [[SrSVG alloc] init];
  NSString *simpleSVG = @"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" "
                        @"height=\"100\"><rect width=\"100\" height=\"100\" fill=\"red\"/></svg>";

  LynxSVGRenderAsyncExecutor syncExecutor =
      ^(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        completion(renderBlock());
      };

  LynxSVGRenderEngine *engine = [self createEngineWithSrSVG:srSvg
                                                      color:nil
                                              asyncExecutor:syncExecutor];
  engine.content = simpleSVG;

  __block BOOL firstCompletionCalled = NO;
  __block BOOL secondCompletionCalled = NO;

  [engine renderWithSize:CGSizeMake(100, 100)
      completion:^(UIImage *_Nullable image) {
        firstCompletionCalled = YES;
      }
      errorHandler:^(LynxError *error) {
        XCTFail(@"First render should not error");
      }];

  [engine renderWithSize:CGSizeMake(100, 100)
      completion:^(UIImage *_Nullable image) {
        secondCompletionCalled = YES;
      }
      errorHandler:^(LynxError *error) {
        XCTFail(@"Second render should not error");
      }];

  XCTAssertTrue(firstCompletionCalled, @"First completion should be called");
  XCTAssertTrue(secondCompletionCalled, @"Second completion should be called");
}

#pragma mark - testAsyncExecutorIsCalled

- (void)testAsyncExecutorIsCalled {
  SrSVG *srSvg = [[SrSVG alloc] init];
  NSString *simpleSVG = @"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100\" "
                        @"height=\"100\"><rect width=\"100\" height=\"100\" fill=\"red\"/></svg>";

  __block BOOL asyncExecutorCalled = NO;
  __block LynxSVGRenderBlock capturedRenderBlock = nil;
  __block LynxSVGRenderCompletion capturedCompletion = nil;

  LynxSVGRenderAsyncExecutor trackingExecutor =
      ^(LynxSVGRenderBlock renderBlock, LynxSVGRenderCompletion completion) {
        asyncExecutorCalled = YES;
        capturedRenderBlock = renderBlock;
        capturedCompletion = completion;
        completion(renderBlock());
      };

  LynxSVGRenderEngine *engine = [self createEngineWithSrSVG:srSvg
                                                      color:nil
                                              asyncExecutor:trackingExecutor];
  engine.content = simpleSVG;

  __block BOOL completionCalled = NO;

  [engine renderWithSize:CGSizeMake(100, 100)
      completion:^(UIImage *_Nullable image) {
        completionCalled = YES;
      }
      errorHandler:^(LynxError *error) {
        XCTFail(@"errorHandler should not be called");
      }];

  XCTAssertTrue(asyncExecutorCalled, @"asyncExecutor should be called");
  XCTAssertNotNil(capturedRenderBlock, @"asyncExecutor should receive a renderBlock");
  XCTAssertNotNil(capturedCompletion, @"asyncExecutor should receive a completion block");
  XCTAssertTrue(completionCalled,
                @"Completion should be called after asyncExecutor invokes completion");
}

@end
