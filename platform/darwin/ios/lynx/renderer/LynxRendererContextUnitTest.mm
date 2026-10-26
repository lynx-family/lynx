// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxServiceTextProtocol.h>
#import "LynxRendererContext+Internal.h"

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxImageLoader.h>
#import <Lynx/LynxImageManager.h>
#import <Lynx/LynxMeasureDelegate.h>
#import <Lynx/LynxRendererContext.h>
#import <Lynx/LynxServiceImageProtocol.h>
#import <Lynx/LynxTextRenderManager.h>
#import <Lynx/LynxUIContext+Internal.h>
#import <Lynx/LynxUIContext.h>
#import <Lynx/LynxUIImage.h>
#import <OCMock/OCMock.h>
#import <XCTest/XCTest.h>
#include "core/renderer/ui_wrapper/painting/paint_image.h"

@interface LynxImageManager (ImageLoadingTest)
- (void)sendCustomEvent:(NSString *)name withParams:(NSDictionary *)params;
@end

@interface LynxRendererContext (ImageLoadingTest)
- (LynxImageManager *)imageManagerForSign:(int32_t)sign;
@end

@interface LynxRendererContextUnitTest : XCTestCase
@end

@implementation LynxRendererContextUnitTest

- (void)testProperties {
  LynxRendererContext *context = [[LynxRendererContext alloc] init];

  // Test bodyView property
  id mockBodyView = OCMProtocolMock(@protocol(LUIBodyView));
  context.bodyView = mockBodyView;
  XCTAssertEqual(context.bodyView, mockBodyView);

  // Test textRenderManager property
  id mockTextRenderManager = OCMClassMock([LynxTextRenderManager class]);
  context.textRenderManager = mockTextRenderManager;
  XCTAssertEqual(context.textRenderManager, mockTextRenderManager);
}

- (void)testTextBundleVersionChangesForInPlaceUpdatesAndRecreatedIDs {
  id service = OCMProtocolMock(@protocol(LynxServiceTextProtocol));
  id services = OCMClassMock([LynxServices class]);
  OCMStub(ClassMethod([services getInstanceWithProtocol:@protocol(LynxServiceTextProtocol)]))
      .andReturn(service);
  LynxRendererContext *context = [LynxRendererContext new];
  int pageStorage = 0;
  void *page = &pageStorage;
  XCTAssertEqual([context textBundleVersion:1], 0u);
  [context updateTextBundle:1 withBundle:page];
  uint64_t firstVersion = [context textBundleVersion:1];
  XCTAssertGreaterThan(firstVersion, 0u);
  [context updateTextBundle:1 withBundle:page];
  uint64_t secondVersion = [context textBundleVersion:1];
  XCTAssertGreaterThan(secondVersion, firstVersion);
  [context destroyTextBundle:1];
  XCTAssertEqual([context textBundleVersion:1], 0u);
  [context updateTextBundle:1 withBundle:page];
  XCTAssertGreaterThan([context textBundleVersion:1], secondVersion);
  [context updateTextBundle:1 withBundle:nullptr];
  XCTAssertEqual([context textBundleVersion:1], 0u);
  XCTAssertTrue([context getTextBundle:1] == nullptr);
  [services stopMocking];
}

- (void)testMissingTextPagesDoNotAccumulateVersionMetadata {
  LynxRendererContext *context = [LynxRendererContext new];
  for (int32_t textID = 0; textID < 100; ++textID) {
    [context updateTextBundle:textID withBundle:nullptr];
    XCTAssertEqual([context textBundleVersion:textID], 0u);
  }
  // Empty legacy-text updates must not leave one cache entry per discarded ID.
  XCTAssertEqual(((NSDictionary *)[context valueForKey:@"textBundleVersions"]).count, 0u);
  XCTAssertEqual(((NSDictionary *)[context valueForKey:@"textBundles"]).count, 0u);
}

@end

@interface LynxEmbeddedImageLoadingUnitTest : XCTestCase
@end

@implementation LynxEmbeddedImageLoadingUnitTest {
  id _loader;
  NSMutableArray<LynxImageLoadOptions *> *_requests;
  NSMutableArray<LynxURL *> *_requestURLs;
  NSMutableArray<NSNumber *> *_cancelled;
  NSMutableArray<NSNumber *> *_requestThreads;
  LynxRendererContext *_context;
  BOOL _completeSynchronously;
  dispatch_block_t _beforeLoaderReturns;
}

