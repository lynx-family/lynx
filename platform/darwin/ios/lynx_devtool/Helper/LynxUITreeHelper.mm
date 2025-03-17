// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/DevToolOverlayDelegate.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/UIView+Lynx.h>
#import <LynxDevtool/LynxScreenCastHelper.h>
#import <LynxDevtool/LynxUITreeHelper.h>
#import <objc/message.h>
#import <objc/runtime.h>

#include <vector>

typedef NS_ENUM(NSUInteger, BoxModelOffset) {
  PAD_LEFT = 0,
  PAD_TOP,
  PAD_RIGHT,
  PAD_BOTTOM,
  BORDER_LEFT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  MARGIN_LEFT,
  MARGIN_TOP,
  MARGIN_RIGHT,
  MARGIN_BOTTOM,
  LAYOUT_LEFT,
  LAYOUT_TOP,
  LAYOUT_RIGHT,
  LAYOUT_BOTTOM,
};

@implementation LynxUITreeHelper {
  __weak LynxUIOwner* _uiOwner;
}

- (void)attachLynxUIOwner:(nullable LynxUIOwner*)uiOwner {
  _uiOwner = uiOwner;
}

- (CGPoint)convertPointFromScreen:(CGPoint)point ToView:(UIView*)view {
  return [[LynxUIKitAPIAdapter getKeyWindow] convertPoint:point toView:view];
}

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y fromUI:(int)uiSign mode:(NSString*)mode {
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  if (uiOwner) {
    UIView* view;
    if (uiSign == 0) {
      // find node from LynxView
      view = uiOwner.rootUI.rootView;
    } else {
      // find node from overlay view
      LynxUI* ui = [uiOwner findUIBySign:uiSign];
      if (ui != NULL) {
        view = ui.view;
      } else {
        return 0;
      }
    }
    UIEvent* event = nil;
    CGPoint point_to_view;
    if ([mode isEqualToString:ScreenshotModeLynxView]) {
      point_to_view = CGPointMake(x, y);
    } else {  // fullscreen
      point_to_view = [self convertPointFromScreen:CGPointMake(x, y) ToView:view];
    }
    UIView* target = [view hitTest:point_to_view withEvent:event];
    if (target && target.lynxSign) {
      return [target.lynxSign intValue];
    }
  }
  return 0;
}

- (CGRect)getRectToWindow {
  CGRect res;
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  if (uiOwner == nil) {
    return res;
  }
  LynxUI* ui = [uiOwner findUIBySign:[uiOwner getRootSign]];
  if (ui != NULL) {
    CGRect re = [ui getRectToWindow];
    int scale = UIScreen.mainScreen.scale;
    res.origin.x = re.origin.x * scale;
    res.origin.y = re.origin.y * scale;
    res.size.width = re.size.width * scale;
    res.size.height = re.size.height * scale;
  }
  return res;
}

- (CGPoint)getViewLocationOnScreen {
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  UIView* view = [[uiOwner uiContext] rootView];
  if (view && view.window) {
    CGPoint pointInWindow = [view convertPoint:CGPointMake(0, 0) toView:nil];
    CGPoint pointInScreen = [view.window convertPoint:pointInWindow toWindow:nil];
    return pointInScreen;
  }
  return CGPointMake(-1, -1);
}

- (NSArray<NSNumber*>*)getVisibleOverlayView {
  NSArray<NSNumber*>* array = [DevToolOverlayDelegate.sharedInstance getAllVisibleOverlaySign];
  return array;
}

- (void)scrollIntoView:(int)node_id {
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  LynxUI* ui = [uiOwner findUIBySign:node_id];
  [ui scrollIntoViewWithSmooth:false blockType:@"center" inlineType:@"center" callback:nil];
}

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString*)mode {
  int node_id = 0;
  if ([mode isEqualToString:ScreenshotModeFullScreen]) {
    NSArray<NSNumber*>* overlays = [self getVisibleOverlayView];
    NSEnumerator* enumerator = [overlays reverseObjectEnumerator];
    NSNumber* num;
    while ((num = [enumerator nextObject]) != nil) {
      node_id = [self findNodeIdForLocationWithX:x withY:y fromUI:[num intValue] mode:mode];
      // overlay node's size is window size and it has one and only
      // one child if id == overlays[i], it means point is not in child so
      // not in overlay Under this circumstances,we need reset id to 0
      if (node_id != 0 && node_id != [num intValue]) {
        return node_id;
      } else {
        node_id = 0;
      }
    }
    node_id =
        node_id != 0 ? node_id : [self findNodeIdForLocationWithX:x withY:y fromUI:0 mode:mode];
  } else {  // lynxview
    node_id = [self findNodeIdForLocationWithX:x withY:y fromUI:0 mode:mode];
  }
  return node_id;
}

