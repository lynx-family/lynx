// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#import "LynxUIOwner.h"
#import "LUIErrorHandling.h"
#import "LynxBaseInspectorOwner.h"
#import "LynxComponentRegistry.h"
#import "LynxEnv+Internal.h"
#import "LynxEnv.h"
#import "LynxEventHandler.h"
#import "LynxEventReporter.h"
#import "LynxFeatureCounter.h"
#import "LynxFontFaceManager.h"
#import "LynxGestureArenaManager.h"
#import "LynxGestureDetectorDarwin.h"
#import "LynxGlobalObserver.h"
#import "LynxPropsProcessor.h"
#import "LynxRootUI.h"
#import "LynxService.h"
#import "LynxShadowNodeOwner.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxUI+Internal.h"
#import "LynxUI+Private.h"
#import "LynxUICollection.h"
#import "LynxUIComponent.h"
#import "LynxUIContext.h"
#import "LynxUIExposure.h"
#import "LynxUIImage.h"
#import "LynxUIIntersectionObserver.h"
#import "LynxUIListContainer.h"
#import "LynxUIListLight.h"
#import "LynxUIMethodProcessor.h"
#import "LynxUIOwner+Accessibility.h"
#import "LynxUIOwner+Private.h"
#import "LynxUIText.h"
#import "LynxUIUnitUtils.h"
#import "LynxView+Internal.h"
#import "LynxViewInternal.h"
#import "LynxWeakProxy.h"
#import "UIView+Lynx.h"

// TODO(zhengsenyao): For white-screen problem investigation of preLayout, remove it later.
// constant defined in LynxContext.m
extern NSString* const kDefaultComponentID;

#pragma mark LynxUIContext (Internal)

@interface LynxUIContext () {
  __weak LynxEventHandler* _eventHandler;
  __weak LynxEventEmitter* _eventEmitter;
  __weak UIView* _rootView;
  NSDictionary* _keyframesDict;
}

@end

@implementation LynxUIContext (Internal)

- (void)setEventHandler:(LynxEventHandler*)eventHandler {
  _eventHandler = eventHandler;
}

- (void)setEventEmitter:(LynxEventEmitter*)eventEmitter {
  _eventEmitter = eventEmitter;
}

- (void)setRootView:(UIView*)rootView {
  _rootView = rootView;
}

- (void)setKeyframesDict:(NSDictionary*)keyframesDict {
  _keyframesDict = keyframesDict;
}

- (void)mergeKeyframesWithLynxKeyframes:(LynxKeyframes*)keyframes forKey:(NSString*)name {
  if (_keyframesDict == nil) {
    _keyframesDict = [[NSMutableDictionary alloc] init];
  }
  [(NSMutableDictionary*)_keyframesDict setValue:keyframes forKey:name];
}

- (void)removeKeyframeWithRemovedName:(NSString*)removedName {
  if (_keyframesDict != nil) {
    [(NSMutableDictionary*)_keyframesDict removeObjectForKey:removedName];
  }
}

@end

#pragma mark LynxUIOwner

@interface LynxUIOwner ()
@property(nonatomic) LynxRootUI* rootUI;
@property(nonatomic) CGSize oldRootSize;
@property(nonatomic) BOOL hasRootAttached;
@property(nonatomic, assign) BOOL enableNewGesture;
@property(nonatomic, weak) UIView<LUIBodyView>* containerView;
@property(nonatomic) NSMutableDictionary<NSString*, LynxWeakProxy*>* nameLynxUIMap;
@property(nonatomic) NSMutableDictionary<NSNumber*, LynxUI*>* uiHolder;
@property(nonatomic) NSMutableArray* a11yMutationList;
@property(nonatomic) NSMutableArray<id<LynxForegroundProtocol>>* foregroundListeners;
/**
 * componentIdToUiIdHolder is used to map radon/fiber component id to element id.
 * Because unlike virtual component id, radon/fiber component id is not equal to element id.
 * In method invokeUIMethod, we need to use this map and radon(js) component id to find related
 * UI.
 */
@property(nonatomic) NSMutableDictionary<NSString*, NSNumber*>* componentIdToUiIdHolder;
@property(nonatomic) NSMutableSet<LynxUI*>* uisThatHasNewLayout;
@property(nonatomic) NSMutableSet<LynxUI*>* uisThatHasOperations;
@property(nonatomic) LynxFontFaceContext* fontFaceContext;
@property(nonatomic) LynxComponentScopeRegistry* componentRegistry;
// Record used components in LynxView.
@property(nonatomic) NSMutableSet<NSString*>* componentSet;
@property(nonatomic) NSMutableDictionary<NSString*, NSHashTable<LynxUI*>*>* a11yIDHolder;
@end

@implementation LynxUIOwner {
  BOOL _enableDetailLog;
}

- (void)attachLynxView:(LynxView* _Nonnull)containerView {
  _containerView = containerView;
  _uiContext.rootView = containerView;
  _uiContext.errorHandler = containerView;
}

- (instancetype)initWithContainerView:(UIView<LUIBodyView>*)containerView
                       templateRender:(LynxTemplateRender*)templateRender
                    componentRegistry:(LynxComponentScopeRegistry*)registry
                        screenMetrics:(LynxScreenMetrics*)screenMetrics
                         errorHandler:(id<LUIErrorHandling>)errorHandler {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, @"LynxUIOwner init")
  self = [super init];
  if (self) {
    _enableDetailLog = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableLynxDetailLog
                                                        defaultValue:NO];
    _containerView = containerView;
    _templateRender = templateRender;
    _hasRootAttached = NO;
    _nameLynxUIMap = [[NSMutableDictionary alloc] init];
    _uiHolder = [[NSMutableDictionary alloc] init];
    _componentIdToUiIdHolder = [[NSMutableDictionary alloc] init];
    _oldRootSize = CGSizeZero;
    _uiContext = [[LynxUIContext alloc] initWithScreenMetrics:screenMetrics];
    _uiContext.rootView = containerView;
    _uiContext.rootUI = _rootUI;
    _uiContext.uiOwner = self;
    _uiContext.errorHandler = errorHandler;
    _uisThatHasNewLayout = [NSMutableSet new];
    _uisThatHasOperations = [NSMutableSet new];
    _fontFaceContext = [LynxFontFaceContext new];
    _fontFaceContext.rootView = containerView;
    _uiContext.fontFaceContext = _fontFaceContext;
    _componentRegistry = registry;
    _componentSet = [[NSMutableSet alloc] init];
    _a11yIDHolder = [[NSMutableDictionary alloc] init];
    _a11yMutationList = [[NSMutableArray alloc] init];
    _foregroundListeners = [[NSMutableArray alloc] init];
    // make sure singleton `LynxEnv` is already initialized
    // for registry of some LynxUI
    [LynxEnv sharedInstance];
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(didReceiveMemoryWarning)
               name:UIApplicationDidReceiveMemoryWarningNotification
             object:nil];

    [self listenAccessibilityFocused];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  return self;
}

