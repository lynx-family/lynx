// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <UIKit/UIKit.h>

@interface LynxViewShellViewController : UIViewController
@property(nonatomic, retain) UINavigationController* navigationController;
@property(nonatomic, strong) NSString* url;
@property(nonatomic, strong) NSData* data;
@property(nonatomic, readwrite) NSMutableDictionary* params;
@property(nonatomic, readwrite) BOOL hiddenNav;

@end
