// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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

- (void)testResizeReuseExcludesEveryCustomFetcher {
  LynxUIContext *uiContext =
      [[LynxUIContext alloc] initWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]];
  LynxContext *lynxContext = [LynxContext new];
  uiContext.lynxContext = lynxContext;
  _context.uiContext = uiContext;
  [self createImage:1 background:NO];
  LynxImageManager *manager = [_context imageManagerForID:1];
  XCTAssertTrue([manager canReuseOnResize]);
  id fetcher = OCMProtocolMock(@protocol(LynxImageFetcher));
  uiContext.imageFetcher = fetcher;
  XCTAssertFalse([manager canReuseOnResize]);
  uiContext.imageFetcher = nil;
  uiContext.mediaResourceFetcher = OCMProtocolMock(@protocol(LynxMediaResourceFetcher));
  XCTAssertFalse([manager canReuseOnResize]);
  lynxContext.hasCustomMediaFetcher = YES;
  XCTAssertNil(uiContext.mediaResourceFetcher);
  XCTAssertFalse([manager canReuseOnResize]);
  lynxContext.hasCustomMediaFetcher = NO;
  uiContext.mediaResourceFetcher = nil;
  XCTAssertTrue([manager canReuseOnResize]);
  [_context destroyImage:1];
  XCTAssertFalse([manager canReuseOnResize]);
}

- (void)testResizeRetriesFailedRequestsButReusesPendingAndSuccessfulRequests {
  [self createImage:1 background:NO];
  LynxImageManager *manager = [_context imageManagerForID:1];
  XCTAssertTrue([manager canReuseOnResize]);
  _requests[0].completed([UIImage new], nil, _requests[0].imageURL.url);
  XCTAssertTrue([manager canReuseOnResize]);
  [self runOnBackgroundThread:^{
    self->_requests[1].completed(nil, [NSError errorWithDomain:@"test" code:1 userInfo:nil],
                                 self -> _requests[1].imageURL.url);
  }];
  XCTAssertFalse([manager canReuseOnResize]);
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

- (void)testZeroSizeWaitsForLayoutButSingleDimensionCanLoad {
  lynx::tasm::ImagePaintInfo info;
  [self createImage:1 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 0u);
  [self createImage:2 size:CGSizeMake(100, 80) paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 2u);
  [self createImage:3 size:CGSizeMake(0, 80) paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 4u);
  info.auto_size = true;
  [self createImage:4 size:CGSizeMake(100, 0) paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 6u);
  [self createImage:5 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 6u);
  [self createImage:6 size:CGSizeZero paintInfo:info background:YES];
  XCTAssertEqual(_requests.count, 8u);
}

- (void)testPrefetchSizeAndAutoSizeBoundaries {
  LynxScreenMetrics *metrics = [[LynxScreenMetrics alloc] initWithScreenSize:CGSizeMake(390, 844)
                                                                       scale:2];
  _context.uiContext = [[LynxUIContext alloc] initWithScreenMetrics:metrics];
  lynx::tasm::ImagePaintInfo info;
  info.prefetch_width = "120px";
  info.prefetch_height = "80px";
  [self createImage:1 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 2u);
  XCTAssertTrue(CGSizeEqualToSize(_requests[0].targetSize, CGSizeMake(120, 80)));
  XCTAssertTrue(CGSizeEqualToSize(_requests[1].targetSize, CGSizeMake(120, 80)));
  [self createImage:2 size:CGSizeMake(100, 60) paintInfo:info background:NO];
  XCTAssertTrue(CGSizeEqualToSize(_requests[2].targetSize, CGSizeMake(100, 60)));
  info.auto_size = true;
  [self createImage:3 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 4u);
  info.auto_size = false;
  info.prefetch_width = "";
  [self createImage:4 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 6u);
  info.prefetch_height = "-1px";
  [self createImage:5 size:CGSizeZero paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 6u);
}

- (void)testAutoSizeCompletionUsesLayoutInsteadOfPrefetchSize {
  LynxUIContext *uiContext =
      [[LynxUIContext alloc] initWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]];
  _context.uiContext = uiContext;
  id contextMock = OCMPartialMock(uiContext);
  LynxImageShadowNode *node = [[LynxImageShadowNode alloc] initWithSign:7 tagName:@"image"];
  id nodeMock = OCMPartialMock(node);
  OCMExpect([nodeMock setNeedsLayout]);
  OCMExpect([contextMock findShadowNodeAndRunTask:7 task:[OCMArg any]])
      .andDo(^(NSInvocation *invocation) {
        __unsafe_unretained void (^task)(LynxShadowNode *) = nil;
        [invocation getArgument:&task atIndex:3];
        task(node);
      });

  lynx::tasm::ImagePaintInfo info;
  info.auto_size = true;
  info.prefetch_width = "120px";
  info.prefetch_height = "80px";
  [self createImage:1 size:CGSizeMake(100, 0) paintInfo:info background:NO];
  XCTAssertEqual(_requests.count, 2u);
  XCTAssertTrue(CGSizeEqualToSize(_requests[0].targetSize, CGSizeMake(120, 80)));
  UIImage *image = OCMClassMock(UIImage.class);
  OCMStub([image size]).andReturn(CGSizeMake(600, 400));
  _requests[0].completed(image, nil, _requests[0].imageURL.url);
  CGSize measured = [(id<LynxMeasureDelegate>)node measureNode:node
                                                     withWidth:100
                                                     widthMode:LynxMeasureModeDefinite
                                                        height:0
                                                    heightMode:LynxMeasureModeIndefinite];
  XCTAssertEqualWithAccuracy(measured.width, 100, 0.001);
  XCTAssertEqualWithAccuracy(measured.height, 100.0 * 400.0 / 600.0, 0.001);
  OCMVerifyAll(contextMock);
  OCMVerifyAll(nodeMock);
  [(id)image stopMocking];
  [nodeMock stopMocking];
  [contextMock stopMocking];
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
  OCMExpect([nodeMock setImageSize:CGSizeMake(600, 400)]);
  tasks[1](node);
  OCMVerifyAll(nodeMock);
  XCTAssertNotNil(oldManager);
  [nodeMock stopMocking];
  [(id)image stopMocking];
  [contextMock stopMocking];
}