- (void)setUp {
  [super setUp];
  _requests = [NSMutableArray new];
  _requestURLs = [NSMutableArray new];
  _cancelled = [NSMutableArray new];
  _requestThreads = [NSMutableArray new];
  _context = [LynxRendererContext new];
  _loader = OCMClassMock([LynxImageLoader class]);
  OCMStub(ClassMethod([_loader sharedInstance])).andReturn(_loader);
  __weak typeof(self) weakSelf = self;
  OCMStub([_loader loadImageWithOptions:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    typeof(self) self = weakSelf;
    __unsafe_unretained LynxImageLoadOptions *options = nil;
    [invocation getArgument:&options atIndex:2];
    NSUInteger index = self->_requests.count;
    [self->_requests addObject:options];
    [self->_requestThreads addObject:@(NSThread.isMainThread)];
    // imageURL is an assign property; deferred mock callbacks must retain it.
    [self->_requestURLs addObject:options.imageURL];
    if (self->_completeSynchronously) {
      options.completed([UIImage new], nil, options.imageURL.url);
    }
    if (self->_beforeLoaderReturns) {
      self->_beforeLoaderReturns();
    }
    NSMutableArray<NSNumber *> *cancelled = self->_cancelled;
    dispatch_block_t cancel = ^{
      XCTAssertTrue(NSThread.isMainThread);
      [cancelled addObject:@(index)];
      // Some fetchers complete even while cancellation is in progress.
      options.completed([UIImage new], nil, options.imageURL.url);
    };
    [invocation retainArguments];
    [invocation setReturnValue:&cancel];
  });
}

- (void)tearDown {
  _beforeLoaderReturns = nil;
  _context = nil;
  [_loader stopMocking];
  [super tearDown];
}

- (void)runOnBackgroundThread:(dispatch_block_t)block {
  // dispatch_sync may execute inline. Keep the main queue blocked until the
  // background work finishes so queued image results cannot apply prematurely.
  XCTAssertTrue(NSThread.isMainThread);
  dispatch_semaphore_t finished = dispatch_semaphore_create(0);
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    XCTAssertFalse(NSThread.isMainThread);
    block();
    dispatch_semaphore_signal(finished);
  });
  XCTAssertEqual(
      dispatch_semaphore_wait(finished, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)), 0);
}

- (LynxURL *)url:(NSString *)url type:(LynxImageRequestType)type {
  LynxURL *result = [LynxURL new];
  result.url = [NSURL URLWithString:url];
  result.type = type;
  result.imageSize = CGSizeMake(100, 80);
  return result;
}

- (void)createImage:(int32_t)key background:(BOOL)background {
  lynx::tasm::ImagePaintInfo info;
  [self createImage:key size:CGSizeMake(100, 80) paintInfo:info background:background];
}

- (void)createImage:(int32_t)key
               size:(CGSize)size
          paintInfo:(const lynx::tasm::ImagePaintInfo &)info
         background:(BOOL)background {
  LynxURL *source = [self url:@"https://example.com/image.gif" type:LynxImageRequestSrc];
  LynxURL *placeholder = [self url:@"https://example.com/placeholder.png"
                              type:LynxImageRequestPlaceholder];
  source.imageSize = placeholder.imageSize = size;
  lynx::tasm::ImagePaintInfo paintInfo = info;
  paintInfo.is_background = background;
  [_context createImageManager:7
                 withSourceURL:source
             andPlaceholderURL:placeholder
                     paintInfo:paintInfo
                     eventMask:3
                      imageKey:key];
}

- (void)testImageStreamsFramesAndBackgroundPreloadsFrames {
  [self createImage:1 background:NO];
  [self createImage:2 background:YES];
  XCTAssertEqual(_requests.count, 4u);
  XCTAssertEqualObjects(_requests[0].contextInfo[LynxImagePreloadAllFrames], @NO);
  XCTAssertEqualObjects(_requests[1].contextInfo[LynxImagePreloadAllFrames], @NO);
  XCTAssertEqualObjects(_requests[2].contextInfo[LynxImagePreloadAllFrames], @YES);
  XCTAssertEqualObjects(_requests[3].contextInfo[LynxImagePreloadAllFrames], @YES);
  XCTAssertEqual(_cancelled.count, 0u);
  XCTAssertEqual([_context imageManagerForSign:7], [_context imageManagerForID:1]);
  [self createImage:3 background:NO];
  XCTAssertEqual(_cancelled.count, 2u);
  XCTAssertFalse([_cancelled containsObject:@2]);
  XCTAssertFalse([_cancelled containsObject:@3]);
  XCTAssertNotNil([_context imageManagerForID:2]);
}