- (NSString*)dictionaryToJson:(NSMutableDictionary*)dict {
  NSString* res;
  NSError* error;
  NSData* jsonData = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&error];
  if (jsonData) {
    res = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];
  }
  return res;
}

/*
 * get LynxUITree Recursively
 *
 * Parameter:
 *  ui:LynxUI, the ui to traverse from
 *
 * Return value:
 * the Dictionary of ui, store ui's name/id/frame/children, and it's
 * children is also Dictionary like itself
 */
- (NSMutableDictionary*)getLynxUITreeRecursive:(LynxUI*)ui {
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
  [dict setValue:NSStringFromClass([ui class]) forKey:@"name"];
  [dict setValue:@(ui.sign) forKey:@"id"];
  [dict setValue:@[
    @(ui.frame.origin.x), @(ui.frame.origin.y), @(ui.frame.size.width), @(ui.frame.size.height)
  ]
          forKey:@"frame"];
  NSMutableArray* array = [[NSMutableArray alloc] init];
  for (LynxUI* child in ui.children) {
    [array addObject:[self getLynxUITreeRecursive:child]];
  }
  [dict setValue:array forKey:@"children"];
  return dict;
}

- (NSString*)getLynxUITree {
  NSString* res;
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  if (uiOwner) {
    LynxUI* root = (LynxUI*)[uiOwner rootUI];
    if (root) {
      NSMutableDictionary* tree = [self getLynxUITreeRecursive:root];
      res = [self dictionaryToJson:tree];
    }
  }
  return res;
}

/*
 * get all properties key/value from current class to given super class
 *
 * Parameters:
 * obj : the object to get all properties
 * type: "view"/"ui/layer"
 * for "view", traverse from current class to UIView class
 * for "ui" , traverse from current class to LynxComponent
 * for "layer", traverse from current class to CALayer
 *
 * return value:
 * the NSMutableDictionary* which store properties key-value
 */
- (NSMutableDictionary*)GetAllPropertyAndValues:(NSObject*)obj withType:(NSString*)type {
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];

  // get all properties from [currentClass, stopClass)
  Class currentClass = [obj class];
  Class stopClass;
  if ([type isEqualToString:@"view"]) {
    stopClass = [UIResponder class];
  } else if ([type isEqualToString:@"ui"] || [type isEqualToString:@"layer"]) {
    stopClass = [NSObject class];
  }

  while (currentClass != stopClass) {
    unsigned int propertyCount;
    objc_property_t* properties = class_copyPropertyList(currentClass, &propertyCount);
    for (unsigned int i = 0; i < propertyCount; i++) {
      objc_property_t property = properties[i];
      @try {
        NSString* propertyName = [NSString stringWithUTF8String:property_getName(property)];
        NSObject* propertyValue = [obj valueForKey:propertyName];
        if (propertyValue) {
          [dict setValue:propertyValue.description forKey:propertyName];
        }
      } @catch (NSException* exception) {
      }
    }

    /*
     *the object properties get by class_copyPropertyList runtime method which won't be released
     *automatically by ARC must free the properties with free()
     */
    free(properties);
    currentClass = [currentClass superclass];
  }
  return dict;
}

/*
 * get ui's editable property
 * actually there are 6 editable properties. In addition to frame/border/margin/visible,
 * background-color/border-color are also editable, which don't need show value on the frontend
 * just show hint about input format.
 */
- (NSMutableDictionary*)getEditablePropertyValueOfUI:(LynxUI*)ui {
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
  [dict setValue:@[
    @(ui.backgroundManager.borderWidth.top), @(ui.backgroundManager.borderWidth.right),
    @(ui.backgroundManager.borderWidth.bottom), @(ui.backgroundManager.borderWidth.left)
  ]
          forKey:@"border"];
  [dict setValue:@[ @(ui.margin.top), @(ui.margin.right), @(ui.margin.bottom), @(ui.margin.left) ]
          forKey:@"margin"];
  [dict setValue:@[
    @(ui.frame.origin.x), @(ui.frame.origin.y), @(ui.frame.size.width), @(ui.frame.size.height)
  ]
          forKey:@"frame"];
  [dict setValue:@(!ui.view.isHidden) forKey:@"visible"];
  return dict;
}