- (void)testExplicitRequestOptionsPreserveDefaultsAndBackgrounds {
  _context.uiContext =
      [[LynxUIContext alloc] initWithScreenMetrics:[LynxScreenMetrics getDefaultLynxScreenMetrics]];
  _context.uiContext.fixNewImageDownSampling = NO;
  [self createImage:1 background:NO];
  XCTAssertNil(_requests[0].contextInfo[LynxImageFetcherContextKeyDownsampling]);
  XCTAssertNil(_requests[0].contextInfo[LynxImageRequestPriority]);
  lynx::tasm::ImagePaintInfo info;
  info.downsampling = true;
  info.request_priority = "high";
  [self createImage:2 size:CGSizeMake(100, 80) paintInfo:info background:NO];
  XCTAssertEqualObjects(_requests[2].contextInfo[LynxImageFetcherContextKeyDownsampling], @YES);
  XCTAssertEqualObjects(_requests[3].contextInfo[LynxImageFetcherContextKeyDownsampling], @YES);
  XCTAssertEqualObjects(_requests[2].contextInfo[LynxImageFixNewImageDownsampling], @NO);
  XCTAssertEqualObjects(_requests[2].contextInfo[LynxImageRequestPriority], @"high");
  _context.uiContext.fixNewImageDownSampling = YES;
  info.request_priority = "low";
  [self createImage:3 size:CGSizeMake(100, 80) paintInfo:info background:NO];
  XCTAssertEqualObjects(_requests[4].contextInfo[LynxImageFixNewImageDownsampling], @YES);
  XCTAssertEqualObjects(_requests[4].contextInfo[LynxImageRequestPriority], @"low");
  info.auto_size = true;
  [self createImage:4 size:CGSizeMake(100, 0) paintInfo:info background:NO];
  XCTAssertNil(_requests[6].contextInfo[LynxImageFetcherContextKeyDownsampling]);
  [self createImage:5 size:CGSizeMake(100, 80) paintInfo:info background:YES];
  XCTAssertNil(_requests[8].contextInfo[LynxImageFetcherContextKeyDownsampling]);
  XCTAssertNil(_requests[8].contextInfo[LynxImageRequestPriority]);
  XCTAssertEqualObjects(_requests[8].contextInfo[LynxImagePreloadAllFrames], @YES);
}

