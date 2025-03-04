// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DARWIN_COMMON_LYNX_BASE_LYNXPAGERELOADHELPERPROTO_H_
#define DARWIN_COMMON_LYNX_BASE_LYNXPAGERELOADHELPERPROTO_H_

#import <Foundation/Foundation.h>
#import "LynxTemplateBundle.h"
#import "LynxTemplateData.h"
#import "LynxView.h"

NS_ASSUME_NONNULL_BEGIN

@protocol LynxPageReloadHelperProto <NSObject>

- (nonnull instancetype)initWithLynxView:(LynxView*)view;

/**
 * Load page from local file
 * @param tem local template data
 * @param url template url
 * @param data init data
 */
- (void)loadFromLocalFile:(NSData*)tem withURL:(NSString*)url initData:(LynxTemplateData*)data;
/**
 * Load page from url
 * @param url template url
 * @param data init data
 */
- (void)loadFromURL:(NSString*)url initData:(LynxTemplateData*)data;
/**
 * Load page from template bundle
 * @param bundle template bundle
 * @param url template url
 * @param data init data
 */
- (void)loadFromBundle:(LynxTemplateBundle*)bundle
               withURL:(NSString*)url
              initData:(LynxTemplateData*)data;

/**
 * Get template data url of page
 */
- (nonnull NSString*)getURL;

/**
 * Get template data of page
 */
- (LynxTemplateData*)getTemplateData;

/**
 * Reload whole LynxView
 */
- (void)reloadLynxView:(BOOL)ignoreCache;
/**
 * Reload partial template data
 * @param templateBin template data
 * @param fromFragments to be updated fragments
 * @param size fragment counts
 */
- (void)reloadLynxView:(BOOL)ignoreCache
          withTemplate:(NSString*)templateBin
         fromFragments:(BOOL)fromFragments
              withSize:(int32_t)size;
/**
 * Notify that page receives new template fragment
 * @param fragment new template fragment
 */
- (void)onReceiveTemplateFragment:(NSString*)fragment withEof:(BOOL)eof;
- (void)navigateLynxView:(nonnull NSString*)url;

- (void)setTextLable:(NSInteger)label;
- (void)removeTextLabel;
- (void)popTextLabel;

/**
 * Bind LynxView
 */
- (void)attachLynxView:(nonnull LynxView*)lynxView;

@end

NS_ASSUME_NONNULL_END

#endif  // DARWIN_COMMON_LYNX_BASE_LYNXPAGERELOADHELPERPROTO_H_
