// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "TestBenchActionManager.h"
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxResourceProvider.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateBundleOption.h>
#import <Lynx/third_party/zlib/zlib.h>
#import <LynxDevtool/LynxInspectorOwner.h>
#import "TestBenchDynamicComponentFetcher.h"
#import "TestBenchEmulateTouchHelper.h"
#import "TestBenchEntranceViewController.h"
#import "TestBenchEnv.h"
#import "TestBenchOpenUrlModule.h"
#import "TestBenchReplayConfig.h"
#import "TestBenchReplayDataModule.h"
#import "TestBenchViewClient.h"

#define SCREEN_WIDTH [UIScreen mainScreen].bounds.size.width
#define SCREEN_HEIGHT [UIScreen mainScreen].bounds.size.height

static const int kVirtual = 1 << 2;

// record some action, which must be called again when page reload
@interface ReloadAction : NSObject
@property NSInteger interval;
@property NSString* functionName;
@property NSDictionary* params;
- (id)initWithParams:(NSInteger)interval
            funcName:(NSString*)functionName
              params:(NSDictionary*)params;
@end

@implementation ReloadAction
- (id)initWithParams:(NSInteger)interval
            funcName:(NSString*)functionName
              params:(NSDictionary*)params {
  if (self = [super init]) {
    _interval = interval;
    _functionName = functionName;
    _params = params;
  }
  return self;
}
@end

@interface TestBenchActionManager ()
@property UIView* parentUI;
@property CGPoint origin;
@property NSData* source;
@property NSData* preloadedSource;
@property int64_t startTime;
@property NSMutableArray* moduleList;
@property(nonnull) LynxView* lynxView;
@property LynxTheme* lynxThemeCache;
@property NSDictionary* threadStrategyData;
@property NSDictionary* globalPropsCache;
@property NSArray<NSDictionary*>* componentList;
@property NSMutableArray* reloadFuncList;
@property NSString* loadTemplateUrl;
@property LynxTemplateData* loadTemplateInitData;
@property TestBenchViewClient* client;
@property TestBenchStateReplayView* stateView;
@property NSDictionary* config;
@property BOOL hasLoadTemplate;
@property TestBenchReplayConfig* replayConfig;
@property TestBenchDynamicComponentFetcher* dynamicComponentFetcher;
@property BOOL isSSRLoaded;
@property float rawFontScaleValue;
@property LynxTemplateBundle* templateBundle;
@property LynxTemplateBundleOption* templateBundleOption;
@property NSDictionary* templateBundleParams;
@property(nonatomic, strong) NSMutableArray* actionCallbacks;

@end

@implementation TestBenchActionManager

- (id)init {
  if (self = [super init]) {
    self.actionCallbacks = [NSMutableArray array];
  }

  return self;
}

- (NSData*)zlibDeflate:(NSData*)compressedData {
  if ([compressedData length] == 0) return nil;

  NSUInteger fullLength = [compressedData length];
  NSUInteger halfLength = [compressedData length] / 2;

  NSMutableData* decompressed = [NSMutableData dataWithLength:fullLength + halfLength];
  BOOL done = NO;
  int status;

  z_stream strm;
  strm.next_in = (Bytef*)[compressedData bytes];
  strm.avail_in = (unsigned)[compressedData length];
  strm.total_out = 0;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;

  if (inflateInit(&strm) != Z_OK) {
    return nil;
  }
  while (!done) {
    if (strm.total_out >= [decompressed length]) {
      [decompressed increaseLengthBy:halfLength];
    }
    strm.next_out = (Bytef*)[decompressed mutableBytes] + strm.total_out;
    strm.avail_out = (uint)([decompressed length] - strm.total_out);
    status = inflate(&strm, Z_SYNC_FLUSH);
    if (status == Z_STREAM_END) {
      done = YES;
    } else {
      if (status != Z_OK) {
        break;
      }
    }
  }
  if (inflateEnd(&strm) != Z_OK) {
    return nil;
  }
  if (done) {
    [decompressed setLength:strm.total_out];
    return [NSData dataWithData:decompressed];
  } else {
    return nil;
  }
}