- (instancetype)initWithContainerView:(LynxView*)containerView
                       templateRender:(LynxTemplateRender*)templateRender
                    componentRegistry:(LynxComponentScopeRegistry*)registry
                        screenMetrics:(LynxScreenMetrics*)screenMetrics {
  return [self initWithContainerView:containerView
                      templateRender:templateRender
                   componentRegistry:registry
                       screenMetrics:screenMetrics
                        errorHandler:containerView];
}

#pragma mark - A11y
- (NSArray<UIView*>*)viewsWithA11yID:(NSString*)a11yID {
  NSMutableArray<UIView*>* ret = [NSMutableArray array];
  [[self uiWithA11yID:a11yID]
      enumerateObjectsUsingBlock:^(LynxUI* _Nonnull obj, NSUInteger idx, BOOL* _Nonnull stop) {
        if (obj.view) {
          [ret addObject:obj.view];
        }
      }];
  return ret;
}

- (NSArray<LynxUI*>*)uiWithA11yID:(NSString*)a11yID {
  return self.a11yIDHolder[a11yID] ? [self.a11yIDHolder[a11yID] allObjects] : @[];
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)didReceiveMemoryWarning {
  [_uiHolder enumerateKeysAndObjectsUsingBlock:^(NSNumber* _Nonnull key, LynxUI* _Nonnull obj,
                                                 BOOL* _Nonnull stop) {
    if ([obj respondsToSelector:@selector(freeMemoryCache)]) {
      [obj freeMemoryCache];
    }
  }];
}

- (LynxUI*)findUIBySign:(NSInteger)sign {
  return [_uiHolder objectForKey:[NSNumber numberWithInteger:sign]];
}

/**
 * Finds the component by its component id.
 *
 * If the component to find is root, whose component id is -1, then getRootUI()
 * is returned.
 * If the component to find is a VirtualComponent, whose component
 * id equals to its sign of ui, then findUIBySign is called to find it directly.
 * If the component to find is a RadonComponent or FiberComponent, whose component id doesn't
 * equal to its sign of ui, then mComponentIdToUiIdHolder is used to find its
 * sign before calling findUIBySign.
 *
 * @param componentId the id of the component to find.
 * @return the component to find.
 */
- (LynxUI*)findUIByComponentId:(NSString*)componentId {
  if (componentId.length == 0 || [componentId isEqualToString:kDefaultComponentID]) {
    return _rootUI;
  }

  NSInteger sign = 0;
  if ([_componentIdToUiIdHolder objectForKey:componentId]) {
    sign = [[_componentIdToUiIdHolder objectForKey:componentId] integerValue];
  } else {
    sign = [componentId integerValue];
  }

  return [self findUIBySign:sign];
}

- (LynxUI*)findUIByIdSelector:(NSString*)idSelector withinUI:(LynxUI*)ui {
  if (ui && [ui.idSelector isEqualToString:idSelector]) {
    return ui;
  }

  for (LynxUI* child in ui.children) {
    if ([child.idSelector isEqualToString:idSelector]) {
      return child;
    }
    if (![child isKindOfClass:LynxUIComponent.class]) {
      LynxUI* target = [self findUIByIdSelector:idSelector withinUI:child];
      if (target != nil) {
        return target;
      }
    }
  }

  return nil;
}

- (LynxUI*)findUIByIdSelectorInParent:(NSString*)idSelector child:(LynxUI*)child {
  if (child && [child.idSelector isEqualToString:idSelector]) {
    return child;
  }

  if (!child) {
    return nil;
  }

  LynxUI* parent = child.parent;
  if (parent) {
    return [self findUIByIdSelectorInParent:idSelector child:child];
  }
  return nil;
}

// refId is used in ReactLynx
- (LynxUI*)findUIByRefId:(NSString*)refId withinUI:(LynxUI*)ui {
  if (ui && [ui.refId isEqualToString:refId]) {
    return ui;
  }

  for (LynxUI* child in ui.children) {
    if ([child.refId isEqualToString:refId]) {
      return child;
    }
    if (![child isKindOfClass:LynxUIComponent.class]) {
      LynxUI* target = [self findUIByRefId:refId withinUI:child];
      if (target != nil) {
        return target;
      }
    }
  }

  return nil;
}

// Due to historical reason, componentSet may be used in another thread. To promise thread safty,
// we return an empty set in this function. And this function will be removed later.
- (NSSet<NSString*>*)componentSet {
  (void)_componentSet;
  return [[NSSet alloc] init];
}

/**
 * Only reported when the component is first created.
 *
 * @param componentName the used component in LynxView
 */
- (void)componentStatistic:(NSString*)componentName {
  // According to the client configuration "enable_component_statistic_report" whether to enable
  // reporting.
  if ([[LynxEnv sharedInstance] enableComponentStatisticReport] &&
      ![_componentSet containsObject:componentName]) {
    [_componentSet addObject:componentName];
    [LynxEventReporter onEvent:@"lynxsdk_component_statistic"
                    instanceId:[_templateRender instanceId]
                  propsBuilder:^NSDictionary<NSString*, NSObject*>* _Nonnull {
                    return @{@"component_name" : componentName};
                  }];
  }
}

- (void)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(LynxUIMethodCallbackBlock)callback
              fromRoot:(NSString*)componentId
               toNodes:(NSArray*)nodes {
  LynxUI* targetUI = [self findUIByComponentId:componentId];
  NSString* errorMsg = [NSString stringWithFormat:@"component not found, nodes: [%@], method: %@",
                                                  [nodes componentsJoinedByString:@", "], method];

  if (targetUI) {
    for (size_t i = 0; i < nodes.count; i++) {
      NSString* node = [nodes objectAtIndex:i];
      BOOL isCalledByRef = params != nil && params.count > 0 && params[@"_isCallByRefId"];
      if (![node hasPrefix:@"#"] && !isCalledByRef) {
        if (callback) {
          callback(
              kUIMethodSelectorNotSupported,
              [node stringByAppendingString:@" not support, only support id selector currently"]);
        }
        return;
      }
      targetUI = isCalledByRef
                     ? [self findUIByRefId:node withinUI:targetUI]
                     : [self findUIByIdSelector:[node substringFromIndex:1] withinUI:targetUI];
      if (!targetUI) {
        errorMsg = [@"not found " stringByAppendingString:node];
        break;
      }
    }
  }

  if (targetUI) {
    [LynxUIMethodProcessor invokeMethod:method
                             withParams:params
                             withResult:callback
                                  forUI:targetUI];
  } else if (callback) {
    callback(kUIMethodNodeNotFound, errorMsg);
  }
}

