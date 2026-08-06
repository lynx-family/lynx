// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <LynxDevtool/LynxRecorderDynamicComponentFetcher.h>
#import <third_party/zlib/zlib.h>

static NSString* ASSETS_SCHEME = @"assets://";
static NSString* LOAD_SCRIPT_BOOTSTRAP_URL = @"lynx-recorder://load-script-bootstrap.js";

static NSError* replayResourceError(NSString* description) {
  return [NSError
      errorWithDomain:@"LynxRecorderDynamicComponentFetcher"
                 code:LynxResourceResponseCodeFailed
             userInfo:@{NSLocalizedDescriptionKey : description ?: @"Invalid recorded resource."}];
}

static NSData* loadDataFromAssets(NSString* schema, NSError** error) {
  NSBundle* bundle = [NSBundle mainBundle];
  NSString* path = [bundle pathForResource:[schema stringByDeletingPathExtension]
                                    ofType:[schema pathExtension]
                               inDirectory:@"Resource"];

  NSData* data = [NSData dataWithContentsOfFile:path options:0 error:error];

  if (data == nil || data.length == 0) {
    NSLog(@"LynxRecorderDynamicComponentFetcher request from file failed, url:%@, error:%@", schema,
          [*error localizedDescription]);
    return nil;
  } else {
    NSLog(@"LynxRecorderDynamicComponentFetcher request from file succeeded, url:%@, data:%@",
          schema, data);
    return data;
  }
}

@interface LynxRecorderDynamicComponentFetcher ()
// Guard mutable replay resource state with @synchronized(self).
@property NSMutableDictionary* dynamicMap;
@property NSMutableDictionary<NSString*, NSData*>* scriptMap;
@property NSMutableDictionary<NSString*, LynxTemplateBundle*>* scriptBundleMap;
@property NSMutableDictionary<NSString*, NSString*>* materializedScriptURLMap;
@property NSString* materializedScriptDirectory;
@property NSArray* recordedActionList;
@property NSArray* recordedInvokedMethodData;
@property NSDictionary* recordedCallbackData;
@end

static NSData* inflateData(NSData* compressedData) {
  if (compressedData.length == 0) {
    return nil;
  }

  NSUInteger bufferGrowth = MAX(compressedData.length, 16 * 1024);
  NSMutableData* decompressed = [NSMutableData dataWithLength:bufferGrowth];
  z_stream stream = {0};
  stream.next_in = (Bytef*)compressedData.bytes;
  stream.avail_in = (uInt)compressedData.length;
  if (inflateInit(&stream) != Z_OK) {
    return nil;
  }

  int status = Z_OK;
  while (status == Z_OK) {
    if (stream.total_out >= decompressed.length) {
      [decompressed increaseLengthBy:bufferGrowth];
    }
    stream.next_out = (Bytef*)decompressed.mutableBytes + stream.total_out;
    stream.avail_out = (uInt)(decompressed.length - stream.total_out);
    status = inflate(&stream, Z_SYNC_FLUSH);
  }

  BOOL succeeded = status == Z_STREAM_END;
  inflateEnd(&stream);
  if (!succeeded) {
    return nil;
  }
  decompressed.length = stream.total_out;
  return decompressed;
}

@implementation LynxRecorderDynamicComponentFetcher

- (void)dealloc {
  if (self.materializedScriptDirectory.length > 0) {
    [[NSFileManager defaultManager] removeItemAtPath:self.materializedScriptDirectory error:nil];
  }
}

