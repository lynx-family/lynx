// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxLogBoxManager.h"
#import <Lynx/LynxLog.h>
#import "LynxLogBox.h"
#import "LynxLogNotification.h"

static int const kMaxBriefMsgSize = 50;

@implementation LynxLogBoxManager {
  __weak UIViewController* _controller;
  LynxLogNotificationManager* _notificationManager;
  LynxLogBox* _logBox;
  NSMutableDictionary<NSNumber*, NSMutableArray*>* _proxyDic;  // level -> proxyArr
  NSMutableDictionary<NSNumber*, NSNumber*>* _currentIndex;  // level -> current index in _proxyArr
}

- (instancetype)initWithViewController:(UIViewController*)controller {
  self = [super init];
  if (self) {
    _controller = controller;
    _notificationManager = [[LynxLogNotificationManager alloc] initWithLogBoxManager:self];
    _logBox = [[LynxLogBox alloc] initWithLogBoxManager:self];
    _proxyDic = [NSMutableDictionary new];
    _currentIndex = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)onNewLog:(NSString*)message
       withLevel:(LynxLogBoxLevel)level
       withProxy:(LynxLogBoxProxy*)proxy {
  NSNumber* levelNum = [NSNumber numberWithInteger:level];
  if ([_proxyDic objectForKey:levelNum] == nil) {
    [_proxyDic setObject:[NSMutableArray new] forKey:levelNum];
    [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:0] forKey:levelNum];
  }
  NSMutableArray* proxyArr = [_proxyDic objectForKey:levelNum];
  BOOL isNew = ![proxyArr containsObject:proxy];
  if (isNew) {
    [proxyArr addObject:proxy];
  }
  NSString* errorMsg = [LynxLogBoxManager extractBriefMessage:message];
  [_notificationManager showNotificationWithMsg:errorMsg withLevel:level];
  [self updateLogMsgInLogBox:message withLevel:level withProxy:proxy isNewProxy:isNew];
}

- (void)onNewConsole:(NSDictionary*)message withProxy:(LynxLogBoxProxy*)proxy {
  if ([_logBox isShowing] && [_logBox getCurrentProxy] == proxy) {
    [_logBox onNewConsole:message withProxy:proxy isOnly:[_logBox isConsoleOnly]];
  }
}

- (void)updateTemplateUrl:(NSString*)url withProxy:(LynxLogBoxProxy*)proxy {
  if ([_logBox isShowing] && [_logBox getCurrentProxy] == proxy) {
    [_logBox updateTemplateUrl:url];
  }
}

- (void)showLogBoxWithLevel:(LynxLogBoxLevel)level {
  NSNumber* levelNum = [NSNumber numberWithInteger:level];
  NSNumber* indexNum = [_currentIndex objectForKey:levelNum];
  if (indexNum == nil) {
    return;
  }
  NSUInteger index = [indexNum unsignedIntegerValue];
  NSMutableArray* proxyArr = [_proxyDic objectForKey:levelNum];
  LynxLogBoxProxy* proxy = [proxyArr objectAtIndex:index];
  NSMutableArray* msg = [proxy logMessagesWithLevel:level];
  NSMutableArray* console = [proxy consoleMessages];
  [_logBox updateViewInfo:[proxy templateUrl] currentIndex:index + 1 totalCount:[proxyArr count]];
  for (NSString* item in msg) {
    if (![_logBox onNewLog:item withLevel:level withProxy:proxy]) {
      break;
    }
  }
  for (NSDictionary* item in console) {
    if (![_logBox onNewConsole:item withProxy:proxy isOnly:NO]) {
      break;
    }
  }
}

- (void)updateLogMsgInLogBox:(NSString*)message
                   withLevel:(LynxLogBoxLevel)level
                   withProxy:(LynxLogBoxProxy*)proxy
                  isNewProxy:(BOOL)isNew {
  if (![_logBox isShowing] || [_logBox getCurrentLevel] != level) {
    return;
  }
  if ([_logBox getCurrentProxy] == proxy) {
    [_logBox onNewLog:message withLevel:level withProxy:proxy];
  } else if (isNew) {
    NSNumber* levelNum = [NSNumber numberWithInteger:level];
    NSNumber* indexNum = [_currentIndex objectForKey:levelNum];
    if (indexNum != nil) {
      NSUInteger index = [indexNum unsignedIntegerValue];
      NSMutableArray* proxyArr = [_proxyDic objectForKey:levelNum];
      [_logBox updateViewInfo:[proxy templateUrl]
                 currentIndex:index + 1
                   totalCount:[proxyArr count]];
    }
  }
}

- (void)removeCurrentLogsWithLevel:(LynxLogBoxLevel)level {
  NSNumber* levelNum = [NSNumber numberWithInteger:level];
  NSNumber* curIndexNum = [_currentIndex objectForKey:levelNum];
  NSMutableArray* proxyArr = [_proxyDic objectForKey:levelNum];
  if (proxyArr == nil || curIndexNum == nil) {
    return;
  }
  NSUInteger index = [curIndexNum unsignedIntegerValue];
  LynxLogBoxProxy* currentProxy = [proxyArr objectAtIndex:index];
  if ([proxyArr count] > 1 && index == [proxyArr count] - 1) {
    [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:levelNum];
    LynxLogBoxProxy* previousProxy = [proxyArr objectAtIndex:(index - 1)];
    NSMutableArray* previousLogs = [previousProxy logMessagesWithLevel:level];
    [_notificationManager updateNotificationMsg:[previousLogs lastObject] withLevel:level];
  } else if ([proxyArr count] == 1) {
    [_currentIndex removeObjectForKey:levelNum];
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
    [_proxyDic removeObjectForKey:levelNum];
  }
  [self showLogBoxWithLevel:level];
}