- (void)invokeUIMethodForSelectorQuery:(NSString*)method
                                params:(NSDictionary*)params
                              callback:(LynxUIMethodCallbackBlock)callback
                                toNode:(int)sign {
  LynxUI* targetUI = [self findUIBySign:sign];
  if (targetUI) {
    LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                       [[targetUI.tagName ?: @"UIOwner"
                           stringByAppendingString:@".invokeUIMethodForSelectorQuery."]
                           stringByAppendingString:method ?: @""]);
    [LynxUIMethodProcessor invokeMethod:method
                             withParams:params
                             withResult:callback
                                  forUI:targetUI];
    LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
  } else if (callback) {
    callback(kUIMethodNoUiForNode, @"node does not have a LynxUI");
  }
}

- (bool)isTransformProps:(NSString*)value {
  if ([value isEqualToString:@"transform"]) {
    return true;
  }
  return false;
}

// TODO(songshourui.null): Once createUISyncWithSign is verified to be stable, remove this method.
- (void)createUIWithSign:(NSInteger)sign
                 tagName:(NSString*)tagName
                eventSet:(NSSet<NSString*>*)eventSet
           lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                   props:(NSDictionary*)props
               nodeIndex:(uint32_t)nodeIndex
      gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.createView." stringByAppendingString:tagName ?: @""])
  LynxUI* ui = [self createLynxUIWithTagName:tagName props:props];
  if (ui) {
    ui.context = _uiContext;
    ui.sign = sign;
    ui.nodeIndex = nodeIndex;
    [ui setImplicitAnimation];
    [ui updateCSSDefaultValue];

    [self updateEventWithUI:ui
                   eventSet:eventSet
              lepusEventSet:lepusEventSet
         gestureDetectorSet:gestureDetectorSet];

    [_uiHolder setObject:ui forKey:[NSNumber numberWithInteger:sign]];
    // Report the usage of the component.
    [self componentStatistic:tagName];
    [self updateComponentIdToUiIdMapIfNeedWithSign:sign tagName:tagName props:props];
    [self updatePropsWithUI:ui props:props isCreateUI:YES];
    if (ui.a11yID) {
      NSHashTable<LynxUI*>* table = self.a11yIDHolder[ui.a11yID];
      if (!table) {
        table = [NSHashTable weakObjectsHashTable];
        self.a11yIDHolder[ui.a11yID] = table;
      }
      [table addObject:ui];
    }
    if ([ui conformsToProtocol:@protocol(LynxForegroundProtocol)]) {
      [self registerForegroundListener:(id<LynxForegroundProtocol>)ui];
    }
    [ui propsDidUpdateForUIOwner];
    if ([ui notifyParent]) {
      [self markHasUIOperationsBottomUp:ui];
    } else {
      [self markHasUIOperations:ui];
    }
    [self addLynxUIToNameLynxUIMap:ui];
  } else {
    NSDictionary* info = @{@"node_index" : [NSString stringWithFormat:@"%d", nodeIndex]};
    NSDictionary* userInfo = @{@"LynxErrorCustomInfo" : info};
    @throw [NSException
        exceptionWithName:@"LynxCreateUIException"
                   reason:[NSString stringWithFormat:@"%@ ui not found when create UI", tagName]
                 userInfo:userInfo];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (LynxUI*)createLynxUIWithTagName:(NSString*)tagName props:(NSDictionary*)props {
  LynxUI* ui;
  BOOL supported = YES;
  if (!_hasRootAttached && [tagName isEqualToString:@"page"]) {
    _hasRootAttached = YES;
    _rootUI = [[LynxRootUI alloc] initWithLynxView:_containerView];
    _uiContext.rootUI = (LynxRootUI*)_rootUI;
    [_uiContext.uiExposure setRootUI:_rootUI];
    ui = _rootUI;
    LLogInfo(@"LynxUIOwner create rootUI %p with containerView %p", _rootUI, _containerView);
  } else {
    Class clazz = [_componentRegistry uiClassWithName:tagName accessible:&supported];
    if (supported) {
      ui = [[clazz alloc] init];
      ui.tagName = tagName;
    }
  }
  return ui;
}

// Both createUISyncWithSign & createUIAsyncWithSign will invoke this method, which can be executed
// on any thread without the expectation of thread safety issues.
- (LynxUI*)createUIInnerWithSign:(NSInteger)sign
                         tagName:(NSString*)tagName
                           clazz:(Class)clazz
                  supportedState:(TagSupportedState)state
                    onMainThread:(BOOL)onMainThread
                        eventSet:(NSSet<NSString*>*)eventSet
                   lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                           props:(NSDictionary*)props
                       nodeIndex:(uint32_t)nodeIndex
              gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  LynxUI* ui = [self createUIWithClass:clazz supportedState:state onMainThread:onMainThread];
  if (ui) {
    ui.nodeIndex = nodeIndex;
    ui.tagName = tagName;
    [ui setContext:_uiContext];
    [ui setSign:sign];
    [ui setImplicitAnimation];
    [ui updateCSSDefaultValue];
    [self updateEventWithUI:ui
                   eventSet:eventSet
              lepusEventSet:lepusEventSet
         gestureDetectorSet:gestureDetectorSet];

    [self updatePropsWithUI:ui props:props isCreateUI:YES];
  } else {
    NSDictionary* info = @{@"node_index" : [NSString stringWithFormat:@"%d", nodeIndex]};
    NSDictionary* userInfo = @{@"LynxErrorCustomInfo" : info};
    @throw [NSException
        exceptionWithName:@"LynxCreateUIException"
                   reason:[NSString stringWithFormat:@"%@ ui not found when create UI", tagName]
                 userInfo:userInfo];
  }
  return ui;
}