- (nullable NSString*)recordedKeyForURL:(NSString*)url inMap:(NSDictionary*)map {
  if (url.length == 0 || map[url] != nil) {
    return url.length > 0 ? url : nil;
  }

  NSString* urlWithoutQuery = [[url componentsSeparatedByString:@"?"] firstObject];
  BOOL hasQuery = [url containsString:@"?"];
  BOOL hasQualifiedPath = [urlWithoutQuery containsString:@"/"];
  if (map[urlWithoutQuery] != nil && (!hasQuery || hasQualifiedPath)) {
    return urlWithoutQuery;
  }

  NSString* lastPathComponent = urlWithoutQuery.lastPathComponent;
  if (map[lastPathComponent] != nil) {
    return lastPathComponent;
  }

  NSString* matchedKey = nil;
  for (id key in map) {
    if (![key isKindOfClass:[NSString class]]) {
      continue;
    }
    NSString* keyWithoutQuery = [[key componentsSeparatedByString:@"?"] firstObject];
    if (![keyWithoutQuery.lastPathComponent isEqualToString:lastPathComponent]) {
      continue;
    }
    if (matchedKey != nil) {
      return nil;
    }
    matchedKey = key;
  }
  return matchedKey;
}

- (void)loadDynamicComponent:(nonnull NSString*)schema
             withLoadedBlock:(nonnull onComponentLoaded)block {
  NSDictionary* data = nil;
  @synchronized(self) {
    data = [self.dynamicMap objectForKey:schema];
  }
  if (data != nil) {
    [self loadDynamicComponentWithCache:data withLoadedBlock:block];
  } else {
    [self loadDynamicComponentWithSchema:schema withLoadedBlock:block];
  }
}

- (void)parse:(NSArray*)actionList {
  [self parse:actionList scripts:nil invokedMethodData:nil callbackData:nil];
}

- (void)parse:(NSArray*)actionList
              scripts:(NSDictionary*)scripts
    invokedMethodData:(NSArray*)invokedMethodData
         callbackData:(NSDictionary*)callbackData {
  actionList = [actionList isKindOfClass:[NSArray class]] ? actionList : @[];
  scripts = [scripts isKindOfClass:[NSDictionary class]] ? scripts : @{};
  invokedMethodData = [invokedMethodData isKindOfClass:[NSArray class]] ? invokedMethodData : @[];
  callbackData = [callbackData isKindOfClass:[NSDictionary class]] ? callbackData : @{};
  NSMutableDictionary* dynamicMap = [[NSMutableDictionary alloc] init];
  NSMutableDictionary<NSString*, NSData*>* scriptMap = [[NSMutableDictionary alloc] init];
  for (id value in actionList) {
    if (![value isKindOfClass:[NSDictionary class]]) {
      continue;
    }
    NSDictionary* obj = value;
    NSString* functionName =
        [obj[@"Function Name"] isKindOfClass:[NSString class]] ? obj[@"Function Name"] : nil;
    NSDictionary* params =
        [obj[@"Params"] isKindOfClass:[NSDictionary class]] ? obj[@"Params"] : nil;
    if ([functionName isEqual:@"LoadComponentWithCallback"] &&
        [params[@"url"] isKindOfClass:[NSString class]]) {
      dynamicMap[params[@"url"]] = params;
    }
  }

  [scripts enumerateKeysAndObjectsUsingBlock:^(NSString* url, NSString* encodedSource, BOOL* stop) {
    if (![url isKindOfClass:[NSString class]] || ![encodedSource isKindOfClass:[NSString class]]) {
      return;
    }
    NSData* compressedData =
        [[NSData alloc] initWithBase64EncodedString:encodedSource
                                            options:NSDataBase64DecodingIgnoreUnknownCharacters];
    NSData* source = inflateData(compressedData);
    if (source.length > 0) {
      scriptMap[url] = source;
    } else {
      NSLog(@"[TestBench] Failed to decode recorded script: %@", url);
    }
  }];

  @synchronized(self) {
    _dynamicMap = dynamicMap;
    _scriptMap = scriptMap;
    _scriptBundleMap = [[NSMutableDictionary alloc] init];
    _materializedScriptURLMap = [[NSMutableDictionary alloc] init];
    _recordedActionList = [actionList copy];
    _recordedInvokedMethodData = [invokedMethodData copy];
    _recordedCallbackData = [callbackData copy];
  }
}

- (NSData*)recordedScriptForURL:(NSString*)url {
  @synchronized(self) {
    NSString* key = [self recordedKeyForURL:url inMap:self.scriptMap];
    return key != nil ? self.scriptMap[key] : nil;
  }
}

