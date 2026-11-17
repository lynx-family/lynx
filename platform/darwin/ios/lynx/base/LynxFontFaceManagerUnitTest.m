// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFontFaceManager.h>
#import <XCTest/XCTest.h>

@interface LynxFontFaceManager (Testing)
- (NSString *)cachedKey:(NSString *)key;
- (bool)onRequestStartForItem:(LynxFontSrcItem *)item
                        cache:(NSString *)cacheKey
             fontFaceObserver:(id<LynxFontFaceObserver>)observer
              didRequestAsync:(BOOL *)didRequestAsync;
@end

@interface LynxFontFaceRaceTestManager : LynxFontFaceManager
@property(nonatomic, copy) void (^beforeRequestStart)(NSString *cacheKey);
@property(nonatomic, assign) NSUInteger requestStartCount;
@end

@implementation LynxFontFaceRaceTestManager
- (bool)onRequestStartForItem:(LynxFontSrcItem *)item
                        cache:(NSString *)cacheKey
             fontFaceObserver:(id<LynxFontFaceObserver>)observer
              didRequestAsync:(BOOL *)didRequestAsync {
  self.requestStartCount++;
  if (self.beforeRequestStart) {
    self.beforeRequestStart(cacheKey);
  }
  return [super onRequestStartForItem:item
                                cache:cacheKey
                     fontFaceObserver:observer
                      didRequestAsync:didRequestAsync];
}

// Use an installed font so tests can exercise notification delivery without a font asset.
- (NSString *)registerFontWithData:(NSData *)data
                      withCacheKey:(NSString *)cacheKey
                          lynxView:(UIView<LUIBodyView> *)lynxView {
  return @"Courier-Bold";
}
@end

@interface LynxFontFaceManagerUnitTest : XCTestCase <LynxFontFaceObserver, LynxResourceFetcher>
@property(nonatomic, strong) LynxFontFaceRaceTestManager *manager;
@property(nonatomic, strong) NSMutableDictionary *fontCache;
@property(nonatomic, strong) LynxFontFaceContext *context;
@property(nonatomic, strong) LynxFontSrcItem *item;
@property(nonatomic, copy) NSString *cacheKey;
@property(nonatomic, assign) NSUInteger notificationCount;
@end

@implementation LynxFontFaceManagerUnitTest
- (void)setUp {
  [super setUp];
  self.manager = [LynxFontFaceRaceTestManager new];
  self.fontCache = [self.manager valueForKey:@"registedFontMap"];
  self.context = [LynxFontFaceContext new];
  // The request is intercepted before decoding; no font file or network is needed.
  NSString *dataURL = [@"data:font/ttf;base64," stringByPaddingToLength:128
                                                             withString:@"A"
                                                        startingAtIndex:0];
  LynxFontFace *face =
      [[LynxFontFace alloc] initWithFamilyName:@"RaceTestFont"
                                        andSrc:[NSString stringWithFormat:@"url('%@')", dataURL]
                               withLynxContext:nil];
  [self.context addFontFace:face];
  self.item = [face srcAtIndex:0];
  self.cacheKey = [self.manager cachedKey:dataURL];
  self.notificationCount = 0;
}

- (void)tearDown {
  self.manager.beforeRequestStart = nil;
  self.manager = nil;
  self.fontCache = nil;
  self.context = nil;
  self.item = nil;
  [super tearDown];
}

- (void)onFontFaceLoad {
  self.notificationCount++;
}

- (UIFont *)generateFont:(BOOL *)didRequestAsync {
  return [self.manager generateFontWithSize:16
                                     weight:UIFontWeightRegular
                                      style:LynxFontStyleNormal
                             fontFamilyName:@"RaceTestFont"
                            fontFaceContext:self.context
                           fontFaceObserver:self
                            didRequestAsync:didRequestAsync];
}

- (LynxFontSrcItem *)startPendingRequest {
  LynxFontSrcItem *firstItem = [LynxFontSrcItem new];
  firstItem.src = self.item.src;
  XCTAssertTrue([self.manager onRequestStartForItem:firstItem
                                              cache:self.cacheKey
                                   fontFaceObserver:nil
                                    didRequestAsync:NULL]);
  self.manager.requestStartCount = 0;
  return firstItem;
}