// During the creation of LynxUI, methods that cannot be executed asynchronously are encapsulated
// within the processUIOnMainThread method, which will be ensured to run on the main thread.
- (void)processUIOnMainThread:(LynxUI*)ui
                     withSign:(NSInteger)sign
                      tagName:(NSString*)tagName
                        props:(NSDictionary*)props {
  if (ui == nil) {
    return;
  }
  [_uiHolder setObject:ui forKey:[NSNumber numberWithInteger:sign]];
  // Report the usage of the component.
  [self componentStatistic:tagName];
  [self updateComponentIdToUiIdMapIfNeedWithSign:sign tagName:tagName props:props];
  if (ui.a11yID) {
    NSHashTable<LynxUI*>* table = self.a11yIDHolder[ui.a11yID];
    if (!table) {
      table = [NSHashTable weakObjectsHashTable];
      self.a11yIDHolder[ui.a11yID] = table;
    }
    [table addObject:ui];
  }
  if ([ui conformsToProtocol:@protocol(LynxForegroundProtocol)]) {
    [self registerForegroundListener:(id<LynxForegroundProtocol>)ui];
  }
  [ui propsDidUpdateForUIOwner];
  if ([ui notifyParent]) {
    [self markHasUIOperationsBottomUp:ui];
  } else {
    [self markHasUIOperations:ui];
  }
  [self addLynxUIToNameLynxUIMap:ui];
}

- (void)createUISyncWithSign:(NSInteger)sign
                     tagName:(NSString*)tagName
                       clazz:(Class)clazz
              supportedState:(TagSupportedState)state
                    eventSet:(NSSet<NSString*>*)eventSet
               lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                       props:(NSDictionary*)props
                   nodeIndex:(uint32_t)nodeIndex
          gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.createView." stringByAppendingString:tagName ?: @""])
  LynxUI* ui = [self createUIInnerWithSign:sign
                                   tagName:tagName
                                     clazz:clazz
                            supportedState:state
                              onMainThread:YES
                                  eventSet:eventSet
                             lepusEventSet:lepusEventSet
                                     props:props
                                 nodeIndex:nodeIndex
                        gestureDetectorSet:gestureDetectorSet];
  [self processUIOnMainThread:ui withSign:sign tagName:tagName props:props];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (LynxUI*)createUIAsyncWithSign:(NSInteger)sign
                         tagName:(NSString*)tagName
                           clazz:(Class)clazz
                  supportedState:(TagSupportedState)state
                        eventSet:(NSSet<NSString*>*)eventSet
                   lepusEventSet:(NSSet<NSString*>*)lepusEventSet
                           props:(NSDictionary*)props
                       nodeIndex:(uint32_t)nodeIndex
              gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.createViewAsync." stringByAppendingString:tagName ?: @""])
  LynxUI* ui = [self createUIInnerWithSign:sign
                                   tagName:tagName
                                     clazz:clazz
                            supportedState:state
                              onMainThread:NO
                                  eventSet:eventSet
                             lepusEventSet:lepusEventSet
                                     props:props
                                 nodeIndex:nodeIndex
                        gestureDetectorSet:gestureDetectorSet];

  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
  return ui;
}

- (Class)getTargetClass:(NSString*)tagName
                  props:(NSDictionary*)props
         supportedState:(TagSupportedState*)state {
  Class clazz = nil;
  if (!_hasRootAttached && [tagName isEqualToString:@"page"]) {
    *state = LynxRootTag;
  } else {
    BOOL supported = YES;
    clazz = [_componentRegistry uiClassWithName:tagName accessible:&supported];
    if (supported) {
      *state = LynxSupportedTag;
    } else {
      *state = LynxUnsupportedTag;
    }
  }
  return clazz;
}

- (BOOL)needCreateUIAsync:(NSString*)tagName {
  BOOL supported = YES;
  Class clazz = [_componentRegistry uiClassWithName:tagName accessible:&supported];
  return (clazz == [LynxUIView class] || clazz == [LynxUIImage class]);
}

// Given a Class and props, create the corresponding LynxUI instance, using different LynxUI init
// methods depending on whether it is called on the main thread or not.
- (LynxUI*)createUIWithClass:(Class)clazz
              supportedState:(TagSupportedState)state
                onMainThread:(BOOL)onMainThread {
  if (state == LynxRootTag) {
    _hasRootAttached = YES;
    _rootUI = [[LynxRootUI alloc] initWithLynxView:_containerView];
    _uiContext.rootUI = (LynxRootUI*)_rootUI;
    [_uiContext.uiExposure setRootUI:_rootUI];
    LLogInfo(@"LynxUIOwner create rootUI %p with containerView %p", _rootUI, _containerView);
    return _rootUI;
  } else if (state == LynxSupportedTag) {
    if (onMainThread) {
      return [[clazz alloc] init];
    } else {
      return [[clazz alloc] initWithoutView];
    }
  }
  return nil;
}

- (void)updatePropsWithUI:(LynxUI*)ui props:(NSDictionary*)props isCreateUI:(BOOL)isCreate {
  if (props && props.count != 0) {
    // TODO(WUJINTIAN): Like eventSet, passing the transition and animation properties separately
    // can avoid extra traversal.
    //  When updating UI, the transition and animation properties need to be consumed first.
    if (!isCreate) {
      if (nil != [props objectForKey:@"animation"]) {
        [ui setAnimation:props[@"animation"]];
      }
      if (nil != [props objectForKey:@"transition"]) {
        [ui setTransition:props[@"transition"]];
      }
    }
    for (NSString* key in props) {
      [LynxPropsProcessor updateProp:props[key] withKey:key forUI:ui];
    }
    if (isCreate) {
      if (nil != [props objectForKey:@"animation"]) {
        [ui setAnimation:props[@"animation"]];
      }
      if (nil != [props objectForKey:@"transition"]) {
        [ui setTransition:props[@"transition"]];
      }
    }
    [ui animationPropsDidUpdate];
  }
  if (ui.copyable) {
    [ui.lynxProps addEntriesFromDictionary:props];
  }
}

- (void)updateEventWithUI:(LynxUI*)ui
                 eventSet:(NSSet<NSString*>*)eventSet
            lepusEventSet:(NSSet<NSString*>*)lepusEventSet
       gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  if (eventSet || lepusEventSet) {
    [ui setRawEvents:eventSet andLepusRawEvents:lepusEventSet];
  }

  if (gestureDetectorSet) {
    [ui setGestureDetectors:gestureDetectorSet];
  }
}