- (void)loadRecordFile {
  NSURLSessionConfiguration* configuration =
      [NSURLSessionConfiguration defaultSessionConfiguration];
  configuration.requestCachePolicy = self.replayConfig.requestCachePolicy;
  NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration];
  __weak typeof(self) _self = self;
  NSURLSessionDataTask* dataTask =
      [session dataTaskWithURL:[NSURL URLWithString:[self.replayConfig url]]
             completionHandler:^(NSData* _Nullable data, NSURLResponse* _Nullable response,
                                 NSError* _Nullable error) {
               __strong typeof(_self) strongSelf = _self;
               [strongSelf.stateView setReplayState:PARSING_JSON_FILE];
               [strongSelf handleRecordFileData:data];
             }];
  [dataTask resume];
}

- (void)registerTestBenchActionCallback:(id<TestBenchActionCallback>)callback {
  [self.actionCallbacks addObject:callback];
}

- (void)startWithUrl:(NSString*)url
              inView:(UIView*)parentView
          withOrigin:(CGPoint)point
           stateView:(nonnull TestBenchStateReplayView*)stateView
        replayConfig:(TestBenchReplayConfig*)replayConfig {
  [self setReplayConfig:replayConfig];
  _parentUI = parentView;
  _origin = point;
  _stateView = stateView;
  _startTime = 0;
  _hasLoadTemplate = NO;

  _threadStrategyData = nil;
  _templateBundle = nil;
  _templateBundleOption = [[LynxTemplateBundleOption alloc] init];
  [_templateBundleOption setContextPoolSize:5];
  [_templateBundleOption setEnableContextAutoRefill:true];
  _templateBundleParams = nil;
  _lynxView = nil;
  _lynxThemeCache = nil;
  _config = nil;
  _rawFontScaleValue = -1;
  _reloadFuncList = [[NSMutableArray alloc] init];

  _loadTemplateUrl = nil;
  _loadTemplateInitData = nil;
  [_stateView setReplayState:DOWNLOAD_JSON_FILE];
  _dynamicComponentFetcher = [[TestBenchDynamicComponentFetcher alloc] init];

  if ([self.replayConfig sourceUrl] != nil) {
    NSURLSessionConfiguration* configuration =
        [NSURLSessionConfiguration defaultSessionConfiguration];
    configuration.requestCachePolicy = self.replayConfig.requestCachePolicy;
    NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration];
    __weak typeof(self) _self = self;
    NSURLSessionDataTask* dataTask =
        [session dataTaskWithURL:[NSURL URLWithString:[self.replayConfig sourceUrl]]
               completionHandler:^(NSData* _Nullable data, NSURLResponse* _Nullable response,
                                   NSError* _Nullable error) {
                 if (error != nil) {
                   NSLog(@"[TestBench] Template.js(%@) is unavailable!: %@",
                         [[error userInfo] objectForKey:@"NSErrorFailingURLKey"],
                         [[error userInfo] objectForKey:@"NSLocalizedDescription"]);
                   return;
                 }
                 __strong typeof(_self) strongSelf = _self;
                 [strongSelf setPreloadedSource:data];
                 [strongSelf downloadRecordFile];
               }];
    [dataTask resume];
  } else {
    [self downloadRecordFile];
  }
}

- (void)downloadRecordFile {
  if ([TestBenchEntranceViewController enableLoadLocalRecordFile:[self.replayConfig url]]) {
    [self loadLocalRecordFile];
  } else {
    [self loadRecordFile];
  }
}

- (BOOL)checkRecordFile:(NSArray*)actionList {
  for (int index = 0; index < actionList.count; ++index) {
    NSDictionary* obj = actionList[index];
    if ([obj[@"Function Name"] isEqual:@"loadTemplate"] ||
        [obj[@"Function Name"] isEqual:@"loadTemplateBundle"]) {
      _templateBundleParams = obj[@"Params"];
      return true;
    }
  }
  return false;
}

- (void)loadLocalRecordFile {
  [self.stateView setReplayState:PARSING_JSON_FILE];
  NSData* data = [TestBenchEntranceViewController localRecordFileData:[self.replayConfig url]];
  [self handleRecordFileData:data];
}

