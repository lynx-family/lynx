/*
The MIT License (MIT)

Copyright (c) 2019 田风有 The MIT License (MIT)

Copyright (c) 2019 田风有

Copyright (c) 2019-2026 YJSettingTableView Software Foundation
(https://github.com/13662049573/TFY_Bannerscroll)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/*
The MIT License (MIT)

Copyright (c) 2013 fahrulazmi

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Reference: <https://www.theiphonewiki.com/wiki/Models>
*/

// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDeviceInfoHelper.h"
#import <CoreFoundation/CoreFoundation.h>
#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import <Lynx/LynxVersion.h>
#import <arpa/inet.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <netinet/in.h>
#import <sys/socket.h>
#import <sys/utsname.h>

typedef enum : NSInteger { NotReachable = 0, ReachableViaWiFi, ReachableViaWWAN } NetworkStatus;
@interface LynxNetReachability : NSObject
@end

@implementation LynxNetReachability {
  SCNetworkReachabilityRef _reachabilityRef;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;
    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(
        kCFAllocatorDefault, (const struct sockaddr *)&zeroAddress);
    _reachabilityRef = reachability;
  }
  return self;
}

- (void)dealloc {
  if (_reachabilityRef != NULL) {
    CFRelease(_reachabilityRef);
  }
}

- (NetworkStatus)networkStatusForFlags:(SCNetworkReachabilityFlags)flags {
  if ((flags & kSCNetworkReachabilityFlagsReachable) == 0) {
    return NotReachable;
  }

  NetworkStatus returnValue = NotReachable;

  if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0) {
    returnValue = ReachableViaWiFi;
  }

  if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand) != 0) ||
       (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0)) {
    if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0) {
      returnValue = ReachableViaWiFi;
    }
  }

  if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN) {
    returnValue = ReachableViaWWAN;
  }

  return returnValue;
}

- (NetworkStatus)currentReachabilityStatus {
  NSAssert(_reachabilityRef != NULL,
           @"currentNetworkStatus called with NULL SCNetworkReachabilityRef");
  NetworkStatus returnValue = NotReachable;
  SCNetworkReachabilityFlags flags;

  if (SCNetworkReachabilityGetFlags(_reachabilityRef, &flags)) {
    returnValue = [self networkStatusForFlags:flags];
  }

  return returnValue;
}

@end

@implementation LynxDeviceInfoHelper