- (void)updateUIWithSign:(NSInteger)sign
                   props:(NSDictionary*)props
                eventSet:(NSSet<NSString*>*)eventSet
           lepusEventSet:(NSSet<NSString*>*)lepusEventSet
      gestureDetectorSet:(NSSet<LynxGestureDetectorDarwin*>*)gestureDetectorSet {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  [self updateComponentIdToUiIdMapIfNeedWithSign:sign tagName:ui.tagName props:props];
  LYNX_TRACE_SECTION_WITH_INFO(LYNX_TRACE_CATEGORY_WRAPPER,
                               [@"UIOwner.updateProps." stringByAppendingString:ui.tagName ?: @""],
                               props)
  [self updateEventWithUI:ui
                 eventSet:eventSet
            lepusEventSet:lepusEventSet
       gestureDetectorSet:gestureDetectorSet];

  [self updatePropsWithUI:ui props:props isCreateUI:NO];
  [ui propsDidUpdateForUIOwner];
  [self markHasUIOperations:ui];
  [self addLynxUIToNameLynxUIMap:ui];
  if ([self observeA11yMutations]) {
    for (NSString* key in props) {
      [self addA11yPropsMutation:key sign:@(sign) a11yID:ui.a11yID toArray:self.a11yMutationList];
    }
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  if (!ui) {
    LLogError(@"LynxUIOwner.mm unable to update ui with sign:%@ props:%@", @(sign), props);
  }
}

- (void)insertNode:(NSInteger)childSign toParent:(NSInteger)parentSign atIndex:(NSInteger)index {
  LynxUI* child = _uiHolder[[NSNumber numberWithInteger:childSign]];
  LynxUI* parent = _uiHolder[[NSNumber numberWithInteger:parentSign]];
  if (child == nil || parent == nil) {
    parent = nil;
    child = nil;
    @throw [NSException exceptionWithName:@"LynxInsertUIException"
                                   reason:@"child or parent is null"
                                 userInfo:nil];
  }
  if (index == -1) {  // If the index is equal to -1 should add to the last
    index = parent.children.count;
  }
#if ENABLE_TRACE_PERFETTO
  // stringByAppendingFormat is not suitable for the situation which may be executed frequently
  NSString* p2c =
      [[parent.tagName stringByAppendingString:@"->"] stringByAppendingString:child.tagName ?: @""];
  [LynxTraceEvent beginSection:LYNX_TRACE_CATEGORY_WRAPPER
                      withName:[[@"UIOwner.insertNode." stringByAppendingString:@"parent->child:"]
                                   stringByAppendingString:p2c ?: @""]];
#endif
  if (_enableDetailLog && [parent isEqual:_rootUI]) {
    LLogInfo(@"LynxUIOwner insert node %p to rootUI %p", child, parent);
  }
  [parent insertChild:child atIndex:index];
  [self markHasUIOperations:parent];
  if ([self observeA11yMutations]) {
    [self addA11yMutation:@"insert"
                     sign:@(childSign)
                   a11yID:child.a11yID
                  toArray:self.a11yMutationList];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)detachNode:(NSInteger)childSign {
  LynxUI* child = _uiHolder[[NSNumber numberWithInteger:childSign]];
  if (child == nil) {
    @throw [NSException exceptionWithName:@"LynxDetachUIException"
                                   reason:@"child is null"
                                 userInfo:nil];
  }
  LynxUI* parent = child.parent;
  if (parent != nil) {
    [self recordNodeThatNeedLayoutBottomUp:parent];
    NSUInteger index = [parent.children indexOfObject:child];
    if (index != NSNotFound) {
      [parent removeChild:child atIndex:index];
    }
    if ([parent notifyParent]) {
      [self markHasUIOperationsBottomUp:parent];
    } else {
      [self markHasUIOperations:parent];
    }
  }
  if ([self observeA11yMutations]) {
    [self addA11yMutation:@"remove"
                     sign:@(childSign)
                   a11yID:child.a11yID
                  toArray:self.a11yMutationList];
  }
}

- (void)removeUIFromHolderRecursively:(LynxUI*)node {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.removeRecursively." stringByAppendingString:node.tagName ?: @""])
  [self removeUIFromHolder:node];
  for (LynxUI* child in node.children) {
    [self removeUIFromHolderRecursively:child];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)removeUIFromHolder:(LynxUI*)node {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.remove." stringByAppendingString:node.tagName ?: @""])

  [_uiHolder removeObjectForKey:@(node.sign)];
  [self removeLynxUIFromNameLynxUIMap:node];
  [_uiContext removeUIFromExposedMap:node];
  [node removeChildrenExposureUI];
  [_uiContext removeUIFromIntersectionManager:node];
  if ([self observeA11yMutations]) {
    [self addA11yMutation:@"detach"
                     sign:@(node.sign)
                   a11yID:node.a11yID
                  toArray:self.a11yMutationList];
  }
  if (node.a11yID) {
    [self.a11yIDHolder[node.a11yID] removeObject:node];
  }
  if ([node conformsToProtocol:@protocol(LynxForegroundProtocol)]) {
    [self unRegisterForegroundListener:(id<LynxForegroundProtocol>)node];
  }

  [[node getGestureArenaManager] removeMember:(id<LynxGestureArenaMember>)node
                                  detectorMap:node.gestureMap];
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)recycleNode:(NSInteger)sign {
  LynxUI* child = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (child == nil) {
    return;
  }
  LynxUI* parent = child.parent;
  if (parent != nil) {
    [self recordNodeThatNeedLayoutBottomUp:parent];
    NSUInteger index = [parent.children indexOfObject:child];
    if (index != NSNotFound) {
      [parent removeChild:child atIndex:index];
    }
    if ([parent notifyParent]) {
      [self markHasUIOperationsBottomUp:parent];
    } else {
      [self markHasUIOperations:parent];
    }
  }
  // FiberElement may be referenced by JS engine. Just clear the parent-son relationship.
  if (![_templateRender enableFiberArch]) {
    [self removeUIFromHolderRecursively:child];
  } else {
    [self removeUIFromHolder:child];
  }
}

- (void)updateUI:(NSInteger)sign
      layoutLeft:(CGFloat)left
             top:(CGFloat)top
           width:(CGFloat)width
          height:(CGFloat)height
         padding:(UIEdgeInsets)padding
          border:(UIEdgeInsets)border
          margin:(UIEdgeInsets)margin
          sticky:(nullable NSArray*)sticky {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (!ui) {
    return;
  }
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                     [@"UIOwner.updateLayout." stringByAppendingString:ui.tagName ?: @""])
  CGRect frame = CGRectMake(left, top, width, height);
  if (ui.alignHeight) {
    (&frame)->size.height = (NSInteger)height;
  }
  if (ui.alignWidth) {
    (&frame)->size.width = (NSInteger)width;
  }
  // To make up for the precision loss caused by float calculation or float to double conversion.
  [LynxUIUnitUtils roundRectToPhysicalPixelGrid:&frame];
  [LynxUIUnitUtils roundInsetsToPhysicalPixelGrid:&padding];
  [LynxUIUnitUtils roundInsetsToPhysicalPixelGrid:&border];
  [LynxUIUnitUtils roundInsetsToPhysicalPixelGrid:&margin];

  [ui updateFrame:frame
              withPadding:padding
                   border:border
                   margin:margin
      withLayoutAnimation:!ui.isFirstAnimatedReady];

  [ui updateSticky:sticky];
  if ([ui notifyParent]) {
    [self markHasUIOperationsBottomUp:ui];
  } else {
    [self markHasUIOperations:ui];
  }
  [self recordNodeThatNeedLayoutBottomUp:ui];
  if ([self observeA11yMutations]) {
    [self addA11yMutation:@"update" sign:@(sign) a11yID:ui.a11yID toArray:self.a11yMutationList];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
}

- (void)updateUI:(NSInteger)sign
      layoutLeft:(CGFloat)left
             top:(CGFloat)top
           width:(CGFloat)width
          height:(CGFloat)height
         padding:(UIEdgeInsets)padding
          border:(UIEdgeInsets)border {
  [self updateUI:sign
      layoutLeft:left
             top:top
           width:width
          height:height
         padding:padding
          border:border
          margin:UIEdgeInsetsZero
          sticky:nil];
}

- (void)recordNodeThatNeedLayoutBottomUp:(LynxUI*)ui {
  while (ui) {
    if ([_uisThatHasNewLayout containsObject:ui]) {
      break;
    }
    [_uisThatHasNewLayout addObject:ui];
    ui = ui.parent;
  }
}

- (void)willContainerViewMoveToWindow:(UIWindow*)window {
  [_uiContext.uiExposure willMoveToWindow:window == nil ? YES : NO];
  [_rootUI dispatchMoveToWindow:window];
}

- (void)onReceiveUIOperation:(id)value onUI:(NSInteger)sign {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui) {
    LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                       [@"UIOwner.ReceiveUIOperation." stringByAppendingString:ui.tagName ?: @""]);
    [ui onReceiveUIOperation:value];
    LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  }
}

- (void)markHasUIOperations:(LynxUI*)ui {
  if (ui) {
    [_uisThatHasOperations addObject:ui];
  }
}

- (void)markHasUIOperationsBottomUp:(LynxUI*)ui {
  while (ui) {
    [_uisThatHasOperations addObject:ui];
    ui = ui.parent;
  }
}

- (void)finishLayoutOperation:(int64_t)operationID componentID:(NSInteger)componentID {
  NSMutableSet<LynxUI*>* uisThatHasOperations = [NSMutableSet setWithSet:_uisThatHasOperations];
  [_uisThatHasOperations removeAllObjects];
  for (LynxUI* ui in uisThatHasOperations) {
    [ui finishLayoutOperation];
  }
  if ([self observeA11yMutations]) {
    [self flushMutations:self.a11yMutationList withBodyView:self.rootUI.rootView];
  }
  // find the right componnet by componentID on async-list
  if (componentID) {
    LynxUIComponent* component = (LynxUIComponent*)[self findUIBySign:componentID];
    [component asyncListItemRenderFinished:operationID];
  }

  if (UIAccessibilityIsVoiceOverRunning()) {
    // Resolve the issue of nested accessibility elements
    [self checkNestedAccessibilityElements:_rootUI];

    // Post `UIAccessibilityLayoutChangedNotification` to trigger updating accessibility elements
    UIAccessibilityPostNotification(UIAccessibilityLayoutChangedNotification, nil);
  }

  // Notify layout did finish.
  [_rootUI.context.observer notifyLayout:nil];
}

- (void)layoutDidFinish {
  NSMutableSet<LynxUI*>* uisThatHasNewLayout = [NSMutableSet setWithSet:_uisThatHasNewLayout];
  [_uisThatHasNewLayout removeAllObjects];
  for (LynxUI* ui in uisThatHasNewLayout) {
    LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER,
                       [@"UIOwner.layoutFinish." stringByAppendingString:ui.tagName ?: @""])
    [ui layoutDidFinished];
    LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER)
  }

  if (_containerView && !CGSizeEqualToSize(_oldRootSize, _rootUI.frame.size)) {
    _containerView.intrinsicContentSize = _rootUI.frame.size;
    _oldRootSize = _rootUI.frame.size;
  }
  [_rootUI.context.eventEmitter dispatchLayoutEvent];

  // Notify layout did finish.
  [_rootUI.context.observer notifyLayout:nil];
}