- (void)handleRecordFileData:(NSData*)data {
  NSString* encodedString = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
  NSError* jsonError;
  NSDictionary* json = nil;
  if ([encodedString hasPrefix:@"{"]) {
    json = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
  } else {
    NSData* decodedData =
        [[NSData alloc] initWithBase64EncodedString:encodedString
                                            options:NSDataBase64DecodingIgnoreUnknownCharacters];
    NSData* unZipData = [self zlibDeflate:decodedData];
    json = [NSJSONSerialization JSONObjectWithData:unZipData options:0 error:&jsonError];
    if (jsonError != nil) {
      // NSCocoaErrorDomain caused by invalid json parsing
      if (jsonError.code == 3840) {
        jsonError = nil;
        NSString* tempString = [[NSString alloc] initWithData:unZipData
                                                     encoding:NSISOLatin1StringEncoding];
        NSData* unZipDataUTF8 = [tempString dataUsingEncoding:NSUTF8StringEncoding];
        json = [NSJSONSerialization JSONObjectWithData:unZipDataUTF8 options:0 error:&jsonError];
      }
    }
  }
  if (jsonError != nil) {
    [self.stateView setReplayState:INVALID_JSON_FILE];
    return;
  }
  self.moduleList = [NSMutableArray new];
  if ([json objectForKey:@"Config"] && json[@"Config"] != [NSNull null]) {
    _config = json[@"Config"];
    // Set jsb ignored info
    if ([_config objectForKey:@"jsbIgnoredInfo"]) {
      [TestBenchReplayDataModule setJSbIgnoredInfo:[_config objectForKey:@"jsbIgnoredInfo"]];
    }

    if ([_config objectForKey:@"jsbSettings"]) {
      [TestBenchReplayDataModule setJsbSettings:[_config objectForKey:@"jsbSettings"]];
    }
  }
  if ([json objectForKey:@"Invoked Method Data"]) {
    [self mockInvokeMethod:json[@"Invoked Method Data"]];
  }
  if ([json objectForKey:@"Callback"]) {
    [self mockCallBack:json[@"Callback"]];
  }
  if ([json objectForKey:@"Component List"]) {
    [self mockComponent:json[@"Component List"]];
  }
  if ([json objectForKey:@"Action List"]) {
    if ([self checkRecordFile:[json objectForKey:@"Action List"]]) {
      NSPredicate* predicate = [NSPredicate predicateWithFormat:@"SELF != %@", [NSNull null]];
      NSArray* actionList =
          [[json objectForKey:@"Action List"] filteredArrayUsingPredicate:predicate];
      [self.dynamicComponentFetcher parse:actionList];
      [self.stateView setReplayState:HANDLE_ACTION_LIST];
      [self handleActionList:actionList];
    } else {
      [self.stateView setReplayState:RECORD_ERROR_MISS_TEMPLATEJS];
    }
  }
}

- (void)runAction:(id)obj
    setReplayTimeCallback:(void (^)(int64_t))setReplayTimeCallback
      onTestBenchComplete:(void (^)(int64_t))onTestBenchComplete {
  NSString* functionName = obj[@"Function Name"];
  if ([functionName isEqualToString:@"updateViewPort"] ||
      [functionName isEqualToString:@"setThreadStrategy"]) {
    functionName = @"initialLynxView";
  }
  if ([functionName isEqualToString:@"loadTemplate"] && [self.replayConfig enablePreDecode]) {
    functionName = @"loadTemplateBundle";
  }
  if (![[self.replayConfig canMockFuncName] containsObject:functionName]) {
    return;
  }
  int64_t recordTime = [obj[@"Record Time"] doubleValue];
  if ([obj[@"RecordMillisecond"] intValue]) {
    recordTime = [obj[@"RecordMillisecond"] doubleValue];
  } else {
    recordTime = recordTime * 1000;
  }
  setReplayTimeCallback(recordTime);
  if ([functionName isEqualToString:@"sendEventDarwin"] && ![self.replayConfig replayGesture]) {
    return;
  }
  NSDictionary* params = obj[@"Params"];
  if (_startTime == 0) {
    _startTime = recordTime;
  }
  NSInteger interval = recordTime - _startTime;
  if ([[self.replayConfig reloadFuncName] containsObject:functionName]) {
    [_reloadFuncList addObject:[[ReloadAction alloc] initWithParams:interval
                                                           funcName:functionName
                                                             params:params]];
  } else {
    [self dispatchAction:functionName params:params interval:interval];
  }
  onTestBenchComplete(interval);
}