- (NSString*)materializedScriptURLForURL:(NSString*)url {
  @synchronized(self) {
    NSString* recordedKey = [self recordedKeyForURL:url inMap:self.scriptMap];
    if (recordedKey == nil) {
      return nil;
    }
    NSString* cachedURL = self.materializedScriptURLMap[recordedKey];
    if (cachedURL != nil) {
      return cachedURL;
    }

    NSData* source = self.scriptMap[recordedKey];
    if (source.length == 0) {
      return nil;
    }
    NSString* cachePath =
        [NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES) firstObject];
    if (self.materializedScriptDirectory.length == 0) {
      NSString* rootDirectory =
          [cachePath stringByAppendingPathComponent:@"lynx-recorder-preloads"];
      self.materializedScriptDirectory =
          [rootDirectory stringByAppendingPathComponent:NSUUID.UUID.UUIDString];
    }
    NSError* directoryError = nil;
    [[NSFileManager defaultManager] createDirectoryAtPath:self.materializedScriptDirectory
                              withIntermediateDirectories:YES
                                               attributes:nil
                                                    error:&directoryError];
    if (directoryError != nil) {
      NSLog(@"[TestBench] Failed to create preload directory: %@", directoryError);
      return nil;
    }

    NSString* extension = recordedKey.pathExtension.length > 0 ? recordedKey.pathExtension : @"js";
    NSString* filename = [NSString stringWithFormat:@"%@.%@", NSUUID.UUID.UUIDString, extension];
    NSString* path = [self.materializedScriptDirectory stringByAppendingPathComponent:filename];
    NSError* writeError = nil;
    if (![source writeToFile:path options:NSDataWritingAtomic error:&writeError]) {
      NSLog(@"[TestBench] Failed to materialize preload script %@: %@", recordedKey, writeError);
      return nil;
    }
    NSString* fileURL = [NSURL fileURLWithPath:path].absoluteString;
    self.materializedScriptURLMap[recordedKey] = fileURL;
    return fileURL;
  }
}

- (nullable NSString*)bootstrapTemplateNamed:(NSString*)name {
  NSBundle* containerBundle = [NSBundle bundleForClass:[LynxRecorderDynamicComponentFetcher class]];
  NSString* resourceBundlePath = [containerBundle pathForResource:@"LynxDebugResources"
                                                           ofType:@"bundle"];
  NSBundle* resourceBundle = [NSBundle bundleWithPath:resourceBundlePath];
  NSString* path = [resourceBundle pathForResource:name ofType:@"js"];
  if (path.length == 0) {
    NSLog(@"[TestBench] Missing replay bootstrap resource: %@.js", name);
    return nil;
  }
  NSError* error = nil;
  NSString* source = [NSString stringWithContentsOfFile:path
                                               encoding:NSUTF8StringEncoding
                                                  error:&error];
  if (source.length == 0) {
    NSLog(@"[TestBench] Failed to load %@.js: %@", name, error);
    return nil;
  }
  return source;
}