- (void)testQueuedAutoSizeLayoutIgnoresReplacedRequest {
  LynxUIContext *uiContext =
      [[LynxUIContext alloc] initWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]];
  _context.uiContext = uiContext;
  id contextMock = OCMPartialMock(uiContext);
  NSMutableArray<void (^)(LynxShadowNode *)> *tasks = [NSMutableArray new];
  OCMStub([contextMock findShadowNodeAndRunTask:7 task:[OCMArg any]])
      .andDo(^(NSInvocation *invocation) {
        __unsafe_unretained void (^task)(LynxShadowNode *) = nil;
        [invocation getArgument:&task atIndex:3];
        [tasks addObject:[task copy]];
      });
  lynx::tasm::ImagePaintInfo info;
  info.auto_size = true;
  [self createImage:1 size:CGSizeMake(100, 0) paintInfo:info background:NO];
  // Keep the old manager alive so rejection depends on its generation.
  LynxImageManager *oldManager = [_context imageManagerForID:1];
  UIImage *image = OCMClassMock(UIImage.class);
  OCMStub([image size]).andReturn(CGSizeMake(600, 400));
  _requests[0].completed(image, nil, _requests[0].imageURL.url);
  [self runOnBackgroundThread:^{
    [self createImage:2 size:CGSizeMake(100, 0) paintInfo:info background:NO];
  }];
  _requests[2].completed(image, nil, _requests[2].imageURL.url);
  XCTAssertEqual(tasks.count, 2u);
  LynxImageShadowNode *node = [[LynxImageShadowNode alloc] initWithSign:7 tagName:@"image"];
  id nodeMock = OCMPartialMock(node);
  OCMReject([nodeMock setImageSize:CGSizeMake(600, 400)]);
  tasks[0](node);
  [nodeMock stopMocking];
  nodeMock = OCMPartialMock(node);
  OCMExpect([nodeMock setNeedsLayout]);
  tasks[1](node);
  OCMVerifyAll(nodeMock);
  CGSize measured = [(id<LynxMeasureDelegate>)node measureNode:node
                                                     withWidth:100
                                                     widthMode:LynxMeasureModeDefinite
                                                        height:0
                                                    heightMode:LynxMeasureModeIndefinite];
  XCTAssertEqualWithAccuracy(measured.height, 100.0 * 400.0 / 600.0, 0.001);
  XCTAssertNotNil(oldManager);
  [nodeMock stopMocking];
  [(id)image stopMocking];
  [contextMock stopMocking];
}

- (void)testFetcherSelectionUsesTheConfiguredLoader {
  // Keep the real loader routing and mock only its two destinations.
  [_loader stopMocking];
  _loader = OCMClassMock([LynxImageLoader class]);
  id service = OCMProtocolMock(@protocol(LynxServiceImageProtocol));
  OCMStub(ClassMethod([_loader imageService])).andReturn(service);
  NSMutableArray<LynxImageLoadOptions *> *serviceRequests = [NSMutableArray new];
  OCMStub([service loadImageWithOptions:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    __unsafe_unretained LynxImageLoadOptions *options = nil;
    [invocation getArgument:&options atIndex:2];
    [serviceRequests addObject:options];
  });
  id fetcher = OCMProtocolMock(@protocol(LynxImageFetcher));
  NSMutableArray<NSDictionary *> *customRequests = [NSMutableArray new];
  OCMStub([fetcher loadImageWithURL:[OCMArg any]
                         processors:[OCMArg any]
                               size:CGSizeMake(100, 80)
                        contextInfo:[OCMArg any]
                         completion:[OCMArg any]])
      .andDo(^(NSInvocation *invocation) {
        __unsafe_unretained NSDictionary *contextInfo = nil;
        [invocation getArgument:&contextInfo atIndex:5];
        [customRequests addObject:contextInfo];
      });
  LynxUIContext *uiContext =
      [[LynxUIContext alloc] initWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]];
  LynxContext *lynxContext = [LynxContext new];
  uiContext.lynxContext = lynxContext;
  _context.uiContext = uiContext;
  id mediaFetcher = OCMProtocolMock(@protocol(LynxMediaResourceFetcher));
  // legacy, media, hidden media, forced service, default service
  const bool cases[][5] = {{true, false, false, false, false},
                           {true, true, false, false, true},
                           {true, true, true, false, false},
                           {true, true, true, true, true},
                           {false, false, false, false, true}};
  int32_t key = 1;
  for (NSNumber *background in @[ @NO, @YES ]) {
    for (const auto &test : cases) {
      [serviceRequests removeAllObjects];
      [customRequests removeAllObjects];
      uiContext.imageFetcher = test[0] ? fetcher : nil;
      uiContext.mediaResourceFetcher = test[1] ? mediaFetcher : nil;
      lynxContext.hasCustomMediaFetcher = test[2];
      [uiContext setTrailUseNewImage:test[3]];
      [self createImage:key++ background:background.boolValue];
      XCTAssertEqual(serviceRequests.count, test[4] ? 2u : 0u);
      XCTAssertEqual(customRequests.count, test[4] ? 0u : 2u);
      NSDictionary *options = test[4] ? serviceRequests[0].contextInfo : customRequests[0];
      BOOL generic = test[1] && !test[2];
      XCTAssertEqualObjects(options[LynxEnableGenericFetcher], @(generic));
      XCTAssertEqualObjects(options[LynxShouldUseImageService], @(test[4]));
      XCTAssertEqualObjects(options[LynxImageSkipRedirection], @(test[4] && !generic));
      XCTAssertEqualObjects(options[LynxImagePreloadAllFrames], background);
    }
  }
}