- (void)onNodeReady:(NSInteger)sign {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui) {
    [ui onNodeReadyForUIOwner];
    // Notify layout did finish.
    [_rootUI.context.observer notifyLayout:nil];
  }
}

- (void)onNodeRemoved:(NSInteger)sign {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui) {
    [self onNodeRemovedRecursively:ui];
  }
}

- (void)onNodeReload:(NSInteger)sign {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui) {
    [ui onNodeReload];
  }
}

- (void)onNodeRemovedRecursively:(LynxUI*)ui {
  [ui onNodeRemoved];
  for (LynxUI* child in ui.children) {
    [self onNodeRemovedRecursively:child];
  }
}

- (nullable LynxUI*)uiWithName:(NSString*)name {
  for (NSNumber* sign in _uiHolder) {
    LynxUI* ui = _uiHolder[sign];
    if ([ui.name isEqualToString:name]) {
      return ui;
    }
  }
  return nil;
}

- (nullable LynxUI*)uiWithIdSelector:(NSString*)idSelector {
  for (NSNumber* sign in _uiHolder) {
    LynxUI* ui = _uiHolder[sign];
    if ([ui.idSelector isEqualToString:idSelector]) {
      return ui;
    }
  }
  return nil;
}

- (void)reset {
  [_uiContext.uiExposure destroyExposure];
  if ([_uiContext.intersectionManager enableNewIntersectionObserver]) {
    [_uiContext.intersectionManager destroyIntersectionObserver];
  }
  [_componentIdToUiIdHolder removeAllObjects];
  _oldRootSize = CGSizeZero;
  [_foregroundListeners removeAllObjects];
  // reset shellptr to avoid dangling pointer
  _uiContext.shellPtr = 0;
  // reset gesture manager in main thread
  [self resetGestureArenaInUIThread];

  // we will dereference LynxUIOwner to LynxUI after calling reset.
  [self releaseUI];
}

