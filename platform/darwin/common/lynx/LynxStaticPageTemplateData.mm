// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxStaticPageTemplateData.h"

#import <Lynx/LynxTemplateData.h>
#import <LynxBase/LynxLog.h>

@interface LynxStaticPageTemplateData : LynxTemplateData

- (instancetype)initWithStaticPageDictionary:(NSDictionary<NSString*, id>*)dictionary;

@end

@implementation LynxStaticPageTemplateData {
  NSDictionary<NSString*, id>* _platformData;
}

- (instancetype)initWithStaticPageDictionary:(NSDictionary<NSString*, id>*)dictionary {
  self = [super initWithDictionary:nil];
  if (self) {
    _platformData = dictionary;
    [super markReadOnly];
  }
  return self;
}

- (void)updateWithDictionary:(NSDictionary*)dictionary {
  _LogE(@"updateWithDictionary is not supported for static-page data");
}

- (void)updateWithTemplateData:(LynxTemplateData*)value {
  _LogE(@"updateWithTemplateData is not supported for static-page data");
}

- (BOOL)checkIsLegalData {
  return YES;
}

- (NSDictionary*)dictionary {
  return _platformData;
}

- (void)markState:(NSString*)name {
  _LogE(@"markState is not supported for static-page data");
}

- (LynxTemplateData*)deepClone {
  return [LynxTemplateData createForStaticPage:_platformData];
}

@end

LynxTemplateData* LynxCreateStaticPageTemplateData(NSDictionary<NSString*, id>* dictionary) {
  return [[LynxStaticPageTemplateData alloc] initWithStaticPageDictionary:dictionary];
}

BOOL LynxTemplateDataIsForStaticPage(LynxTemplateData* data) {
  return [data isKindOfClass:[LynxStaticPageTemplateData class]];
}