- (nullable NSString*)JSONLiteralForObject:(id)object {
  if (![NSJSONSerialization isValidJSONObject:object]) {
    return nil;
  }
  NSData* data = [NSJSONSerialization dataWithJSONObject:object options:0 error:nil];
  return [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
}

- (nullable NSString*)prepareLoadScriptBootstrapForURLs:(NSArray<NSString*>*)urls {
  (void)urls;
  @synchronized(self) {
    NSMutableDictionary<NSString*, NSString*>* sources = [NSMutableDictionary dictionary];
    [self.scriptMap enumerateKeysAndObjectsUsingBlock:^(NSString* url, NSData* data, BOOL* stop) {
      if (![url isKindOfClass:[NSString class]] || ![data isKindOfClass:[NSData class]] ||
          [url isEqual:LOAD_SCRIPT_BOOTSTRAP_URL]) {
        return;
      }
      NSString* source = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
      if (source != nil) {
        sources[url] = source;
      }
    }];

    NSDictionary* replayContext = @{
      @"version" : @1,
      @"config" : @{},
      @"actions" : self.recordedActionList ?: @[],
      @"invokedMethods" : self.recordedInvokedMethodData ?: @[],
      @"callbacks" : self.recordedCallbackData ?: @{},
      @"scripts" : sources,
    };
    NSString* replayContextLiteral = [self JSONLiteralForObject:replayContext];
    if (replayContextLiteral.length == 0) {
      return nil;
    }

    NSString* bootstrap = [self bootstrapTemplateNamed:@"lynx_recorder_load_script_bootstrap"];
    bootstrap = [bootstrap stringByReplacingOccurrencesOfString:@"__LYNX_RECORDER_REPLAY_CONTEXT__"
                                                     withString:replayContextLiteral];
    if (bootstrap.length == 0) {
      return nil;
    }

    self.scriptMap[LOAD_SCRIPT_BOOTSTRAP_URL] = [bootstrap dataUsingEncoding:NSUTF8StringEncoding];
    return LOAD_SCRIPT_BOOTSTRAP_URL;
  }
}

- (void)setRecordedTemplateBundle:(LynxTemplateBundle*)bundle forURL:(NSString*)url {
  if (bundle != nil && url.length > 0) {
    @synchronized(self) {
      self.scriptBundleMap[url] = bundle;
    }
  }
}

- (void)request:(LynxResourceRequest*)request onComplete:(LynxResourceLoadBlock)callback {
  NSLog(@"DemoLynxResourceProvider request url:%@", request.url);

  NSString* url = request.url;

  NSData* recordedScript = [self recordedScriptForURL:url];
  if (recordedScript.length > 0) {
    NSLog(@"[TestBench] Load recorded script: %@", url);
    callback([[LynxResourceResponse alloc] initWithData:recordedScript]);
    return;
  }

  if ([url hasPrefix:ASSETS_SCHEME]) {
    NSError* error;
    NSData* data = loadDataFromAssets([url substringFromIndex:[ASSETS_SCHEME length]], &error);
    if (data == nil) {
      callback([[LynxResourceResponse alloc] initWithError:error
                                                      code:LynxResourceResponseCodeFailed]);
    } else {
      callback([[LynxResourceResponse alloc] initWithData:data]);
    }
    return;
  }

  if (_urlRedirectMap != Nil && [_urlRedirectMap objectForKey:url] != Nil) {
    url = [_urlRedirectMap objectForKey:url];
    recordedScript = [self recordedScriptForURL:url];
    if (recordedScript.length > 0) {
      NSLog(@"[TestBench] Load redirected recorded script: %@", url);
      callback([[LynxResourceResponse alloc] initWithData:recordedScript]);
      return;
    }
  }

  NSURL* nsUrl = [NSURL URLWithString:url];
  if (nsUrl == nil) {
    callback([[LynxResourceResponse alloc]
        initWithError:replayResourceError([NSString
                          stringWithFormat:@"Invalid replay resource URL: %@", url ?: @""])
                 code:LynxResourceResponseCodeFailed]);
    return;
  }
  NSURLRequest* nsRequest = [NSURLRequest requestWithURL:nsUrl
                                             cachePolicy:NSURLRequestReloadIgnoringCacheData
                                         timeoutInterval:5];
  NSError* error = [[NSError alloc] init];
  NSData* data = [NSURLConnection sendSynchronousRequest:nsRequest
                                       returningResponse:nil
                                                   error:&error];
  if (data == nil || data.length == 0) {
    NSLog(@"DemoLynxResourceProvider request failed, url:%@, error:%@", request.url,
          [error localizedDescription]);
    callback([[LynxResourceResponse alloc] initWithError:error
                                                    code:LynxResourceResponseCodeFailed]);
  } else {
    NSLog(@"DemoLynxResourceProvider request successfully, url:%@, data:%@", request.url, data);
    callback([[LynxResourceResponse alloc] initWithData:data]);
  }
}

- (dispatch_block_t)fetchResource:(LynxResourceRequest*)request
                       onComplete:(LynxGenericResourceCompletionBlock)callback {
  [self request:request
      onComplete:^(LynxResourceResponse* response) {
        callback(response.data, response.error);
      }];
  return ^{
  };
}

- (dispatch_block_t)fetchResourcePath:(LynxResourceRequest*)request
                           onComplete:(LynxGenericResourcePathCompletionBlock)callback {
  NSError* error = replayResourceError(@"Recorded resources do not expose local file paths.");
  callback(nil, error);
  return ^{
  };
}

- (void)fetchTemplate:(LynxResourceRequest*)request
           onComplete:(LynxTemplateResourceCompletionBlock)callback {
  LynxTemplateBundle* recordedBundle = nil;
  @synchronized(self) {
    NSString* recordedKey = [self recordedKeyForURL:request.url inMap:self.scriptBundleMap];
    recordedBundle = recordedKey != nil ? self.scriptBundleMap[recordedKey] : nil;
  }
  if (recordedBundle != nil) {
    NSLog(@"[TestBench] Load recorded script bundle: %@", request.url);
    callback([[LynxTemplateResource alloc] initWithBundle:recordedBundle], nil);
    return;
  }

  [self loadDynamicComponent:request.url
             withLoadedBlock:^(NSData* data, NSError* error) {
               if (data.length > 0) {
                 callback([[LynxTemplateResource alloc] initWithNSData:data], nil);
               } else {
                 callback(nil, error);
               }
             }];
}

- (void)fetchSSRData:(LynxResourceRequest*)request
          onComplete:(LynxSSRResourceCompletionBlock)callback {
  [self request:request
      onComplete:^(LynxResourceResponse* response) {
        callback(response.data, response.error);
      }];
}

- (void)cancel:(LynxResourceRequest*)request {
}

- (void)loadDynamicComponentWithCache:(NSDictionary*)cache
                      withLoadedBlock:(onComponentLoaded)block {
  if (![cache[@"source"] isKindOfClass:[NSString class]]) {
    block(nil, replayResourceError(@"Recorded dynamic component source is invalid."));
    return;
  }
  NSData* res =
      [[NSData alloc] initWithBase64EncodedString:cache[@"source"]
                                          options:NSDataBase64DecodingIgnoreUnknownCharacters];
  if (res.length == 0) {
    block(nil, replayResourceError(@"Recorded dynamic component source cannot be decoded."));
    return;
  }
  if ([cache[@"sync_tag"] boolValue]) {
    block(res, nil);
  } else {
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async(queue, ^{
      block(res, nil);
    });
  }
}

- (void)loadDynamicComponentWithSchema:(nonnull NSString*)schema
                       withLoadedBlock:(nonnull onComponentLoaded)block {
  if (schema.length == 0) {
    block(nil, replayResourceError(@"Dynamic component URL is empty."));
    return;
  }
  if ([schema hasPrefix:ASSETS_SCHEME]) {
    NSError* error;
    NSData* data = loadDataFromAssets([schema substringFromIndex:[ASSETS_SCHEME length]], &error);
    block(data, error);
    return;
  }

  NSURL* url = [NSURL URLWithString:schema];
  if (url == nil) {
    block(nil, replayResourceError(
                   [NSString stringWithFormat:@"Invalid dynamic component URL: %@", schema]));
    return;
  }
  NSURLRequest* request = [NSURLRequest requestWithURL:url
                                           cachePolicy:NSURLRequestReloadIgnoringCacheData
                                       timeoutInterval:2];
  // use async request dynamic component template to to ensure the stability of test result
  [NSURLConnection
      sendAsynchronousRequest:request
                        queue:[[NSOperationQueue alloc] init]
            completionHandler:^(NSURLResponse* _Nullable response, NSData* _Nullable data,
                                NSError* _Nullable connectionError) {
              // response: The response header returned by the server
              // data: The response body returned by the server
              // connectionError: The connection error
              if (!connectionError) {
                block(data, nil);
              } else {
                block(data, connectionError);
              }
            }];
}
@end