- (void)releaseUI {
  _hasRootAttached = NO;
  [_uiHolder removeAllObjects];
  [_uisThatHasNewLayout removeAllObjects];
  [_uisThatHasOperations removeAllObjects];
  [_a11yIDHolder removeAllObjects];
  if ([NSThread isMainThread]) {
    // LynxUI dealloc will be triggered when rootUI is set to nil, so we need to make sure the reset
    // process is called on the main thread.
    _rootUI = nil;
  } else {
    LLogInfo(@"LynxUIOwner:releaseUI on a child thread");
    [LynxFeatureCounter count:LynxFeatureObjcUiOwnerReleaseOnChildThread
                   instanceId:[_templateRender instanceId]];
  }
}

- (void)pauseRootLayoutAnimation {
  _rootUI.layoutAnimationRunning = NO;
}

- (void)resumeRootLayoutAnimation {
  _rootUI.layoutAnimationRunning = YES;
}

- (void)updateAnimationKeyframes:(NSDictionary*)keyframesDict {
  NSString* removeName = [keyframesDict objectForKey:@"removeKeyframe"];
  if ([removeName length] != 0) {
    [_uiContext removeKeyframeWithRemovedName:removeName];
  }
  NSDictionary* dict = [keyframesDict objectForKey:@"keyframes"];
  for (NSString* name in dict) {
    LynxKeyframes* keyframes = [[LynxKeyframes alloc] init];
    keyframes.styles = dict[name];
    [_uiContext mergeKeyframesWithLynxKeyframes:keyframes forKey:name];
  }
}

/**
 * Called during cell reuse, in prepareForReuse, and on successful reuse, call restart.
 */
- (void)resetAnimation {
  [_uiHolder enumerateKeysAndObjectsUsingBlock:^(NSNumber* _Nonnull key, LynxUI* _Nonnull obj,
                                                 BOOL* _Nonnull stop) {
    [obj.animationManager resetAnimation];
  }];
}
- (void)restartAnimation {
  [_uiHolder enumerateKeysAndObjectsUsingBlock:^(NSNumber* _Nonnull key, LynxUI* _Nonnull obj,
                                                 BOOL* _Nonnull stop) {
    [obj.animationManager restartAnimation];

    if ([obj isKindOfClass:[LynxUIImage class]]) {
      LynxUIImage* uiImage = (LynxUIImage*)obj;
      if (uiImage.isAnimated) {
        [uiImage startAnimating];
      }
    }
  }];
}

/**
 * when LynxView is enter foreground
 */
- (void)onEnterForeground {
  [self resumeAnimation];
  for (id listener in _foregroundListeners) {
    [listener onEnterForeground];
  }
}

/**
 * when LynxView is enter background
 */
- (void)onEnterBackground {
  for (id listener in _foregroundListeners) {
    [listener onEnterBackground];
  }
}

/**
 * register listener for LynxUI which implement `LynxForegroundProtocol` protocol
 * LynxForegroundProtocol is triggered when lynxview enter/exit foreground
 */
- (void)registerForegroundListener:(id<LynxForegroundProtocol>)listener {
  [_foregroundListeners addObject:listener];
}

/**
 * unregister listener for LynxUI which implement `LynxForegroundProtocol` protocol
 * LynxForegroundProtocol is triggered when lynxview enter/exit foreground
 */
- (void)unRegisterForegroundListener:(id<LynxForegroundProtocol>)listener {
  [_foregroundListeners removeObject:listener];
}

- (void)resumeAnimation {
  [_uiHolder enumerateKeysAndObjectsUsingBlock:^(NSNumber* _Nonnull key, LynxUI* _Nonnull obj,
                                                 BOOL* _Nonnull stop) {
    [obj.animationManager resumeAnimation];
  }];
}

- (void)updateFontFaceWithDictionary:(NSDictionary*)dic {
  if (dic == nil) return;

  NSString* fontFamily = [dic valueForKey:@"font-family"];
  NSString* src = [dic valueForKey:@"src"];

  LynxFontFace* face = [[LynxFontFace alloc] initWithFamilyName:fontFamily
                                                         andSrc:src
                                                withLynxContext:self.uiContext.lynxContext];
  if (face == nil) return;

  [_fontFaceContext addFontFace:face];
}

- (LynxComponentScopeRegistry*)getComponentRegistry {
  return _componentRegistry;
}

- (void)didMoveToWindow:(BOOL)windowIsNil {
  [_uiContext.uiExposure didMoveToWindow:windowIsNil];
  if ([_uiContext.intersectionManager enableNewIntersectionObserver]) {
    [_uiContext.intersectionManager didMoveToWindow:windowIsNil];
  }
  if (!windowIsNil) {
    [self resumeAnimation];
  }
}

#pragma mark - property: nameLynxUIMap related

- (void)addLynxUIToNameLynxUIMap:(LynxUI*)ui {
  if (ui.name && ui.name.length != 0) {
    LynxWeakProxy* weakUI = [LynxWeakProxy proxyWithTarget:ui];
    [_nameLynxUIMap setObject:weakUI forKey:ui.name];
  }
}

- (void)removeLynxUIFromNameLynxUIMap:(LynxUI*)ui {
  if (ui.name && ui.name.length != 0) {
    LynxWeakProxy* weakLynxUI = [_nameLynxUIMap objectForKey:ui.name];
    if (weakLynxUI) {
      [_nameLynxUIMap removeObjectForKey:ui.name];
    }
  }
}

- (nullable LynxWeakProxy*)weakLynxUIWithName:(NSString*)name {
  return [_nameLynxUIMap objectForKey:name];
}

- (id<LynxBaseInspectorOwner>)baseInspectOwner {
  if ([_containerView respondsToSelector:@selector(baseInspectorOwner)]) {
    return [_containerView performSelector:@selector(baseInspectOwner)];
  }
  return nil;
}

- (BOOL)observeA11yMutations {
  return self.rootUI.context.enableA11yIDMutationObserver && UIAccessibilityIsVoiceOverRunning();
}

#pragma mark - property: componentIdToUiIdMap related