- (void)dispatchAction:(NSString*)functionName
                params:(NSDictionary*)params
              interval:(NSInteger)interval {
  SEL selector = NSSelectorFromString([functionName stringByAppendingString:@":"]);
  IMP imp = [self methodForSelector:selector];
  void (*func)(id, SEL, NSDictionary*) = (__typeof__(func))imp;

  __weak typeof(self) _self = self;
  if ([self respondsToSelector:selector]) {
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(interval * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
                     if (_self) {
                       __strong typeof(_self) strongSelf = _self;
                       func(strongSelf, selector, params);
                     }
                   });
  }
}

- (void)handleActionList:(NSArray*)actionList {
  __block BOOL hasSetReplayStartTime = NO;
  if ([self.replayConfig enablePreDecode]) {
    [self preDecodeTemplate:_templateBundleParams];
  }
  [actionList enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL* stop) {
    @try {
      [self runAction:obj
          setReplayTimeCallback:^(int64_t recordTime) {
            if (!hasSetReplayStartTime) {
              [TestBenchReplayDataModule setTime:recordTime];
              hasSetReplayStartTime = YES;
            }
          }
          onTestBenchComplete:^(int64_t interval) {
            if (idx == actionList.count - 1 && self.onTestBenchComplete) {
              __weak typeof(self) _self = self;
              dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(interval * NSEC_PER_MSEC)),
                             dispatch_get_main_queue(), ^{
                               if (_self) {
                                 __strong typeof(_self) strongSelf = _self;
                                 strongSelf.onTestBenchComplete();
                               }
                             });
            }
          }];
    } @catch (NSException* exception) {
      NSLog(@"[TestBench][HandleAction] exception in index %ld: %@", idx, exception);
    }
  }];
}

- (LynxThreadStrategyForRender)getThreadStrategy:(int32_t)schemaThreadStrategy
                         testBenchThreadStrategy:(int32_t)testBenchThreadStrategy {
  int32_t threadStrategy = testBenchThreadStrategy;
  // Prioritize thread strategy from schema over internal data
  if (schemaThreadStrategy >= LynxThreadStrategyForRenderAllOnUI &&
      schemaThreadStrategy <= LynxThreadStrategyForRenderMultiThreads) {
    threadStrategy = schemaThreadStrategy;
  }
  switch (threadStrategy) {
    case 1:
      return LynxThreadStrategyForRenderMostOnTASM;
    case 2:
      return LynxThreadStrategyForRenderPartOnLayout;
    case 3:
      return LynxThreadStrategyForRenderMultiThreads;
    default:
      return LynxThreadStrategyForRenderAllOnUI;
  }
}

- (LynxViewSizeMode)getViewSizeMode:(int)params {
  switch (params) {
    case 0:
      return LynxViewSizeModeUndefined;
    case 1:
      return LynxViewSizeModeExact;
    default:
      return LynxViewSizeModeMax;
  }
}

- (void)preDecodeTemplate:(NSDictionary*)params {
  NSString* source = params[@"source"];
  if (_preloadedSource) {
    _source = _preloadedSource;
  } else {
    _source =
        [[NSData alloc] initWithBase64EncodedString:source
                                            options:NSDataBase64DecodingIgnoreUnknownCharacters];
  }
  _templateBundle = [[LynxTemplateBundle alloc] initWithTemplate:[self source]
                                                          option:_templateBundleOption];
}

- (void)updateMetaData:(NSDictionary*)params {
  if (_lynxView == nil) {
    return;
  }
  NSDictionary* templateDataJson = params[@"templateData"];
  NSDictionary* globalPropsJson = params[@"global_props"];
  LynxUpdateMeta* meta = [[LynxUpdateMeta alloc] init];
  LynxTemplateData* templateData = nil;
  if (![templateDataJson isKindOfClass:[NSNull class]]) {
    templateData = [[LynxTemplateData alloc] initWithDictionary:templateDataJson[@"value"]];
    NSString* preprocessorName = [templateDataJson objectForKey:@"preprocessorName"];
    bool readOnly = [[templateDataJson objectForKey:@"readOnly"] boolValue];
    if (![@"" isEqualToString:preprocessorName]) {
      [templateData markState:preprocessorName];
    }
    if (readOnly) {
      [templateData markReadOnly];
    }
  }
  LynxTemplateData* globalProps = [[LynxTemplateData alloc] initWithDictionary:globalPropsJson];
  meta.data = templateData;
  meta.globalProps = globalProps;
  [_lynxView updateMetaData:meta];
}