- (NSString*)getUINodeInfo:(int)id {
  NSString* res;
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  if (uiOwner) {
    LynxUI* ui = [uiOwner findUIBySign:(int)id];
    if (ui != nil) {
      NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
      [dict setValue:@(ui.sign) forKey:@"id"];
      NSMutableDictionary* editableProps = [self getEditablePropertyValueOfUI:ui];
      [dict setValue:editableProps forKey:@"editableProps"];

      NSMutableDictionary* uiDictionary = [[NSMutableDictionary alloc] init];
      [uiDictionary setValue:NSStringFromClass([ui class]) forKey:@"name"];
      [uiDictionary setValue:[self GetAllPropertyAndValues:ui withType:@"ui"]
                      forKey:@"readonlyProps"];
      [dict setValue:uiDictionary forKey:@"ui"];

      NSMutableDictionary* viewDictionary = [[NSMutableDictionary alloc] init];
      [viewDictionary setValue:NSStringFromClass(ui.view.class) forKey:@"name"];
      [viewDictionary setValue:[self GetAllPropertyAndValues:ui.view withType:@"view"]
                        forKey:@"readonlyProps"];
      [dict setValue:viewDictionary forKey:@"view"];

      NSMutableDictionary* layerDictionary = [[NSMutableDictionary alloc] init];
      [layerDictionary setValue:[self GetAllPropertyAndValues:ui.view.layer withType:@"layer"]
                         forKey:@"CALayer"];
      if (ui.backgroundManager.backgroundLayer) {
        [layerDictionary setValue:[self GetAllPropertyAndValues:ui.backgroundManager.backgroundLayer
                                                       withType:@"layer"]
                           forKey:@"LynxBackgroundSubBackgroundLayer"];
      }
      if (ui.backgroundManager.borderLayer) {
        [layerDictionary setValue:[self GetAllPropertyAndValues:ui.backgroundManager.borderLayer
                                                       withType:@"layer"]
                           forKey:@"LynxBorderLayer"];
      }
      [dict setValue:layerDictionary forKey:@"layers"];

      res = [self dictionaryToJson:dict];
    }
  }
  return res;
}

- (UIColor*)parseColorString:(NSString*)colorString {
  @try {
    NSString* redString = [colorString substringWithRange:NSMakeRange(1, 2)];
    NSString* greenString = [colorString substringWithRange:NSMakeRange(3, 2)];
    NSString* blueString = [colorString substringWithRange:NSMakeRange(5, 2)];
    NSString* alphaString = [colorString substringWithRange:NSMakeRange(7, 2)];
    unsigned int r, g, b, a;
    [[NSScanner scannerWithString:redString] scanHexInt:&r];
    [[NSScanner scannerWithString:greenString] scanHexInt:&g];
    [[NSScanner scannerWithString:blueString] scanHexInt:&b];
    [[NSScanner scannerWithString:alphaString] scanHexInt:&a];
    return [UIColor colorWithRed:((float)r / 255.0f)
                           green:((float)g / 255.0f)
                            blue:((float)b / 255.0f)
                           alpha:((float)a / 255.0f)];
  } @catch (NSException* exception) {
    LLogError(@"UITree Debug:parse color value string fail");
  }
  return NULL;
}

/* stringToArrayValue: decode value string of frame/border/margin style to float array
 *
 * parameter:
 * 4 num string, such as "3,2, 3, 5", split num by "," and there may be whitespace
 *
 * return value: array
 *[num1, num2, num3, num4]
 */
- (std::vector<float>)stringToArrayValue:(NSString*)strValue {
  std::vector<float> res;
  @try {
    strValue = [strValue stringByReplacingOccurrencesOfString:@" " withString:@""];
    NSArray* valueArray = [strValue componentsSeparatedByString:@","];
    res.push_back([valueArray[0] floatValue]);
    res.push_back([valueArray[1] floatValue]);
    res.push_back([valueArray[2] floatValue]);
    res.push_back([valueArray[3] floatValue]);
    return res;
  } @catch (NSException* exception) {
    LLogError(@"UITree Debug:parse frame value string fail");
  }
  return res;
}

- (int)setFrame:(NSString*)styleContent ofUI:(LynxUI*)ui {
  std::vector<float> frame_value = [self stringToArrayValue:styleContent];
  if (frame_value.size() == 4) {
    [ui setFrame:CGRectMake(frame_value[0], frame_value[1], frame_value[2], frame_value[3])];
    [ui frameDidChange];
    return 0;
  } else {
    return -1;
  }
}

- (int)setMargin:(NSString*)styleContent ofUI:(LynxUI*)ui {
  std::vector<float> margin_value = [self stringToArrayValue:styleContent];
  if (margin_value.size() == 4) {
    [ui setFrame:CGRectMake((ui.frame.origin.x - ui.margin.left + margin_value[3]),
                            (ui.frame.origin.y - ui.margin.top + margin_value[0]),
                            ui.frame.size.width, ui.frame.size.height)];
    [ui updateFrame:ui.frame
                withPadding:ui.padding
                     border:ui.border
                     margin:UIEdgeInsetsMake(margin_value[0], margin_value[3], margin_value[2],
                                             margin_value[1])
        withLayoutAnimation:true];
    [ui frameDidChange];
    return 0;
  } else {
    return -1;
  }
}

