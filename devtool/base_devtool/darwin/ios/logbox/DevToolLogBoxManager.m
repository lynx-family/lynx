// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "DevToolLogBoxManager.h"
#import "DevToolLogBox.h"
#import "DevToolLogBoxNotification.h"

static int const kMaxBriefMsgSize = 50;

@implementation DevToolLogBoxManager {
  __weak UIViewController* _controller;
  DevToolLogBoxNotificationManager* _notificationManager;
  DevToolLogBox* _logBox;
  NSMutableDictionary<NSString*, NSMutableArray*>* _proxyDic;  // level -> proxyArr
  NSMutableDictionary<NSString*, NSNumber*>* _currentIndex;  // level -> current index in _proxyArr
}

- (instancetype)initWithViewController:(UIViewController*)controller {
  self = [super init];
  if (self) {
    _controller = controller;
    _notificationManager = [[DevToolLogBoxNotificationManager alloc] initWithLogBoxManager:self];
    _logBox = [[DevToolLogBox alloc] initWithLogBoxManager:self];
    _proxyDic = [NSMutableDictionary new];
    _currentIndex = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)onNewLog:(NSString*)message
       withLevel:(NSString*)level
       withProxy:(DevToolLogBoxProxy*)proxy {
  if ([_proxyDic objectForKey:level] == nil) {
    [_proxyDic setObject:[NSMutableArray new] forKey:level];
    [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:0] forKey:level];
  }
  NSMutableArray* proxyArr = [_proxyDic objectForKey:level];
  BOOL isNew = ![proxyArr containsObject:proxy];
  if (isNew) {
    [proxyArr addObject:proxy];
  }
  NSString* errorMsg = [DevToolLogBoxManager extractBriefMessage:message];
  [_notificationManager showNotificationWithMsg:errorMsg withLevel:level];
  [self updateLogMsgInLogBox:message withLevel:level withProxy:proxy isNewProxy:isNew];
}

- (void)updateEntryUrlForLogSrc:(NSString*)url withProxy:(DevToolLogBoxProxy*)proxy {
  if ([_logBox isShowing] && [_logBox getCurrentProxy] == proxy) {
    [_logBox updateEntryUrlForLogSrc:url];
  }
}

- (void)showLogBoxWithLevel:(NSString*)level {
  NSNumber* indexNum = [_currentIndex objectForKey:level];
  if (indexNum == nil) {
    return;
  }
  NSUInteger index = [indexNum unsignedIntegerValue];
  NSMutableArray* proxyArr = [_proxyDic objectForKey:level];
  DevToolLogBoxProxy* proxy = [proxyArr objectAtIndex:index];
  NSMutableArray* msg = [proxy logMessagesWithLevel:level];
  [_logBox updateViewInfo:[proxy entryUrlForLogSrc]
             currentIndex:index + 1
               totalCount:[proxyArr count]];
  for (NSString* item in msg) {
    if (![_logBox onNewLog:item withLevel:level withProxy:proxy]) {
      break;
    }
  }
}

- (void)updateLogMsgInLogBox:(NSString*)message
                   withLevel:(NSString*)level
                   withProxy:(DevToolLogBoxProxy*)proxy
                  isNewProxy:(BOOL)isNew {
  if (![_logBox isShowing] || [_logBox getCurrentLevel] != level) {
    return;
  }
  if ([_logBox getCurrentProxy] == proxy) {
    [_logBox onNewLog:message withLevel:level withProxy:proxy];
  } else if (isNew) {
    NSNumber* indexNum = [_currentIndex objectForKey:level];
    if (indexNum != nil) {
      NSUInteger index = [indexNum unsignedIntegerValue];
      NSMutableArray* proxyArr = [_proxyDic objectForKey:level];
      [_logBox updateViewInfo:[proxy entryUrlForLogSrc]
                 currentIndex:index + 1
                   totalCount:[proxyArr count]];
    }
  }
}

- (void)removeCurrentLogsWithLevel:(NSString*)level {
  NSNumber* curIndexNum = [_currentIndex objectForKey:level];
  NSMutableArray* proxyArr = [_proxyDic objectForKey:level];
  if (proxyArr == nil || curIndexNum == nil) {
    return;
  }
  NSUInteger index = [curIndexNum unsignedIntegerValue];
  DevToolLogBoxProxy* currentProxy = [proxyArr objectAtIndex:index];
  if ([proxyArr count] > 1 && index == [proxyArr count] - 1) {
    [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:level];
    DevToolLogBoxProxy* previousProxy = [proxyArr objectAtIndex:(index - 1)];
    NSMutableArray* previousLogs = [previousProxy logMessagesWithLevel:level];
    [_notificationManager updateNotificationMsg:[previousLogs lastObject] withLevel:level];
  } else if ([proxyArr count] == 1) {
    [_currentIndex removeObjectForKey:level];
    [_notificationManager removeNotificationWithLevel:level];
  }
  NSMutableArray* currentLogs = [currentProxy logMessagesWithLevel:level];
  if (currentLogs != nil) {
    NSNumber* updateCount = [NSNumber numberWithInteger:-[currentLogs count]];
    [_notificationManager updateNotificationMsgCount:updateCount withLevel:level];
  }
  [currentProxy removeLogMessagesWithLevel:level];
  [proxyArr removeObjectAtIndex:index];
  if ([proxyArr count] == 0) {
    [_proxyDic removeObjectForKey:level];
  }
  [self showLogBoxWithLevel:level];
}

