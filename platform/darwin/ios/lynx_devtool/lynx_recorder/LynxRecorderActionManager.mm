// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxResourceProvider.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateBundleOption.h>
#import <LynxDevtool/LynxDebugInfoRecorderDelegate.h>
#import <LynxDevtool/LynxInspectorOwner.h>
#import <LynxDevtool/LynxRecorderActionManager.h>
#import <LynxDevtool/LynxRecorderDynamicComponentFetcher.h>
#import <LynxDevtool/LynxRecorderEntranceViewController.h>
#import <LynxDevtool/LynxRecorderEnv.h>
#import <LynxDevtool/LynxRecorderOpenUrlModule.h>
#import <LynxDevtool/LynxRecorderReplayConfig.h>
#import <LynxDevtool/LynxRecorderReplayDataModule.h>
#import <LynxDevtool/LynxRecorderViewClient.h>
#import <third_party/zlib/zlib.h>

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

@interface LynxRecorderReplayDataProviderInternal : NSObject <LynxRecorderReplayDataProvider>
@property NSArray* functionCall;
@property NSDictionary* callbackData;
@property NSArray* jsbIgnoredInfo;
@property NSDictionary* jsbSettings;
@property NSDictionary* sharedData;
@end

@implementation LynxRecorderReplayDataProviderInternal
- (NSDictionary*)getJsbSettings {
  return _jsbSettings;
}
- (NSArray*)getFunctionCall {
  return _functionCall;
}

- (NSDictionary*)getCallbackData {
  return _callbackData;
}

- (NSArray*)getJSbIgnoredInfo {
  return _jsbIgnoredInfo;
}

- (NSDictionary*)getSharedData {
  return _sharedData;
}

@end

@interface LynxRecorderActionManager ()
@property UIView* parentUI;
@property CGPoint origin;
@property NSData* source;
@property NSData* preloadedSource;
@property int64_t startTime;
@property NSMutableArray* moduleList;
@property(nonnull) LynxView* lynxView;
@property(nonnull) LynxDebugInfoRecorderDelegate* lynxDebugInfoRecorderDelegate;
@property LynxTheme* lynxThemeCache;
@property NSDictionary* threadStrategyData;
@property NSDictionary* globalPropsCache;
@property NSArray<NSDictionary*>* componentList;
@property NSMutableArray* reloadFuncList;
@property NSString* loadTemplateUrl;
@property LynxTemplateData* loadTemplateInitData;
@property LynxRecorderViewClient* client;
@property LynxRecorderReplayStateView* stateView;
@property NSDictionary* config;
@property BOOL hasLoadTemplate;
@property LynxRecorderReplayConfig* replayConfig;
@property LynxRecorderDynamicComponentFetcher* dynamicComponentFetcher;
@property BOOL isSSRLoaded;
@property float rawFontScaleValue;
@property LynxTemplateBundle* templateBundle;
@property LynxTemplateBundleOption* templateBundleOption;
@property NSDictionary* templateBundleParams;
@property CGSize screenSize;
@property(nonatomic, strong) NSMutableArray* actionCallbacks;
@property(nonatomic, strong) id<LynxRecorderTouchHelper> touchHelper;
@property LynxRecorderReplayDataProviderInternal* dataProvider;

@end

@implementation LynxRecorderActionManager

- (id)init {
  if (self = [super init]) {
    self.actionCallbacks = [NSMutableArray array];
    self.lynxDebugInfoRecorderDelegate = [[LynxDebugInfoRecorderDelegate alloc] init];
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
               if (error != nil || [(NSHTTPURLResponse*)response statusCode] != 200) {
                 [strongSelf.stateView setReplayState:ERROR_DOWNLOAD_FAILED];
                 return;
               }
               [strongSelf.stateView setReplayState:PARSING_JSON_FILE];
               [strongSelf handleRecordFileData:data];
             }];
  [dataTask resume];
}

- (void)registerLynxRecorderActionCallback:(id<LynxRecorderActionCallback>)callback {
  [self.actionCallbacks addObject:callback];
}