- (void)initialLynxView:(NSDictionary*)params {
  if ([params objectForKey:@"threadStrategy"]) {
    _threadStrategyData = params;
    return;
  } else {
    if (!_threadStrategyData && _lynxView == nil) {
      return;
    }
  }

  CGFloat preferredLayoutHeight = [params[@"preferredLayoutHeight"] floatValue];
  CGFloat preferredLayoutWidth = [params[@"preferredLayoutWidth"] floatValue];
  CGFloat preferredMaxLayoutHeight = [params[@"preferredMaxLayoutHeight"] floatValue];
  CGFloat preferredMaxLayoutWidth = [params[@"preferredMaxLayoutWidth"] floatValue];

  LynxViewSizeMode widthMode = [self getViewSizeMode:[params[@"layoutWidthMode"] intValue]];
  LynxViewSizeMode heightMode = [self getViewSizeMode:[params[@"layoutHeightMode"] intValue]];

  if (_config != nil && [_config valueForKey:@"screenWidth"] != Nil &&
      [_config valueForKey:@"screenHeight"] != Nil) {
    CGFloat record_screen_width = [_config[@"screenWidth"] floatValue];
    CGFloat record_screen_height = [_config[@"screenHeight"] floatValue];
    if (record_screen_height != 0 && record_screen_width != 0) {
      preferredLayoutWidth = (preferredLayoutWidth / record_screen_width) * (SCREEN_WIDTH);
      preferredMaxLayoutWidth = (preferredMaxLayoutWidth / record_screen_width) * (SCREEN_WIDTH);
      preferredLayoutHeight = (preferredLayoutHeight / record_screen_height) * (SCREEN_HEIGHT);
      preferredMaxLayoutHeight =
          (preferredMaxLayoutHeight / record_screen_height) * (SCREEN_HEIGHT);
    }
  }

  __weak __typeof(self) weakSelf = self;
  if (_lynxView == nil) {
    _lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder* builder) {
      __strong __typeof(weakSelf) strongSelf = weakSelf;
      builder.config =
          [[LynxConfig alloc] initWithProvider:LynxConfig.globalConfig.templateProvider];
      [builder.config registerModule:[TestBenchReplayDataModule class]];
      [builder.config registerModule:[TestBenchOpenUrlModule class]];
      [builder
          setFrame:CGRectMake(strongSelf->_origin.x, strongSelf->_origin.y,
                              [UIScreen mainScreen].bounds.size.width - strongSelf->_origin.x,
                              [UIScreen mainScreen].bounds.size.height - strongSelf->_origin.y)];
      NSInteger schema_thread_strategy = [self.replayConfig thread_mode];
      [builder setThreadStrategyForRender:
                   [self getThreadStrategy:(int32_t)schema_thread_strategy
                       testBenchThreadStrategy:[strongSelf->_threadStrategyData[@"threadStrategy"]
                                                   intValue]]];
      if ([strongSelf->_threadStrategyData valueForKey:@"enableJSRuntime"]) {
        BOOL enableJSRuntime = [strongSelf->_threadStrategyData[@"enableJSRuntime"] boolValue] &&
                               ![_replayConfig enableAirStrictMode];
        [builder setEnableJSRuntime:enableJSRuntime];
        [builder setEnableAirStrictMode:!enableJSRuntime];
      }

      builder.fetcher = strongSelf->_dynamicComponentFetcher;

      NSString* filePath = [TestBenchReplayDataModule replayTimeEnvJScript];
      NSString* groupName = @"ark";
      if (strongSelf->_lynxGroup == nil) {
        LynxGroupOption* groupOption = [[LynxGroupOption alloc] init];
        groupOption.preloadJSPaths = @[ filePath ];
        strongSelf->_lynxGroup = [[LynxGroup alloc] initWithName:groupName
                                             withLynxGroupOption:groupOption];
      }
      builder.group = strongSelf->_lynxGroup;
      if (strongSelf->_rawFontScaleValue != -1) {
        builder.fontScale = strongSelf->_rawFontScaleValue;
      }
      // provide a change to register module or resource provider
      for (id<TestBenchActionCallback> callback in strongSelf.actionCallbacks) {
        [callback onLynxViewWillBuild:self builder:builder];
      }
    }];

    _lynxView.layoutWidthMode = widthMode;
    _lynxView.layoutHeightMode = heightMode;
    _lynxView.preferredLayoutWidth = preferredLayoutWidth;
    _lynxView.preferredLayoutHeight = preferredLayoutHeight;
    _lynxView.preferredMaxLayoutHeight = preferredMaxLayoutHeight;
    _lynxView.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
    [_parentUI addSubview:_lynxView];
    _threadStrategyData = nil;
    _client = [[TestBenchViewClient alloc] init];
    _client.manager = self;
    [_lynxView.getLifecycleDispatcher addLifecycleClient:_client];
    [_lynxView setResourceFetcher:_client];
    [_lynxView addObserver:self
                forKeyPath:@"frame"
                   options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
                   context:nil];
    // provide a change to register module or resource provider
    for (id<TestBenchActionCallback> callback in self.actionCallbacks) {
      [callback onLynxViewDidBuild:_lynxView];
    }
  } else {
    _lynxView.layoutWidthMode = widthMode;
    _lynxView.layoutHeightMode = heightMode;
    if (preferredLayoutWidth > [UIScreen mainScreen].bounds.size.width) {
      preferredLayoutWidth = [UIScreen mainScreen].bounds.size.width;
    }
    if ((preferredLayoutHeight > [UIScreen mainScreen].bounds.size.height) &&
        [self.replayConfig heightLimit]) {
      preferredLayoutHeight = [UIScreen mainScreen].bounds.size.height;
    }

    [_lynxView updateViewportWithPreferredLayoutWidth:preferredLayoutWidth
                                preferredLayoutHeight:preferredLayoutHeight];
  }
}