- (void)updateComponentIdToUiIdMapIfNeedWithSign:(NSInteger)sign
                                         tagName:(NSString*)tagName
                                           props:(NSDictionary*)props {
  if ([tagName isEqualToString:@"component"] && [props objectForKey:@"ComponentID"]) {
    id componentIDValue = [props objectForKey:@"ComponentID"];
    if ([componentIDValue isKindOfClass:[NSString class]]) {
      [_componentIdToUiIdHolder setObject:@(sign) forKey:componentIDValue];
    } else if ([componentIDValue isKindOfClass:[NSNumber class]]) {
      [_componentIdToUiIdHolder setObject:@(sign) forKey:[componentIDValue stringValue]];
    }
  }
}

// TODO(linxiaosong): Only return common, custom and virtual info now. More info may be needed in
// the future.
// TODO(chennengshi): After the Decouple Layout is completed, unify this function and the similar
// functions in ShadowNodeOwner.
- (NSInteger)getTagInfo:(NSString*)tagName {
  NSInteger result = 0;
  BOOL supported = YES;
  Class clazz = [_componentRegistry shadowNodeClassWithName:tagName accessible:&supported];
  if (supported) {
    LynxShadowNode* node;
    NSInteger sign = 0;
    if (clazz) {
      node = [[clazz alloc] initWithSign:sign tagName:tagName];
    }
    if (node != nil) {
      result |= LynxShadowNodeTypeCustom;
      if ([node isVirtual]) {
        result |= LynxShadowNodeTypeVirtual;
      }
    } else {
      result |= LynxShadowNodeTypeCommon;
    }
  }
  return result;
}

#pragma mark - list

- (void)updateScrollInfo:(NSInteger)containerID
         estimatedOffset:(float)estimatedOffset
                  smooth:(bool)smooth
               scrolling:(bool)scrolling {
  LynxUI* ui = [self findUIBySign:containerID];
  if ([ui isKindOfClass:LynxUIListContainer.class]) {
    LynxUIListContainer* list = (LynxUIListContainer*)ui;
    [list updateScrollInfoWithEstimatedOffset:estimatedOffset smooth:smooth scrolling:scrolling];
  }
}

- (void)insertListComponent:(NSInteger)listSign componentSign:(NSInteger)componentSign {
  LynxUI* child = _uiHolder[[NSNumber numberWithInteger:componentSign]];
  LynxUI* list = _uiHolder[[NSNumber numberWithInteger:listSign]];
  if ([child isKindOfClass:LynxUIComponent.class] &&
      [list isKindOfClass:LynxUIListContainer.class]) {
    LynxUIComponent* component = (LynxUIComponent*)child;
    LynxUIListContainer* listContainer = (LynxUIListContainer*)list;
    [listContainer insertListComponent:component];
  }
}

- (void)removeListComponent:(NSInteger)listSign componentSign:(NSInteger)componentSign {
  LynxUI* child = _uiHolder[[NSNumber numberWithInteger:componentSign]];
  LynxUI* list = _uiHolder[[NSNumber numberWithInteger:listSign]];
  if ([child isKindOfClass:LynxUIComponent.class]) {
    LynxUIComponent* component = (LynxUIComponent*)child;
    LLogInfo(@" removeListComponent remove view: %p -> %p, %@", component, component.view.superview,
             [component itemKey]);
  }

  if (child.view.superview.superview == list.view) {
    [child.view.superview removeFromSuperview];
    [child.view removeFromSuperview];
  }
}

- (void)updateContentOffsetForListContainer:(NSInteger)containerID
                                contentSize:(float)contentSize
                                     deltaX:(float)deltaX
                                     deltaY:(float)deltaY {
  LynxUI* ui = [self findUIBySign:containerID];
  if ([ui isKindOfClass:LynxUIListContainer.class]) {
    LynxUIListContainer* listContainer = (LynxUIListContainer*)ui;
    listContainer.needAdjustContentOffset = YES;
    listContainer.targetContentSize = contentSize;
    listContainer.targetDelta =
        CGPointMake(listContainer.targetDelta.x + deltaX, listContainer.targetDelta.y + deltaY);
  }
}

#pragma mark list native storage
- (void)listWillReuseNode:(NSInteger)sign withItemKey:(NSString*)itemKey {
  LynxUI* node = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (node) {
    if ([node.parent isKindOfClass:LynxUICollection.class] ||
        [node.parent isKindOfClass:LynxUIListContainer.class]) {
      [node onListCellPrepareForReuse:itemKey withList:node.parent];
    }
  }
}

- (void)listCellWillAppear:(NSInteger)sign withItemKey:(NSString*)itemKey {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui && [ui.parent isKindOfClass:LynxUIListContainer.class]) {
    [ui onListCellAppear:itemKey withList:ui.parent];
  }
}

- (void)ListCellDisappear:(NSInteger)sign exist:(BOOL)isExist withItemKey:(NSString*)itemKey {
  LynxUI* ui = _uiHolder[[NSNumber numberWithInteger:sign]];
  if (ui && [ui.parent isKindOfClass:LynxUIListContainer.class]) {
    [ui onListCellDisappear:itemKey exist:isExist withList:ui.parent];
  }
}

#pragma mark gesture

- (void)initNewGestureInUIThread:(BOOL)enableNewGesture {
  if ([NSThread isMainThread]) {
    _enableNewGesture = enableNewGesture;
    if (enableNewGesture && !_gestureArenaManager) {
      _gestureArenaManager = [[LynxGestureArenaManager alloc] init];
      [_uiContext.eventHandler setGestureArenaManagerAndGetIndex:_gestureArenaManager];
    }
  } else {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      if (strongSelf) {
        strongSelf->_enableNewGesture = enableNewGesture;
        if (strongSelf->_enableNewGesture && !strongSelf->_gestureArenaManager) {
          strongSelf->_gestureArenaManager = [[LynxGestureArenaManager alloc] init];
          [strongSelf->_uiContext.eventHandler
              setGestureArenaManagerAndGetIndex:strongSelf->_gestureArenaManager];
        }
      }
    });
  }
}

- (void)resetGestureArenaInUIThread {
  if ([NSThread isMainThread]) {
    _gestureArenaManager = nil;
  } else {
    // Keep reference count until ui thread execution
    __block LynxGestureArenaManager* gestureManager = _gestureArenaManager;
    dispatch_async(dispatch_get_main_queue(), ^{
      gestureManager = nil;
    });
  }
}

- (NSInteger)getRootSign {
  return _rootUI.rootView.tag;
}

@end