- (void)registerTouchHelper:(id<LynxRecorderTouchHelper>)touchHelper {
  self.touchHelper = touchHelper;
}

- (void)create {
  if (_lynxView != nil) {
    [self.lynxView removeFromSuperview];
    self.lynxView = nil;
  }
  [self.stateView setReplayState:DOWNLOAD_JSON_FILE];
  [self.parentUI addSubview:_stateView];

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

- (void)startWithUrl:(NSString*)url
              inView:(UIView*)parentView
          withOrigin:(CGPoint)point
        replayConfig:(LynxRecorderReplayConfig*)replayConfig {
  [self startWithUrl:url
              inView:parentView
          withOrigin:point
        replayConfig:replayConfig
              NavBar:CGSizeMake(0, 0)];
}

- (void)startWithUrl:(NSString*)url
              inView:(UIView*)parentView
          withOrigin:(CGPoint)point
        replayConfig:(LynxRecorderReplayConfig*)replayConfig
              NavBar:(CGSize)navBarSize {
  self.url = url;
  if (![url hasPrefix:[LynxRecorderEnv sharedInstance].lynxRecorderUrlPrefix]) {
    _stateView = [[LynxRecorderReplayStateView alloc] init];
    [parentView addSubview:_stateView];
    [_stateView setReplayState:ERROR_MISS_LYNXRECORDER_HEADER];
    return;
  }
  [self setReplayConfig:replayConfig];
  _dataProvider = [[LynxRecorderReplayDataProviderInternal alloc] init];
  _parentUI = parentView;
  [_parentUI setBackgroundColor:self.replayConfig.backgroundColor];
  _origin = point;
  _stateView = [[LynxRecorderReplayStateView alloc] init];
  _startTime = 0;
  _hasLoadTemplate = NO;
  _screenSize.width = [UIScreen mainScreen].bounds.size.width;
  _screenSize.height = [UIScreen mainScreen].bounds.size.height - navBarSize.height -
                       [UIApplication sharedApplication].statusBarFrame.size.height;
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
  _dynamicComponentFetcher = [[LynxRecorderDynamicComponentFetcher alloc] init];

  [self create];
}

- (void)downloadRecordFile {
  if ([LynxRecorderEntranceViewController enableLoadLocalRecordFile:[self.replayConfig url]]) {
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
  NSData* data = [LynxRecorderEntranceViewController localRecordFileData:[self.replayConfig url]];
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
      self.dataProvider.jsbIgnoredInfo = [_config objectForKey:@"jsbIgnoredInfo"];
    }

    if ([_config objectForKey:@"jsbSettings"]) {
      self.dataProvider.jsbSettings = [_config objectForKey:@"jsbSettings"];
    }
  }
  if ([json objectForKey:@"Debug Info"]) {
    NSArray* debugInfoList = [json objectForKey:@"Debug Info"];
    for (NSDictionary* infoDict in debugInfoList) {
      NSString* url = infoDict[@"url"];
      NSString* content = infoDict[@"content"];
      [self.lynxDebugInfoRecorderDelegate setDebugInfo:url debugInfo:content];
    }
  }
  if ([json objectForKey:@"Invoked Method Data"]) {
    self.dataProvider.functionCall = json[@"Invoked Method Data"];
  }
  if ([json objectForKey:@"Callback"]) {
    self.dataProvider.callbackData = json[@"Callback"];
  }

  if ([json objectForKey:@"SharedData"]) {
    self.dataProvider.sharedData = json[@"SharedData"];
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

- (void)runAction:(id)obj onTestBenchComplete:(void (^)(int64_t))onTestBenchComplete {
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
  if (_startTime == 0) {
    _startTime = recordTime;
  }
  if ([functionName isEqualToString:@"sendEventDarwin"] && ![self.replayConfig replayGesture]) {
    return;
  }
  NSDictionary* params = obj[@"Params"];
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

- (NSString*)replayTimeEnvJScript {
  NSString* bundlePath = [[NSBundle mainBundle] pathForResource:@"LynxDebugResources"
                                                         ofType:@"bundle"];
  NSBundle* resourceBundle = [NSBundle bundleWithPath:bundlePath];
  NSString* jsonPath = [resourceBundle pathForResource:@"lynx_recorder" ofType:@"js"];
  NSData* data = [NSData dataWithContentsOfFile:jsonPath];
  NSString* script = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

  script =
      [script stringByReplacingOccurrencesOfString:@"###LYNX_RECORDER_REPLAY_TIME###"
                                        withString:[NSString stringWithFormat:@"%lld", _startTime]];

  NSString* docPath =
      [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];

  NSString* filePath = [docPath stringByAppendingPathComponent:@"lynx_recorder.js"];

  NSFileManager* fileManager = [NSFileManager defaultManager];

  [fileManager createFileAtPath:filePath
                       contents:[script dataUsingEncoding:NSUTF8StringEncoding]
                     attributes:nil];

  return [@"file://" stringByAppendingString:filePath];
}

- (void)handleActionList:(NSArray*)actionList {
  __block BOOL hasSetReplayStartTime = NO;
  if ([self.replayConfig enablePreDecode]) {
    [self preDecodeTemplate:_templateBundleParams];
  }
  [actionList enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL* stop) {
    @try {
      [self runAction:obj
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
      [builder.config registerModule:[LynxRecorderReplayDataModule class]
                               param:strongSelf.dataProvider];
      [builder.config registerModule:[LynxRecorderOpenUrlModule class]];
      [builder setFrame:CGRectMake(strongSelf->_origin.x, strongSelf->_origin.y,
                                   strongSelf->_screenSize.width, strongSelf->_screenSize.height)];
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

      NSMutableArray* preloadJSPaths = [[NSMutableArray alloc] init];
      if (![self.replayConfig forbidTimeFreeze]) {
        [preloadJSPaths addObject:[self replayTimeEnvJScript]];
      }
      NSString* groupName = @"ark";
      if (strongSelf->_lynxGroup == nil) {
        LynxGroupOption* groupOption = [[LynxGroupOption alloc] init];
        groupOption.preloadJSPaths = preloadJSPaths;
        strongSelf->_lynxGroup = [[LynxGroup alloc] initWithName:groupName
                                             withLynxGroupOption:groupOption];
      }
      builder.group = strongSelf->_lynxGroup;
      if (strongSelf->_rawFontScaleValue != -1) {
        builder.fontScale = strongSelf->_rawFontScaleValue;
      }
      // provide a change to register module or resource provider
      for (id<LynxRecorderActionCallback> callback in strongSelf.actionCallbacks) {
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
    _client = [[LynxRecorderViewClient alloc] init];
    _client.manager = self;
    [_lynxView.getLifecycleDispatcher addLifecycleClient:_client];
    [_lynxView setResourceFetcher:_client];
    [_lynxView addObserver:self
                forKeyPath:@"frame"
                   options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
                   context:nil];
    // provide a change to register module or resource provider
    for (id<LynxRecorderActionCallback> callback in self.actionCallbacks) {
      [callback onLynxViewDidBuild:_lynxView];
    }
  } else {
    _lynxView.layoutWidthMode = widthMode;
    _lynxView.layoutHeightMode = heightMode;
    if (preferredLayoutWidth > [UIScreen mainScreen].bounds.size.width) {
      preferredLayoutWidth = [UIScreen mainScreen].bounds.size.width;
    }
    if ((preferredLayoutHeight > _screenSize.height) && [self.replayConfig heightLimit]) {
      preferredLayoutHeight = _screenSize.height;
    }

    [_lynxView updateViewportWithPreferredLayoutWidth:preferredLayoutWidth
                                preferredLayoutHeight:preferredLayoutHeight];
  }
  if (_lynxView.baseInspectorOwner != nil) {
    [_lynxView.baseInspectorOwner setDebugInfoInterceptor:self.lynxDebugInfoRecorderDelegate];
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

- (void)switchEngineFromUIThread:(NSDictionary*)params {
  if (_lynxView == nil) {
    return;
  }
  bool attach = [[params objectForKey:@"attach"] boolValue];
  if (attach) {
    [_lynxView attachEngineToUIThread];
  } else {
    [_lynxView detachEngineFromUIThread];
  }
}

- (NSString*)convertNSDictToJsonString:(NSDictionary*)dict {
  NSError* error;
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:dict
                                                     options:NSJSONWritingPrettyPrinted
                                                       error:&error];
  NSString* jsonString = [NSString new];
  if (jsonData) {
    jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  }
  return jsonString;
}

- (LynxView*)buildFakeLynxView {
  CGFloat preferredLayoutHeight = 0.0f;
  CGFloat preferredLayoutWidth = 0.0f;
  CGFloat preferredMaxLayoutHeight = 0.0f;
  CGFloat preferredMaxLayoutWidth = 0.0f;

  LynxViewSizeMode widthMode = [self getViewSizeMode:0];
  LynxViewSizeMode heightMode = [self getViewSizeMode:1];
  LynxView* lynxView = [[LynxView alloc] initWithBuilderBlock:^(LynxViewBuilder* builder) {
    builder.config = [[LynxConfig alloc] initWithProvider:LynxConfig.globalConfig.templateProvider];
    [builder.config registerModule:[LynxRecorderReplayDataModule class] param:self.dataProvider];
    [builder.config registerModule:[LynxRecorderOpenUrlModule class]];
    [builder setFrame:CGRectMake(self->_origin.x, self->_origin.y, self->_screenSize.width,
                                 self->_screenSize.height)];
    NSInteger schema_thread_strategy = [self.replayConfig thread_mode];
    [builder
        setThreadStrategyForRender:
            [self getThreadStrategy:(int32_t)schema_thread_strategy
                testBenchThreadStrategy:[self->_threadStrategyData[@"threadStrategy"] intValue]]];
    if ([self->_threadStrategyData valueForKey:@"enableJSRuntime"]) {
      BOOL enableJSRuntime = ![_replayConfig enableAirStrictMode];
      [builder setEnableJSRuntime:enableJSRuntime];
      [builder setEnableAirStrictMode:!enableJSRuntime];
    }

    builder.fetcher = self->_dynamicComponentFetcher;

    NSMutableArray* preloadJSPaths = [[NSMutableArray alloc] init];
    if (![self.replayConfig forbidTimeFreeze]) {
      [preloadJSPaths addObject:[self replayTimeEnvJScript]];
    }
    NSString* groupName = @"ark";
    if (self->_lynxGroup == nil) {
      LynxGroupOption* groupOption = [[LynxGroupOption alloc] init];
      groupOption.preloadJSPaths = preloadJSPaths;
      self->_lynxGroup = [[LynxGroup alloc] initWithName:groupName withLynxGroupOption:groupOption];
    }
    builder.group = self->_lynxGroup;
    if (self->_rawFontScaleValue != -1) {
      builder.fontScale = self->_rawFontScaleValue;
    }
    // provide a change to register module or resource provider
    for (id<LynxRecorderActionCallback> callback in self.actionCallbacks) {
      [callback onLynxViewWillBuild:self builder:builder];
    }
  }];

  lynxView.layoutWidthMode = widthMode;
  lynxView.layoutHeightMode = heightMode;
  lynxView.preferredLayoutWidth = preferredLayoutWidth;
  lynxView.preferredLayoutHeight = preferredLayoutHeight;
  lynxView.preferredMaxLayoutHeight = preferredMaxLayoutHeight;
  lynxView.preferredMaxLayoutWidth = preferredMaxLayoutWidth;
  [_parentUI addSubview:lynxView];
  _threadStrategyData = nil;
  _client = [[LynxRecorderViewClient alloc] init];
  _client.manager = self;
  [lynxView.getLifecycleDispatcher addLifecycleClient:_client];
  [lynxView setResourceFetcher:_client];
  [lynxView addObserver:self
             forKeyPath:@"frame"
                options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
                context:nil];

  if (lynxView.baseInspectorOwner != nil) {
    [lynxView.baseInspectorOwner setDebugInfoInterceptor:self.lynxDebugInfoRecorderDelegate];
  }
  // provide a change to register module or resource provider
  for (id<LynxRecorderActionCallback> callback in self.actionCallbacks) {
    [callback onLynxViewDidBuild:lynxView];
  }
  return lynxView;
}

- (void)avoidLynxViewBeingNull {
  if (_lynxView == nil) {
    _lynxView = [self buildFakeLynxView];
  }
}

- (void)loadTemplate:(NSDictionary*)params {
  [self avoidLynxViewBeingNull];
  NSString* url = [[LynxRecorderEnv sharedInstance] lynxRecorderUrlPrefix];
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
    LynxLoadMeta* loadMeta = [LynxLoadMeta new];
    loadMeta.lynxViewConfig = [NSMutableDictionary new];
    NSMutableDictionary* nativeConfig = [NSMutableDictionary new];
    if ([[self replayConfig] disableOptPushStyleToBundle]) {
      [nativeConfig setObject:@NO forKey:@"enableOptPushStyleToBundle"];
    }
    if ([[self replayConfig] enableTextGradientOpt]) {
      [nativeConfig setObject:@YES forKey:@"enableTextGradientOpt"];
    }
    [loadMeta.lynxViewConfig setObject:[self convertNSDictToJsonString:nativeConfig]
                                forKey:@"platform_config"];
    loadMeta.binaryData = _source;
    loadMeta.url = url;
    loadMeta.initialData = initData;
    [_lynxView loadTemplate:loadMeta];
  } else if (isCSR && _isSSRLoaded) {
    LynxLoadMeta* loadMeta = [LynxLoadMeta new];
    loadMeta.binaryData = _source;
    loadMeta.url = url;
    loadMeta.initialData = initData;
    loadMeta.loadMode = LynxLoadModeHydrateSSR;
    [_lynxView loadTemplate:loadMeta];
    _isSSRLoaded = false;
  } else {
    LynxLoadMeta* loadMeta = [LynxLoadMeta new];
    loadMeta.binaryData = _source;
    loadMeta.url = url;
    loadMeta.initialData = initData;
    loadMeta.loadMode = LynxLoadModeRenderSSR;
    [_lynxView loadTemplate:loadMeta];
    _isSSRLoaded = true;
  }
}

- (void)loadTemplateBundle:(NSDictionary*)params {
  [self avoidLynxViewBeingNull];
  NSString* url = [[LynxRecorderEnv sharedInstance] lynxRecorderUrlPrefix];
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

    LynxLoadMeta* loadMeta = [LynxLoadMeta new];
    loadMeta.lynxViewConfig = [NSMutableDictionary new];
    NSMutableDictionary* nativeConfig = [NSMutableDictionary new];
    if ([[self replayConfig] disableOptPushStyleToBundle]) {
      [nativeConfig setObject:@NO forKey:@"enableOptPushStyleToBundle"];
    }
    if ([[self replayConfig] enableTextGradientOpt]) {
      [nativeConfig setObject:@YES forKey:@"enableTextGradientOpt"];
    }
    [loadMeta.lynxViewConfig setObject:[self convertNSDictToJsonString:nativeConfig]
                                forKey:@"platform_config"];
    loadMeta.templateBundle = [[LynxTemplateBundle alloc] initWithTemplate:[self source]
                                                                    option:_templateBundleOption];
    loadMeta.url = url;
    loadMeta.initialData = initData;

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

// Sometimes, clients pass the size of the front-end page to the globalprops,
// which is often related to the device size. When we replay on different devices,
// we need to update this part of the data.
- (void)preprocessGlobalPropsDictData:(NSMutableDictionary*)params {
  [params enumerateKeysAndObjectsUsingBlock:^(id key, id value, BOOL* stop) {
    if ([value isKindOfClass:[NSDictionary class]]) {
      [self preprocessGlobalPropsDictData:value];
    } else if ([value isKindOfClass:[NSArray class]]) {
      [self preprocessGlobalPropsArrayData:value];
    } else if ([key isKindOfClass:[NSString class]]) {
      if ([key isEqual:@"screenWidth"]) {
        [params setObject:@(self.screenSize.width) forKey:@"screenWidth"];
      } else if ([key isEqual:@"screenHeight"]) {
        [params setObject:@(self.screenSize.height) forKey:@"screenHeight"];
      }
    }
  }];
}

- (void)preprocessGlobalPropsArrayData:(NSArray*)array {
  [array enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL* stop) {
    if ([obj isKindOfClass:[NSDictionary class]]) {
      [self preprocessGlobalPropsDictData:obj];
    } else if ([obj isKindOfClass:[NSArray class]]) {
      [self preprocessGlobalPropsArrayData:obj];
    }
  }];
}

- (void)setGlobalProps:(NSDictionary*)params {
  if (_lynxView != nil) {
    _globalPropsCache = nil;
    NSMutableDictionary* mutableDictionary = [params[@"global_props"] mutableCopy];
    if ([self.replayConfig enableSizeOptimization]) {
      [self preprocessGlobalPropsDictData:mutableDictionary];
    }
    LynxTemplateData* globalProps = [[LynxTemplateData alloc] initWithDictionary:mutableDictionary];
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
  [self.touchHelper emulateTouch:dict];
}

- (void)endTestBench {
  __weak typeof(self) weakSelf = self;

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW,
                               (int64_t)([self.replayConfig delayEndInterval] * NSEC_PER_MSEC)),
                 dispatch_get_main_queue(), ^{
                   LynxRecorderActionManager* strongSelf = weakSelf;
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

- (void)getSourceAndContinue:(NSString*)url
                 cachePolicy:(NSURLRequestCachePolicy)cachePolicy
               continueBlock:(void (^)(NSData*))continueBlock {
  NSURLSessionConfiguration* configuration =
      [NSURLSessionConfiguration defaultSessionConfiguration];
  configuration.requestCachePolicy = cachePolicy;
  NSURLSession* session = [NSURLSession sessionWithConfiguration:configuration];
  NSURLSessionDataTask* dataTask =
      [session dataTaskWithURL:[NSURL URLWithString:url]
             completionHandler:^(NSData* _Nullable data, NSURLResponse* _Nullable response,
                                 NSError* _Nullable error) {
               if (error != nil) {
                 NSLog(@"[TestBench] Template.js(%@) is unavailable!: %@",
                       [[error userInfo] objectForKey:@"NSErrorFailingURLKey"],
                       [[error userInfo] objectForKey:@"NSLocalizedDescription"]);
                 return;
               }
               continueBlock(data);
             }];
  [dataTask resume];
}

- (void)reload {
  if ([self.replayConfig createWhenReload]) {
    [self create];
  } else {
    _loadTemplateInitData = [_loadTemplateInitData deepClone];
    if ([self.replayConfig sourceUrl] != nil) {
      __weak typeof(self) _self = self;
      [self getSourceAndContinue:[self.replayConfig sourceUrl]
                     cachePolicy:self.replayConfig.requestCachePolicy
                   continueBlock:^(NSData* data) {
                     dispatch_async(dispatch_get_main_queue(), ^{
                       if (_self) {
                         __strong typeof(_self) strongSelf = _self;
                         [strongSelf.lynxView loadTemplate:data
                                                   withURL:strongSelf.loadTemplateUrl
                                                  initData:strongSelf.loadTemplateInitData];
                       }
                     });
                   }];
    } else {
      [_lynxView loadTemplate:_source withURL:_loadTemplateUrl initData:_loadTemplateInitData];
    }
  }
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