- (int)setBorder:(NSString*)styleContent ofUI:(LynxUI*)ui {
  std::vector<float> border_value = [self stringToArrayValue:styleContent];
  if (border_value.size() == 4) {
    ui.backgroundManager.borderWidth =
        UIEdgeInsetsMake(border_value[0], border_value[3], border_value[2], border_value[1]);
    [ui propsDidUpdate];
    return 0;
  } else {
    return -1;
  }
}

- (int)setBackgroundColor:(NSString*)styleContent ofUI:(LynxUI*)ui {
  UIColor* color = [self parseColorString:styleContent];
  if (color != NULL) {
    ui.backgroundManager.backgroundColor = color;
    [ui propsDidUpdate];
    return 0;
  } else {
    return -1;
  }
}
- (int)setBorderColor:(NSString*)styleContent ofUI:(LynxUI*)ui {
  UIColor* color = [self parseColorString:styleContent];
  if (color != NULL) {
    ui.backgroundManager.borderTopColor = color;
    ui.backgroundManager.borderLeftColor = color;
    ui.backgroundManager.borderBottomColor = color;
    ui.backgroundManager.borderRightColor = color;
    [ui propsDidUpdate];
    return 0;
  } else {
    return -1;
  }
}

- (int)setVisibility:(NSString*)styleContent ofUI:(LynxUI*)ui {
  if ([styleContent isEqualToString:@"true"]) {
    [ui.view setHidden:false];
    return 0;
  } else if ([styleContent isEqualToString:@"false"]) {
    [ui.view setHidden:true];
    return 0;
  } else {
    return -1;
  }
}

- (int)setUIStyle:(int)id withStyleName:(NSString*)name withStyleContent:(NSString*)content {
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  if (uiOwner) {
    LynxUI* ui = [uiOwner findUIBySign:(int)id];
    if (ui) {
      content = [content stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];
      if ([name isEqualToString:@"frame"]) {
        return [self setFrame:content ofUI:ui];
      } else if ([name isEqualToString:@"margin"]) {
        return [self setMargin:content ofUI:ui];
      } else if ([name isEqualToString:@"border"]) {
        return [self setBorder:content ofUI:ui];
      } else if ([name isEqualToString:@"background-color"]) {
        return [self setBackgroundColor:content ofUI:ui];
      } else if ([name isEqualToString:@"border-color"]) {
        return [self setBorderColor:content ofUI:ui];
      } else if ([name isEqualToString:@"visible"]) {
        return [self setVisibility:content ofUI:ui];
      } else {
        LLogError(@"unsupported style %@", name);
      }
    }
  }
  return -1;
}

- (NSArray<NSNumber*>*)getTransformValue:(NSInteger)sign
               withPadBorderMarginLayout:(NSArray<NSNumber*>*)arrayLayout {
  std::vector<float> padBorderMarginLayout = NSArrayToVector(arrayLayout);
  std::vector<float> res;
  __strong typeof(_uiOwner) uiOwner = _uiOwner;
  LynxUI* ui = [uiOwner findUIBySign:sign];
  if (ui != nil) {
    for (int i = 0; i < 4; i++) {
      TransOffset arr;
      if (i == 0) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[PAD_LEFT] +
                                            padBorderMarginLayout[BORDER_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[PAD_RIGHT] -
                                            padBorderMarginLayout[BORDER_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[PAD_TOP] +
                                            padBorderMarginLayout[BORDER_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[PAD_BOTTOM] -
                                            padBorderMarginLayout[BORDER_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else if (i == 1) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[BORDER_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[BORDER_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[BORDER_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[BORDER_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else if (i == 2) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else {
        arr = [ui getTransformValueWithLeft:-padBorderMarginLayout[MARGIN_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:padBorderMarginLayout[MARGIN_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:-padBorderMarginLayout[MARGIN_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:padBorderMarginLayout[MARGIN_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      }
      res.push_back(arr.left_top.x);
      res.push_back(arr.left_top.y);
      res.push_back(arr.right_top.x);
      res.push_back(arr.right_top.y);
      res.push_back(arr.right_bottom.x);
      res.push_back(arr.right_bottom.y);
      res.push_back(arr.left_bottom.x);
      res.push_back(arr.left_bottom.y);
    }
  }

  NSArray<NSNumber*>* result = VectorToNSArray(res);
  return result;
}

std::vector<float> NSArrayToVector(NSArray<NSNumber*>* array) {
  std::vector<float> result;
  result.reserve(array.count);
  for (NSNumber* num in array) {
    result.push_back(num.floatValue);
  }
  return result;
}

NSArray<NSNumber*>* VectorToNSArray(const std::vector<float>& vec) {
  NSMutableArray<NSNumber*>* result = [NSMutableArray arrayWithCapacity:vec.size()];
  for (float value : vec) {
    [result addObject:@(value)];
  }
  return [result copy];
}

@end