+ (NSString *)getDeviceModel {
  struct utsname systemInfo;
  uname(&systemInfo);

  NSString *deviceModel = [NSString stringWithCString:systemInfo.machine
                                             encoding:NSASCIIStringEncoding];

  if ([deviceModel isEqualToString:@"iPhone3,1"]) return @"iPhone 4";
  if ([deviceModel isEqualToString:@"iPhone3,2"]) return @"iPhone 4";
  if ([deviceModel isEqualToString:@"iPhone3,3"]) return @"iPhone 4";
  if ([deviceModel isEqualToString:@"iPhone4,1"]) return @"iPhone 4S";
  if ([deviceModel isEqualToString:@"iPhone5,1"]) return @"iPhone 5";
  if ([deviceModel isEqualToString:@"iPhone5,2"]) return @"iPhone 5 (GSM+CDMA)";
  if ([deviceModel isEqualToString:@"iPhone5,3"]) return @"iPhone 5c (GSM)";
  if ([deviceModel isEqualToString:@"iPhone5,4"]) return @"iPhone 5c (GSM+CDMA)";
  if ([deviceModel isEqualToString:@"iPhone6,1"]) return @"iPhone 5s (GSM)";
  if ([deviceModel isEqualToString:@"iPhone6,2"]) return @"iPhone 5s (GSM+CDMA)";
  if ([deviceModel isEqualToString:@"iPhone7,1"]) return @"iPhone 6 Plus";
  if ([deviceModel isEqualToString:@"iPhone7,2"]) return @"iPhone 6";
  if ([deviceModel isEqualToString:@"iPhone8,1"]) return @"iPhone 6s";
  if ([deviceModel isEqualToString:@"iPhone8,2"]) return @"iPhone 6s Plus";
  if ([deviceModel isEqualToString:@"iPhone8,4"]) return @"iPhone SE";
  if ([deviceModel isEqualToString:@"iPhone9,1"]) return @"iPhone 7";
  if ([deviceModel isEqualToString:@"iPhone9,2"]) return @"iPhone 7 Plus";
  if ([deviceModel isEqualToString:@"iPhone9,3"]) return @"iPhone 7";
  if ([deviceModel isEqualToString:@"iPhone9,4"]) return @"iPhone 7 Plus";
  if ([deviceModel isEqualToString:@"iPhone10,1"]) return @"iPhone_8";
  if ([deviceModel isEqualToString:@"iPhone10,4"]) return @"iPhone_8";
  if ([deviceModel isEqualToString:@"iPhone10,2"]) return @"iPhone_8_Plus";
  if ([deviceModel isEqualToString:@"iPhone10,5"]) return @"iPhone_8_Plus";
  if ([deviceModel isEqualToString:@"iPhone10,3"]) return @"iPhone X";
  if ([deviceModel isEqualToString:@"iPhone10,6"]) return @"iPhone X";
  if ([deviceModel isEqualToString:@"iPhone11,8"]) return @"iPhone XR";
  if ([deviceModel isEqualToString:@"iPhone11,2"]) return @"iPhone XS";
  if ([deviceModel isEqualToString:@"iPhone11,6"]) return @"iPhone XS Max";
  if ([deviceModel isEqualToString:@"iPhone11,4"]) return @"iPhone XS Max";
  if ([deviceModel isEqualToString:@"iPhone12,1"]) return @"iPhone 11";
  if ([deviceModel isEqualToString:@"iPhone12,3"]) return @"iPhone 11 Pro";
  if ([deviceModel isEqualToString:@"iPhone12,5"]) return @"iPhone 11 Pro Max";
  if ([deviceModel isEqualToString:@"iPhone12,8"]) return @"iPhone SE2";
  if ([deviceModel isEqualToString:@"iPhone13,1"]) return @"iPhone 12 mini";
  if ([deviceModel isEqualToString:@"iPhone13,2"]) return @"iPhone 12";
  if ([deviceModel isEqualToString:@"iPhone13,3"]) return @"iPhone 12 Pro";
  if ([deviceModel isEqualToString:@"iPhone13,4"]) return @"iPhone 12 Pro Max";
  if ([deviceModel isEqualToString:@"iPhone14,4"]) return @"iPhone 13 mini";
  if ([deviceModel isEqualToString:@"iPhone14,5"]) return @"iPhone 13";
  if ([deviceModel isEqualToString:@"iPhone14,2"]) return @"iPhone 13 Pro";
  if ([deviceModel isEqualToString:@"iPhone14,3"]) return @"iPhone 13 Pro Max";
  if ([deviceModel isEqualToString:@"i386"] || [deviceModel isEqualToString:@"x86_64"])
    return @"Simulator";
  return @"iPhone";
}

+ (NSString *)getSystemVersion {
  return [[UIDevice currentDevice] systemVersion];
}

+ (NSString *)getLynxVersion {
  return [LynxVersion versionString];
}

+ (NSString *)getNetworkType {
  LynxNetReachability *reachability = [[LynxNetReachability alloc] init];

  NetworkStatus status = [reachability currentReachabilityStatus];

  switch (status) {
    case ReachableViaWWAN: {
      CTTelephonyNetworkInfo *info = [CTTelephonyNetworkInfo new];
      NSString *radioAccessTechnology = info.currentRadioAccessTechnology;

      if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyGPRS]) {
        return @"2G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyEdge]) {
        return @"2G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyWCDMA]) {
        return @"3G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyHSDPA]) {
        return @"3G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyHSUPA]) {
        return @"3G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMA1x]) {
        return @"CDMA";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORev0]) {
        return @"CDMA";
        ;
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevA]) {
        return @"CDMA";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyCDMAEVDORevB]) {
        return @"CDMA";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyeHRPD]) {
        return @"3G";
      } else if ([radioAccessTechnology isEqualToString:CTRadioAccessTechnologyLTE]) {
        return @"LTE";
      }
      break;
    }
    case ReachableViaWiFi: {
      return @"WiFi";
      break;
    }
    default:
      break;
  }

  return @"undefined";
}

@end
