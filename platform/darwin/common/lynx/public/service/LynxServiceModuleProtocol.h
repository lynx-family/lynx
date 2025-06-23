// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxServiceProtocol.h>
#import <Lynx/LynxView.h>

NS_ASSUME_NONNULL_BEGIN

// TODO(@nihao.royal): remove LynxServiceModuleProtocol;
@protocol LynxServiceModuleProtocol <LynxServiceProtocol>

/**
 * @depreacted: No need to initialize global props by LynxService.
 */
- (void)initGlobalProps:(LynxView *)lynxView;

/**
 *  @depreacted:  No need to clear module by LynxService.
 */
- (void)clearModuleForDestroy:(LynxView *)lynxView;

/**
 * clone globalProps when Page reload
 */
- (void)cloneGlobalPropsForReload:(LynxView *)lynxView;

@end

NS_ASSUME_NONNULL_END