- (void)removeLogsWithLevel:(NSString*)level {
  NSMutableArray* proxyArr = [_proxyDic objectForKey:level];
  for (DevToolLogBoxProxy* item in proxyArr) {
    [item removeLogMessagesWithLevel:level];
  }
  [_proxyDic removeObjectForKey:level];
  [_currentIndex removeObjectForKey:level];
}

- (void)changeView:(NSNumber*)indexNum withLevel:(NSString*)level {
  NSNumber* curIndexNum = [_currentIndex objectForKey:level];
  if (curIndexNum != nil && indexNum != nil) {
    NSUInteger index = [indexNum unsignedIntegerValue];  // indexNum start from 1
    NSUInteger curIndex = [curIndexNum unsignedIntegerValue];
    if (index > 0 && curIndex != index - 1) {
      [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:level];
    }
  }
  [self showLogBoxWithLevel:level];
}

- (void)onProxyReset:(DevToolLogBoxProxy*)proxy {
  [_logBox dismissIfNeeded];
  [_proxyDic enumerateKeysAndObjectsUsingBlock:^(
                 NSString* _Nonnull level, NSMutableArray* _Nonnull proxyArr, BOOL* _Nonnull stop) {
    NSNumber* curIndexNum = [_currentIndex objectForKey:level];
    if (proxyArr != nil && curIndexNum != nil) {
      NSUInteger index = [curIndexNum unsignedIntegerValue];
      DevToolLogBoxProxy* currentProxy = [proxyArr objectAtIndex:index];
      if (currentProxy == proxy && index > 0) {
        [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:level];
        if (index == ([proxyArr count] - 1)) {
          DevToolLogBoxProxy* previousProxy = [proxyArr objectAtIndex:(index - 1)];
          NSMutableArray* previousLogs = [previousProxy logMessagesWithLevel:level];
          [_notificationManager updateNotificationMsg:[previousLogs lastObject] withLevel:level];
        }
      } else if (currentProxy == proxy && [proxyArr count] == 1) {
        [_currentIndex removeObjectForKey:level];
        [_notificationManager removeNotificationWithLevel:level];
      }
      NSMutableArray* currentLogs = [currentProxy logMessagesWithLevel:level];
      if ([proxyArr containsObject:proxy] && currentLogs != nil) {
        NSNumber* updateCount = [NSNumber numberWithInteger:-[currentLogs count]];
        [_notificationManager updateNotificationMsgCount:updateCount withLevel:level];
      }
      [proxyArr removeObject:proxy];
      if ([proxyArr count] == 0) {
        [_proxyDic removeObjectForKey:level];
      }
    }
  }];
}

- (void)showNotification {
  [_notificationManager showNotification];
}

- (void)hideNotification {
  [_notificationManager hideNotification];
}

+ (NSString*)extractBriefMessage:(NSString*)message {
  if (!message) {
    return @"";
  }
  NSString* briefMessage;
  NSError* parseError;
  NSData* data = [message dataUsingEncoding:NSUTF8StringEncoding];
  NSDictionary* dic = [NSJSONSerialization JSONObjectWithData:data options:0 error:&parseError];
  if (parseError) {
    NSLog(@"An error occurred when parse json: %@", [parseError localizedDescription]);
  } else if ([dic isKindOfClass:[NSDictionary class]]) {
    id rawError;
    id errorStr = [dic objectForKey:@"error"];
    if (!errorStr) {
      NSLog(@"Cannot find 'error' field in json");
    } else if ([errorStr isKindOfClass:[NSString class]]) {
      id errorDic =
          [NSJSONSerialization JSONObjectWithData:[errorStr dataUsingEncoding:NSUTF8StringEncoding]
                                          options:0
                                            error:&parseError];
      if (!parseError && [errorDic isKindOfClass:[NSDictionary class]]) {
        rawError = [errorDic objectForKey:@"rawError"];
      } else {
        briefMessage = errorStr;
      }
    } else if ([errorStr isKindOfClass:[NSDictionary class]]) {
      rawError = [errorStr objectForKey:@"rawError"];
    }
    if (rawError && [rawError isKindOfClass:[NSDictionary class]]) {
      id rawMessage = [rawError objectForKey:@"message"];
      if ([rawMessage isKindOfClass:[NSString class]]) {
        briefMessage = rawMessage;
      }
    }
  }
  if (!briefMessage) {
    briefMessage = message;
  }
  if ([briefMessage length] > kMaxBriefMsgSize) {
    briefMessage = [briefMessage substringToIndex:kMaxBriefMsgSize];
  }
  briefMessage = [briefMessage stringByReplacingOccurrencesOfString:@"\n" withString:@" "];
  return briefMessage;
}

@end