- (void)testReplacementCancelsBothRequestsAndIgnoresLateCallbacks {
  [self createImage:1 background:NO];
  LynxImageManager *oldManager = [_context imageManagerForID:1];
  id oldManagerMock = OCMPartialMock(oldManager);
  OCMReject([oldManagerMock sendCustomEvent:[OCMArg any] withParams:[OCMArg any]]);
  UIImageView *view = [UIImageView new];
  [oldManager setTarget:view];
  [self createImage:2 background:NO];
  XCTAssertEqual(_cancelled.count, 2u);
  XCTAssertNil(view.image);
  UIImage *stale = [UIImage new];
  _requests[0].completed(stale, nil, _requests[0].imageURL.url);
  _requests[1].completed(stale, nil, _requests[1].imageURL.url);
  _requests[0].completed(nil, [NSError errorWithDomain:@"test" code:1 userInfo:nil],
                         _requests[0].imageURL.url);
  XCTAssertEqual([[oldManager valueForKey:@"images"] count], 0u);

  LynxImageManager *current = [_context imageManagerForID:2];
  [current setTarget:view];
  UIImage *fresh = [UIImage new];
  _requests[2].completed(fresh, nil, _requests[2].imageURL.url);
  XCTAssertEqual(view.image, fresh);
  [_context destroyImage:1];
  XCTAssertEqual([_context imageManagerForSign:7], current);
  XCTAssertEqual(_cancelled.count, 2u);
  [oldManagerMock stopMocking];
}

- (void)testSameManagerRejectsSupersededRequestIncludingEmptySource {
  LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
  LynxURL *url = [self url:@"https://example.com/image.png" type:LynxImageRequestSrc];
  [manager requestImage:url withType:LynxImageRequestSrc];
  [manager requestImage:url withType:LynxImageRequestSrc];
  XCTAssertEqual(_cancelled.count, 1u);
  _requests[0].completed([UIImage new], nil, url.url);
  XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
  [manager requestImage:nil withType:LynxImageRequestSrc];
  _requests[1].completed([UIImage new], nil, url.url);
  XCTAssertEqual(_cancelled.count, 2u);
  XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
}