- (void)updateFontScale:(NSDictionary*)params {
  NSString* type = [params objectForKey:@"type"];
  float scale = [[params objectForKey:@"scale"] floatValue];
  if ([type isEqualToString:@"updateFontScale"] && _lynxView != nil) {
    [_lynxView updateFontScale:scale];
    return;
  } else {
    _rawFontScaleValue = scale;
  }
}

- (void)mockInvokeMethod:(NSArray*)methodDataList {
  [TestBenchReplayDataModule addFunctionCallArray:methodDataList];
}

- (void)mockCallBack:(NSDictionary*)callbackDictionary {
  [TestBenchReplayDataModule addCallbackDictionary:callbackDictionary];
}

- (void)mockComponent:(NSArray*)componentList {
  _componentList = componentList;
}

- (void)updateDataByPreParsedData:(NSDictionary*)params {
  NSDictionary* value = [params objectForKey:@"value"];
  if ([value isKindOfClass:[NSNull class]]) {
    value = @{};
  }

  NSString* processorName = [params objectForKey:@"preprocessorName"];
  NSDictionary* update_data_option = [params objectForKey:@"updatePageOption"];
  if (update_data_option != nil &&
      [[update_data_option objectForKey:@"reset_page_data"] boolValue]) {
    LynxTemplateData* data = [[LynxTemplateData alloc] initWithDictionary:value];
    [_lynxView resetDataWithTemplateData:data];
    return;
  }
  [_lynxView updateDataWithDictionary:value processorName:processorName];
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id>*)change
                       context:(void*)context {
  if ([keyPath isEqualToString:@"frame"]) {
    CGRect frame = _parentUI.frame;
    frame.size = _lynxView.frame.size;
    _parentUI.frame = frame;
    if ([_parentUI respondsToSelector:@selector(updateScrollContainerSize)]) {
      SEL selector = @selector(updateScrollContainerSize);
      NSMethodSignature* methodSignature = [_parentUI methodSignatureForSelector:selector];
      NSInvocation* invocation = [NSInvocation invocationWithMethodSignature:methodSignature];
      [invocation setSelector:selector];
      [invocation setTarget:_parentUI];
      [invocation invoke];
    }
  }
}

- (void)dealloc {
  [self.lynxView removeObserver:self forKeyPath:@"frame"];
}

