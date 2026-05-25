// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/UIView+Lynx.h>

#import <Lynx/LynxRenderer.h>
#import <Lynx/LynxRendererHost.h>

#import <objc/runtime.h>

@interface UIView (LynxRendererHostInternal) <LynxRendererHost>
@end

@implementation UIView (Lynx)

- (void)setRenderer:(LynxRenderer *)renderer {
  objc_setAssociatedObject(self, @selector(renderer), renderer, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxRenderer *)renderer {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setRendererContext:(LynxRendererContext *)rendererContext {
  objc_setAssociatedObject(self, @selector(rendererContext), rendererContext,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxRendererContext *)rendererContext {
  return objc_getAssociatedObject(self, _cmd);
}

- (instancetype)initWithRendererContext:(LynxRendererContext *)context {
  self = [self init];
  if (self) {
    self.rendererContext = context;
  }
  return self;
}

- (LynxRenderer *)createRendererWithSign:(int32_t)sign andContext:(LynxRendererContext *)context {
  return [[LynxRenderer alloc] initWithRenderHost:(UIView<LynxRendererHost> *)self
                                          andSign:sign
                                       andContext:context];
}

- (UIView *)view {
  return self;
}

- (void)setLynxBackgroundLayer:(LynxBackgroundSubLayer *)lynxBackgroundLayer {
  objc_setAssociatedObject(self, @selector(lynxBackgroundLayer), lynxBackgroundLayer,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxBackgroundSubLayer *)lynxBackgroundLayer {
  return objc_getAssociatedObject(self, @selector(lynxBackgroundLayer));
}

- (void)setLynxBorderLayer:(LynxBorderLayer *)lynxBorderLayer {
  objc_setAssociatedObject(self, @selector(lynxBorderLayer), lynxBorderLayer,
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (LynxBorderLayer *)lynxBorderLayer {
  return objc_getAssociatedObject(self, @selector(lynxBorderLayer));
}

- (NSNumber *)lynxSign {
  return objc_getAssociatedObject(self, _cmd);
}

- (void)setLynxSign:(NSNumber *)sign {
  objc_setAssociatedObject(self, @selector(lynxSign), sign, OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)lynxClickable {
  NSNumber *number = objc_getAssociatedObject(self, _cmd);
  return [number boolValue];
}

- (void)setLynxClickable:(BOOL)clickable {
  objc_setAssociatedObject(self, @selector(lynxClickable), [NSNumber numberWithBool:clickable],
                           OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

- (BOOL)lynxEnableTapGestureSimultaneously {
  NSNumber *number = objc_getAssociatedObject(self, _cmd);
  return [number boolValue];
}

- (void)setLynxEnableTapGestureSimultaneously:(BOOL)enable {
  objc_setAssociatedObject(self, @selector(lynxEnableTapGestureSimultaneously),
                           [NSNumber numberWithBool:enable], OBJC_ASSOCIATION_RETAIN_NONATOMIC);
}

@end