- (void)testPlaceholderWithoutSourceDoesNotEmitSourceLoad {
  lynx::tasm::ImagePaintInfo info;
  [_context createImageManager:7
                 withSourceURL:nil
             andPlaceholderURL:[self url:@"https://example.com/placeholder.png"
                                    type:LynxImageRequestPlaceholder]
                     paintInfo:info
                     eventMask:3
                      imageKey:1];
  XCTAssertEqual(_requests.count, 1u);
  LynxImageManager *manager = [_context imageManagerForID:1];
  id managerMock = OCMPartialMock(manager);
  OCMReject([managerMock sendCustomEvent:[OCMArg any] withParams:[OCMArg any]]);
  UIImageView *view = [UIImageView new];
  [manager setTarget:view];
  UIImage *placeholder = [UIImage new];
  _requests[0].completed(placeholder, nil, _requests[0].imageURL.url);
  XCTAssertEqual(view.image, placeholder);
  [managerMock stopMocking];
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
  int32_t key = 1;
  for (NSNumber *background in @[ @NO, @YES ]) {
    [serviceRequests removeAllObjects];
    [customRequests removeAllObjects];
    lynxContext.hasCustomMediaFetcher = NO;
    uiContext.imageFetcher = fetcher;
    uiContext.mediaResourceFetcher = nil;
    [uiContext setTrailUseNewImage:NO];
    [self createImage:key++ background:background.boolValue];
    XCTAssertEqual(customRequests.count, 2u);
    XCTAssertEqual(serviceRequests.count, 0u);
    XCTAssertEqualObjects(customRequests[0][LynxEnableGenericFetcher], @NO);
    XCTAssertEqualObjects(customRequests[0][LynxShouldUseImageService], @NO);
    XCTAssertEqualObjects(customRequests[0][LynxImageSkipRedirection], @NO);
    XCTAssertEqualObjects(customRequests[0][LynxImagePreloadAllFrames], background);

    uiContext.mediaResourceFetcher = OCMProtocolMock(@protocol(LynxMediaResourceFetcher));
    [self createImage:key++ background:background.boolValue];
    XCTAssertEqual(customRequests.count, 2u);
    XCTAssertEqual(serviceRequests.count, 2u);
    XCTAssertEqualObjects(serviceRequests[0].contextInfo[LynxEnableGenericFetcher], @YES);
    XCTAssertEqualObjects(serviceRequests[0].contextInfo[LynxImageSkipRedirection], @NO);
    XCTAssertEqualObjects(serviceRequests[0].contextInfo[LynxImagePreloadAllFrames], background);

    // Keep the injected fetcher, but exercise the real context getter's gate.
    lynxContext.hasCustomMediaFetcher = YES;
    XCTAssertNil(uiContext.mediaResourceFetcher);
    [self createImage:key++ background:background.boolValue];
    XCTAssertEqual(customRequests.count, 4u);
    XCTAssertEqual(serviceRequests.count, 2u);
    XCTAssertEqualObjects(customRequests[2][LynxEnableGenericFetcher], @NO);
    XCTAssertEqualObjects(customRequests[2][LynxImageSkipRedirection], @NO);

    [uiContext setTrailUseNewImage:YES];
    [self createImage:key++ background:background.boolValue];
    XCTAssertEqual(serviceRequests.count, 4u);
    XCTAssertEqualObjects(serviceRequests[2].contextInfo[LynxEnableGenericFetcher], @NO);
    XCTAssertEqualObjects(serviceRequests[2].contextInfo[LynxImageSkipRedirection], @YES);
    uiContext.mediaResourceFetcher = nil;
    lynxContext.hasCustomMediaFetcher = NO;
    [uiContext setTrailUseNewImage:NO];
    uiContext.imageFetcher = nil;
    [self createImage:key++ background:background.boolValue];
    XCTAssertEqual(serviceRequests.count, 6u);
    XCTAssertEqual(customRequests.count, 4u);
    XCTAssertEqualObjects(serviceRequests[4].contextInfo[LynxEnableGenericFetcher], @NO);
    XCTAssertEqualObjects(serviceRequests[4].contextInfo[LynxImageSkipRedirection], @YES);
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

- (void)testDestroyInvalidatesPendingSourceAndPlaceholder {
  [self createImage:1 background:NO];
  LynxImageManager *manager = [_context imageManagerForID:1];
  [_context destroyImage:1];
  XCTAssertEqual(_cancelled.count, 2u);
  XCTAssertNil([_context imageManagerForSign:7]);
  _requests[0].completed([UIImage new], nil, _requests[0].imageURL.url);
  _requests[1].completed([UIImage new], nil, _requests[1].imageURL.url);
  XCTAssertEqual([[manager valueForKey:@"images"] count], 0u);
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

- (void)testSynchronousCompletionDoesNotLeaveCancellationBlock {
  _completeSynchronously = YES;
  [self createImage:1 background:NO];
  [_context destroyImage:1];
  XCTAssertEqual(_cancelled.count, 0u);
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

- (void)testElementThreadStartsLoadingWithoutWaitingForMainThread {
  [self runOnBackgroundThread:^{
    [self createImage:1 background:NO];
  }];
  XCTAssertEqual(_requests.count, 2u);
  XCTAssertEqualObjects(_requestThreads, (@[ @NO, @NO ]));
  [_context destroyImage:1];
  XCTAssertEqual(_cancelled.count, 2u);
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
      XCTAssertEqual(self->_cancelled.count, 2u);
      XCTAssertNil([self->_context imageManagerForID:1]);
      [finished fulfill];
    });
  });
  [self waitForExpectations:@[ finished ] timeout:5];
}

@end