- (void)loadTemplate:(NSDictionary*)params {
  NSString* url = [[TestBenchEnv sharedInstance] testBenchUrlPrefix];
  if (_globalPropsCache) {
    [_lynxView updateGlobalPropsWithTemplateData:[[LynxTemplateData alloc]
                                                     initWithDictionary:_globalPropsCache]];
  }
  if (_lynxThemeCache) {
    [_lynxView setTheme:_lynxThemeCache];
    _lynxThemeCache = nil;
  }
  if (_stateView != nil) {
    [_stateView removeFromSuperview];
    _stateView = nil;
  }
  NSString* source = params[@"source"];
  NSDictionary* initDict = params[@"templateData"];

  if (_preloadedSource) {
    _source = _preloadedSource;
  } else {
    _source =
        [[NSData alloc] initWithBase64EncodedString:source
                                            options:NSDataBase64DecodingIgnoreUnknownCharacters];
  }
  LynxTemplateData* initData = [[LynxTemplateData alloc] initWithDictionary:initDict];
  NSString* preprocessorName = [initDict objectForKey:@"preprocessorName"];
  bool readOnly = [[initDict objectForKey:@"readOnly"] boolValue];
  if (![@"" isEqualToString:preprocessorName]) {
    [initData markState:preprocessorName];
  }
  if (readOnly) {
    [initData markReadOnly];
  }

  _loadTemplateUrl = url;
  _loadTemplateInitData = initData;
  _hasLoadTemplate = YES;

  bool isCSR = true;
  if ([params objectForKey:@"isCSR"]) {
    isCSR = [[params objectForKey:@"isCSR"] boolValue];
  }

  if (isCSR && !_isSSRLoaded) {
    [_lynxView loadTemplate:_source withURL:url initData:initData];
  } else if (isCSR && _isSSRLoaded) {
    [_lynxView ssrHydrate:_source withURL:url initData:initData];
    _isSSRLoaded = false;
  } else {
    [_lynxView loadSSRData:_source withURL:url initData:initData];
    _isSSRLoaded = true;
  }
}

- (void)loadTemplateBundle:(NSDictionary*)params {
  NSString* url = [[TestBenchEnv sharedInstance] testBenchUrlPrefix];
  if (_globalPropsCache) {
    [_lynxView updateGlobalPropsWithTemplateData:[[LynxTemplateData alloc]
                                                     initWithDictionary:_globalPropsCache]];
  }
  if (_lynxThemeCache) {
    [_lynxView setTheme:_lynxThemeCache];
    _lynxThemeCache = nil;
  }
  if (_stateView != nil) {
    [_stateView removeFromSuperview];
    _stateView = nil;
  }
  NSString* source = params[@"source"];
  NSDictionary* initDict = params[@"templateData"];

  LynxTemplateData* initData = [[LynxTemplateData alloc] initWithDictionary:initDict];
  NSString* preprocessorName = [initDict objectForKey:@"preprocessorName"];
  bool readOnly = [[initDict objectForKey:@"readOnly"] boolValue];
  if (![@"" isEqualToString:preprocessorName]) {
    [initData markState:preprocessorName];
  }
  if (readOnly) {
    [initData markReadOnly];
  }
  _loadTemplateUrl = url;
  _loadTemplateInitData = initData;
  _hasLoadTemplate = YES;
  if (_templateBundle == nil) {
    if (_preloadedSource) {
      _source = _preloadedSource;
    } else {
      _source =
          [[NSData alloc] initWithBase64EncodedString:source
                                              options:NSDataBase64DecodingIgnoreUnknownCharacters];
    }
    [_lynxView
        loadTemplateBundle:[[LynxTemplateBundle alloc] initWithTemplate:[self source]
                                                                 option:_templateBundleOption]
                   withURL:url
                  initData:initData];
  } else {
    [_lynxView loadTemplateBundle:_templateBundle withURL:url initData:initData];
  }
}

- (void)reloadTemplate:(NSDictionary*)params {
  NSDictionary* value = [params objectForKey:@"value"];
  NSString* processorName = [params objectForKey:@"preprocessorName"];
  LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithDictionary:value];
  [templateData markState:processorName];
  [templateData markReadOnly];
  [_lynxView reloadTemplateWithTemplateData:templateData];
}