- (void)removeLogsWithLevel:(LynxLogBoxLevel)level {
  NSNumber* levelNum = [NSNumber numberWithInteger:level];
  NSMutableArray* proxyArr = [_proxyDic objectForKey:levelNum];
  for (LynxLogBoxProxy* item in proxyArr) {
    [item removeLogMessagesWithLevel:level];
  }
  [_proxyDic removeObjectForKey:levelNum];
  [_currentIndex removeObjectForKey:levelNum];
}

- (void)changeView:(NSNumber*)indexNum withLevel:(LynxLogBoxLevel)level {
  NSNumber* levelNum = [NSNumber numberWithInteger:level];
  NSNumber* curIndexNum = [_currentIndex objectForKey:levelNum];
  if (curIndexNum != nil && indexNum != nil) {
    NSUInteger index = [indexNum unsignedIntegerValue];  // indexNum start from 1
    NSUInteger curIndex = [curIndexNum unsignedIntegerValue];
    if (index > 0 && curIndex != index - 1) {
      [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:levelNum];
    }
  }
  [self showLogBoxWithLevel:level];
}

- (void)reloadFromLogBox:(LynxLogBoxProxy*)proxy {
  [self reloadWithProxy:proxy];
  [proxy reloadLynxViewFromLogBox];
}

- (void)reloadWithProxy:(LynxLogBoxProxy*)proxy {
  [_logBox dismissIfNeeded];
  [_proxyDic enumerateKeysAndObjectsUsingBlock:^(
                 NSNumber* _Nonnull item, NSMutableArray* _Nonnull proxyArr, BOOL* _Nonnull stop) {
    NSInteger itemLevel = [item integerValue];
    NSNumber* curIndexNum = [_currentIndex objectForKey:item];
    if (proxyArr != nil && curIndexNum != nil) {
      NSUInteger index = [curIndexNum unsignedIntegerValue];
      LynxLogBoxProxy* currentProxy = [proxyArr objectAtIndex:index];
      if (currentProxy == proxy && index > 0) {
        [_currentIndex setObject:[NSNumber numberWithUnsignedInteger:(index - 1)] forKey:item];
        if (index == ([proxyArr count] - 1)) {
          LynxLogBoxProxy* previousProxy = [proxyArr objectAtIndex:(index - 1)];
          NSMutableArray* previousLogs = [previousProxy logMessagesWithLevel:itemLevel];
          [_notificationManager updateNotificationMsg:[previousLogs lastObject]
                                            withLevel:itemLevel];
        }
      } else if (currentProxy == proxy && [proxyArr count] == 1) {
        [_currentIndex removeObjectForKey:item];
        [_notificationManager removeNotificationWithLevel:itemLevel];
      }
      NSMutableArray* currentLogs = [currentProxy logMessagesWithLevel:itemLevel];
      if ([proxyArr containsObject:proxy] && currentLogs != nil) {
        NSNumber* updateCount = [NSNumber numberWithInteger:-[currentLogs count]];
        [_notificationManager updateNotificationMsgCount:updateCount withLevel:itemLevel];
      }
      [proxyArr removeObject:proxy];
      if ([proxyArr count] == 0) {
        [_proxyDic removeObjectForKey:item];
      }
    }
  }];
}

- (void)showConsoleMsgsWithProxy:(LynxLogBoxProxy*)proxy {
  NSMutableArray* msg = [proxy consoleMessages];
  [_logBox updateViewInfo:[proxy templateUrl] currentIndex:1 totalCount:1];
  for (NSDictionary* item in msg) {
    if (![_logBox onNewConsole:item withProxy:proxy isOnly:YES]) {
      break;
    }
  }
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
    LLogWarn(@"An error occurred when parse json: %@", [parseError localizedDescription]);
  } else {
    id rawError;
    id errorStr = [dic objectForKey:@"error"];
    if (!errorStr) {
      LLogWarn(@"Cannot find 'error' field in json");
    } else if ([errorStr isKindOfClass:[NSString class]]) {
      NSDictionary* errorDic =
          [NSJSONSerialization JSONObjectWithData:[errorStr dataUsingEncoding:NSUTF8StringEncoding]
                                          options:0
                                            error:&parseError];
      if (errorDic && !parseError) {
        rawError = [errorDic objectForKey:@"rawError"];
      } else {
        briefMessage = errorStr;
      }
    } else if ([errorStr isKindOfClass:[NSDictionary class]]) {
      rawError = [errorStr objectForKey:@"rawError"];
    }
    if (rawError && [rawError isKindOfClass:[NSDictionary class]]) {
      briefMessage = [rawError objectForKey:@"message"];
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