- (void)testFontCompletesBetweenCacheLookupAndSubscription {
  LynxFontSrcItem *firstItem = [self startPendingRequest];
  NSMutableDictionary *cache = self.fontCache;
  self.manager.beforeRequestStart = ^(NSString *key) {
    // Simulate another context finishing after generateFont's initial cache lookup,
    // but before this item subscribes. Use an installed font to check the actual UIFont.
    @synchronized(cache) {
      firstItem.dataFontName = @"Courier-Bold";
      firstItem.src = nil;
      cache[key] = firstItem.dataFontName;
    }
  };

  BOOL didRequestAsync = NO;
  UIFont *font = [self generateFont:&didRequestAsync];

  XCTAssertEqual(self.manager.requestStartCount, 1u);
  XCTAssertEqualObjects(self.item.dataFontName, @"Courier-Bold");
  XCTAssertEqualObjects(font.fontName, @"Courier-Bold");
  XCTAssertFalse(didRequestAsync);
  XCTAssertEqual(self.notificationCount, 0u);
  XCTAssertEqual(firstItem.notifierArray.count, 0u);
}

- (void)testReadyCacheUsesFontWithoutStartingRequest {
  self.fontCache[self.cacheKey] = @"Courier-Bold";
  BOOL didRequestAsync = NO;
  UIFont *font = [self generateFont:&didRequestAsync];

  XCTAssertEqualObjects(font.fontName, @"Courier-Bold");
  XCTAssertEqualObjects(self.item.dataFontName, @"Courier-Bold");
  XCTAssertEqual(self.manager.requestStartCount, 0u);
  XCTAssertFalse(didRequestAsync);
  XCTAssertEqual(self.notificationCount, 0u);
}

- (void)testPendingRequestKeepsObserverWithoutStartingAnotherLoad {
  LynxFontSrcItem *firstItem = [self startPendingRequest];
  BOOL didRequestAsync = NO;
  [self generateFont:&didRequestAsync];

  XCTAssertTrue(didRequestAsync);
  XCTAssertNil(self.item.dataFontName);
  XCTAssertEqual(self.manager.requestStartCount, 1u);
  XCTAssertTrue([self.fontCache[self.cacheKey] containsObject:self.item]);
  XCTAssertEqual(firstItem.notifierArray.count, 1u);
  XCTAssertEqual([firstItem.notifierArray pointerAtIndex:0], (__bridge void *)self);
  XCTAssertEqual(self.notificationCount, 0u);
}

- (dispatch_block_t)loadResourceWithURL:(NSURL *)url
                                   type:(LynxFetchResType)type
                             completion:(LynxResourceLoadCompletionBlock)completion {
  completion(YES, [@"font" dataUsingEncoding:NSUTF8StringEncoding], nil, url);
  return nil;
}

- (void)testSynchronousFetcherKeepsQueuedNotification {
  self.item.src = @"file:///test-font.ttf";
  self.context.resourceFetcher = self;

  BOOL didRequestAsync = NO;
  UIFont *font = [self generateFont:&didRequestAsync];
  XCTAssertEqualObjects(font.fontName, @"Courier-Bold");
  XCTAssertTrue(didRequestAsync);

  XCTestExpectation *notified = [self expectationWithDescription:@"font notification delivered"];
  dispatch_async(dispatch_get_main_queue(), ^{
    XCTAssertEqual(self.notificationCount, 1u);
    [notified fulfill];
  });
  [self waitForExpectations:@[ notified ] timeout:5];
}

- (void)testReadyRequestCopiesFontNameWithoutObserver {
  self.fontCache[self.cacheKey] = @"Courier-Bold";

  XCTAssertFalse([self.manager onRequestStartForItem:self.item
                                               cache:self.cacheKey
                                    fontFaceObserver:nil
                                     didRequestAsync:NULL]);
  XCTAssertEqualObjects(self.item.dataFontName, @"Courier-Bold");
  XCTAssertNil(self.item.notifierArray);
}
@end