- (void)sendGlobalEvent:(NSDictionary*)params {
  NSArray* arguments = params[@"arguments"];
  // have no data we should ignore
  if (arguments.count != 2) {
    return;
  }
  // first is eventName
  NSString* name = arguments[0];
  if ([name isEqualToString:@"exposure"] || [name isEqualToString:@"disexposure"]) {
    return;
  }
  // second is event infomation
  NSArray* args = arguments[1];

  [_lynxView sendGlobalEvent:name withParams:args];
}

- (void)setGlobalProps:(NSDictionary*)params {
  if (_lynxView != nil) {
    _globalPropsCache = nil;
    LynxTemplateData* globalProps =
        [[LynxTemplateData alloc] initWithDictionary:params[@"global_props"]];
    [self.lynxView setGlobalPropsWithTemplateData:globalProps];
  } else {
    _globalPropsCache = params[@"global_props"];
  }
}

- (void)updateConfig:(NSDictionary*)params {
  if (self.lynxView != nil) {
    NSDictionary* config = [params objectForKey:@"config"];
    if ([config objectForKey:@"theme"]) {
      NSDictionary* theme = [config objectForKey:@"theme"];
      LynxTheme* lynxTheme = [[LynxTheme alloc] init];
      lynxTheme.themeConfig = theme;
      if (_lynxView != nil) {
        _lynxThemeCache = nil;
        [self.lynxView setTheme:lynxTheme];
      } else {
        _lynxThemeCache = lynxTheme;
      }
    }
  }
}

- (void)sendEventDarwin:(NSDictionary*)params {
  if (self.lynxView == nil) {
    return;
  }
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] initWithDictionary:params];
  [dict setObject:self.lynxView forKey:@"LynxView"];
  [TestBenchEmulateTouchHelper emulateTouch:dict];
}

- (void)endTestBench {
  __weak typeof(self) weakSelf = self;

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                               (int64_t)([self.replayConfig delayEndInterval] * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), ^{
                   TestBenchActionManager* strongSelf = weakSelf;
                   if (strongSelf.lynxView.baseInspectorOwner != nil) {
                     [strongSelf.lynxView.baseInspectorOwner
                         endTestbench:[strongSelf getDumpFilePath]];
                   }
                   if (self.endTestBenchBlock) {
                     self.endTestBenchBlock();
                   }
                 });
}

- (NSString*)getDumpFilePath {
  NSDate* currentDate =
      [[NSDate alloc] initWithTimeIntervalSinceNow:[[NSDate date] timeIntervalSinceNow]];
  NSDateFormatter* dateFormat = [[NSDateFormatter alloc] init];
  [dateFormat setDateFormat:@"yyyy-MM-dd-HHmmss"];
  NSString* dateString = [dateFormat stringFromDate:currentDate];
  NSString* filePath =
      [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject]
          stringByAppendingFormat:@"/lynx-testbench-%@", dateString];
  return filePath;
}

- (void)reload {
  _loadTemplateInitData = [_loadTemplateInitData deepClone];
  [_lynxView loadTemplate:_source withURL:_loadTemplateUrl initData:_loadTemplateInitData];
}

// when page reload, execute these functions in order
- (void)reloadAction {
  int64_t endTestTime = 1000;
  if (self.firstScreenBlock) {
    self.firstScreenBlock();
  }
  __weak typeof(self) _self = self;
  for (int index = 0; index < _reloadFuncList.count; index++) {
    ReloadAction* action = _reloadFuncList[index];
    SEL selector = NSSelectorFromString([action.functionName stringByAppendingString:@":"]);
    IMP imp = [self methodForSelector:selector];
    void (*func)(id, SEL, NSDictionary*) = (__typeof__(func))imp;
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(action.interval * NSEC_PER_MSEC)),
                   dispatch_get_main_queue(), ^{
                     if (_self) {
                       __strong typeof(_self) strongSelf = _self;
                       func(strongSelf, selector, action.params);
                     }
                   });
    endTestTime = action.interval + 1000 > endTestTime ? action.interval : endTestTime;
  }
  // TODO(zhaosong.lmm): This is to prevent timeouts when retrieving layout files in e2e testing.
  // Just a temporary fix and we need to adopt a more reasonable approach.
  if (endTestTime >= 60 * 1000) {
    endTestTime = 5 * 1000;
  }
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(endTestTime * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), ^{
                   if (_self) {
                     __strong typeof(_self) strongSelf = _self;
                     [strongSelf endTestBench];
                   }
                 });
}

@end