- (void)testQueuedBackgroundCompletionIsIgnoredAfterDestroy {
  [self createImage:1 background:NO];
  LynxImageManager *manager = [_context imageManagerForID:1];
  [self runOnBackgroundThread:^{
    self->_requests[0].completed([UIImage new], nil, self -> _requests[0].imageURL.url);
  }];
  [_context destroyImage:1];
  XCTestExpectation *finished = [self expectationWithDescription:@"queued completion"];
  dispatch_async(dispatch_get_main_queue(), ^{
    XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
    [finished fulfill];
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

- (void)testAcceptedResultFinishesBeforeBackgroundReplacementCleanup {
  [self createImage:1 background:NO];
  LynxImageManager *oldManager = [_context imageManagerForID:1];
  UIImageView *oldView = [UIImageView new];
  [oldManager setTarget:oldView];
  id service = OCMProtocolMock(@protocol(LynxServiceImageProtocol));
  OCMStub(ClassMethod([_loader imageService])).andReturn(service);
  __block BOOL replaced = NO;
  __weak typeof(self) weakSelf = self;
  OCMStub([service isAnimatedImage:[OCMArg any]]).andDo(^(NSInvocation *invocation) {
    typeof(self) self = weakSelf;
    if (!replaced) {
      replaced = YES;
      // applyImage has passed the main-thread generation check. Replacing from
      // another thread must not deadlock or redirect the accepted result.
      [self runOnBackgroundThread:^{
        [self createImage:2 background:NO];
      }];
    }
  });
  UIImage *oldImage = [UIImage new];
  _requests[0].completed(oldImage, nil, _requests[0].imageURL.url);
  XCTAssertTrue(replaced);
  XCTAssertEqual(oldView.image, oldImage);
  XCTAssertEqual(_cancelled.count, 0u);
  UIImageView *newView = [UIImageView new];
  [[_context imageManagerForID:2] setTarget:newView];
  UIImage *newImage = [UIImage new];
  _requests[2].completed(newImage, nil, _requests[2].imageURL.url);
  XCTestExpectation *finished = [self expectationWithDescription:@"replacement cleanup"];
  dispatch_async(dispatch_get_main_queue(), ^{
    XCTAssertEqual(self->_cancelled.count, 1u);
    self->_requests[0].completed([UIImage new], nil, self -> _requests[0].imageURL.url);
    XCTAssertEqual(oldView.image, oldImage);
    XCTAssertEqual(newView.image, newImage);
    [finished fulfill];
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

- (void)testPendingCompletionDoesNotRetainManager {
  __weak LynxImageManager *weakManager;
  @autoreleasepool {
    LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
    weakManager = manager;
    [manager requestImage:[self url:@"https://example.com/image.png" type:LynxImageRequestSrc]
                 withType:LynxImageRequestSrc];
  }
  XCTAssertNil(weakManager);
  XCTAssertEqual(_cancelled.count, 1u);
  _requests[0].completed([UIImage new], nil, _requests[0].imageURL.url);
}

- (void)testBackgroundSynchronousCompletionDoesNotLeaveCancellationBlock {
  _completeSynchronously = YES;
  __block LynxImageManager *manager;
  [self runOnBackgroundThread:^{
    [self createImage:1 background:NO];
    manager = [self->_context imageManagerForID:1];
  }];
  [_context destroyImage:1];
  XCTAssertEqual(_cancelled.count, 0u);
  XCTestExpectation *finished = [self expectationWithDescription:@"stale synchronous completion"];
  dispatch_async(dispatch_get_main_queue(), ^{
    XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
    [finished fulfill];
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

- (void)testDestroyBeforeBackgroundLoaderReturnsCancelsReturnedRequest {
  LynxImageManager *manager = [[LynxImageManager alloc] initWithContext:nil];
  dispatch_semaphore_t destroyed = dispatch_semaphore_create(0);
  XCTestExpectation *finished = [self expectationWithDescription:@"loader returned after reset"];
  _beforeLoaderReturns = ^{
    dispatch_async(dispatch_get_main_queue(), ^{
      [manager reset];
      dispatch_semaphore_signal(destroyed);
    });
    XCTAssertEqual(
        dispatch_semaphore_wait(destroyed, dispatch_time(DISPATCH_TIME_NOW, 5 * NSEC_PER_SEC)), 0);
  };
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    [manager requestImage:[self url:@"https://example.com/image.png" type:LynxImageRequestSrc]
                 withType:LynxImageRequestSrc];
    dispatch_async(dispatch_get_main_queue(), ^{
      XCTAssertEqual(self->_cancelled.count, 1u);
      XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
      [finished fulfill];
    });
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

- (void)testElementThreadDestructionCancelsOnMainThread {
  XCTestExpectation *finished = [self expectationWithDescription:@"image lifecycle"];
  dispatch_async(dispatch_get_global_queue(QOS_CLASS_DEFAULT, 0), ^{
    [self createImage:1 background:NO];
    [self->_context destroyImage:1];
    dispatch_async(dispatch_get_main_queue(), ^{
      XCTAssertEqual(self->_requests.count, 2u);
      XCTAssertEqualObjects(self->_requestThreads, (@[ @NO, @NO ]));
      XCTAssertEqual(self->_cancelled.count, 2u);
      XCTAssertNil([self->_context imageManagerForID:1]);
      [finished fulfill];
    });
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

@end
