// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static com.lynx.tasm.behavior.PropertyIDConstants.*;

import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.mapbuffer.DynamicFromMapBuffer;
import com.lynx.react.bridge.mapbuffer.MapBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableMapBufferWrapper;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.text.BaseTextShadowNode;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.list.AbsLynxList;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import java.util.Iterator;

public class CSSPropertySetter {
  public static class UIPaintStyles {
    // background
    ReadableArray mBackgroundClip; // BackgroundClip
    int mBackgroundColor = android.graphics.Color.TRANSPARENT; // BackgroundColor
    ReadableArray mBackgroundImage; // BackgroundImage
    ReadableArray mBackgroundOrigin; // BackgroundOrigin
    ReadableArray mBackgroundPosition; // BackgroundPosition
    ReadableArray mBackgroundRepeat; // BackgroundRepeat
    ReadableArray mBackgroundSize; // BackgroundSize

    // mask-image
    ReadableArray mMaskImage; // MaskImage
    ReadableArray mMaskOrigin; // MaskOrigin
    ReadableArray mMaskRepeat; // MaskRepeat
    ReadableArray mMaskSize; // MaskSize
    ReadableArray mMaskPosition; // mMaskPosition;
    ReadableArray mMaskClip; // MaskClip

    // border-radius
    ReadableArray mBorderRadius;
    ReadableArray mBorderTopLeftRadius;
    ReadableArray mBorderTopRightRadius;
    ReadableArray mBorderBottomRightRadius;
    ReadableArray mBorderBottomLeftRadius;

    // border-style
    int mBorderStyle = -1;
    int mBorderLeftStyle = -1;
    int mBorderRightStyle = -1;
    int mBorderTopStyle = -1;
    int mBorderBottomStyle = -1;

    // border-width
    int mBorderWidth;
    int mBorderLeftWidth;
    int mBorderRightWidth;
    int mBorderTopWidth;
    int mBorderBottomWidth;

    // border-color
    Integer mBorderColor;
    Integer mBorderLeftColor;
    Integer mBorderRightColor;
    Integer mBorderTopColor;
    Integer mBorderBottomColor;

    // overflow
    Integer mOverflow;
    Integer mOverflowX;
    Integer mOverflowY;

    // outline
    int mOutlineColor = android.graphics.Color.BLACK;
    float mOutlineWidth;
    int mOutlineStyle;

    float mFontSize;

    int mDirection = StyleConstants.DIRECTION_LTR;

    // transform
    ReadableArray mTransformOrigin;
    ReadableArray mPerspective;
    ReadableArray mBoxShadow;
    ReadableArray mTransform;

    int mImageRendering;

    ReadableArray mClipPath;
    ReadableArray mFilter;

    // layout animation (to be deprecated!)
    double mLayoutAnimationCreateDelay;
    double mLayoutAnimationCreateDuration;
    int mLayoutAnimationCreateProperty;
    ReadableArray mLayoutAnimationCreateTimingFunction;
    double mLayoutAnimationDeleteDelay;
    double mLayoutAnimationDeleteDuration;
    int mLayoutAnimationDeleteProperty;
    ReadableArray mLayoutAnimationDeleteTimingFunction;
    double mLayoutAnimationUpdateDelay;
    double mLayoutAnimationUpdateDuration;
    ReadableArray mLayoutAnimationUpdateTimingFunction;

    float mOpacity;
    int mVisibility;
  }
  public static void updateStyles(LynxBaseUI lynxUI, MapBuffer initialStyles) {
    if (initialStyles == null) {
      return;
    }
    UIPaintStyles uiPaintStyles = lynxUI.getOrCreateUIPaintStyles();
    if (uiPaintStyles == null) {
      return;
    }
    Iterator<MapBuffer.Entry> iter = initialStyles.iterator();
    while (iter.hasNext()) {
      MapBuffer.Entry entry = iter.next();
      int key = entry.getKey();
      switch (key) {
        case BackgroundClip:
          ReadableArray bgClip = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundClip(bgClip);
          uiPaintStyles.mBackgroundClip = bgClip;
          break;
        case BackgroundColor:
          int bgColor = entry.getInt();
          lynxUI.setBackgroundColor(bgColor);
          uiPaintStyles.mBackgroundColor = bgColor;
          break;
        case BackgroundImage:
          ReadableArray bgImage = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundImage(bgImage);
          uiPaintStyles.mBackgroundImage = bgImage;
          break;
        case BackgroundOrigin:
          ReadableArray bgOrigin = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundOrigin(bgOrigin);
          uiPaintStyles.mBackgroundOrigin = bgOrigin;
          break;
        case BackgroundPosition:
          ReadableArray bgPosition = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundPosition(bgPosition);
          uiPaintStyles.mBackgroundPosition = bgPosition;
          break;
        case BackgroundRepeat:
          ReadableArray bgRepeat = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundRepeat(bgRepeat);
          uiPaintStyles.mBackgroundRepeat = bgRepeat;
          break;
        case BackgroundSize:
          ReadableArray bgSize = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBackgroundSize(bgSize);
          uiPaintStyles.mBackgroundSize = bgSize;
          break;
        case MaskImage:
          ReadableArray maskImage = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskImage(maskImage);
          uiPaintStyles.mMaskImage = maskImage;
          break;
        case MaskOrigin:
          ReadableArray maskOrigin = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskOrigin(maskOrigin);
          uiPaintStyles.mMaskOrigin = maskOrigin;
          break;
        case MaskRepeat:
          ReadableArray maskRepeat = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskRepeat(maskRepeat);
          uiPaintStyles.mMaskRepeat = maskRepeat;
          break;
        case MaskSize:
          ReadableArray maskSize = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskSize(maskSize);
          uiPaintStyles.mMaskSize = maskSize;
          break;
        case MaskPosition:
          ReadableArray maskPosition = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskPosition(maskPosition);
          uiPaintStyles.mMaskPosition = maskPosition;
          break;
        case MaskClip:
          ReadableArray maskClip = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setMaskClip(maskClip);
          uiPaintStyles.mMaskClip = maskClip;
          break;
        case BorderRadius:
          ReadableArray borderRadius = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBorderRadius(0, borderRadius);
          uiPaintStyles.mBorderRadius = borderRadius;
          break;
        case BorderTopLeftRadius:
          ReadableArray borderTopLeftRadius = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBorderRadius(1, borderTopLeftRadius);
          uiPaintStyles.mBorderTopLeftRadius = borderTopLeftRadius;
          break;
        case BorderTopRightRadius:
          ReadableArray borderTopRightRadius = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBorderRadius(2, borderTopRightRadius);
          uiPaintStyles.mBorderTopRightRadius = borderTopRightRadius;
          break;
        case BorderBottomRightRadius:
          ReadableArray borderBottomRightRadius =
              new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBorderRadius(3, borderBottomRightRadius);
          uiPaintStyles.mBorderBottomRightRadius = borderBottomRightRadius;
          break;
        case BorderBottomLeftRadius:
          ReadableArray borderBottomLeftRadius = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBorderRadius(4, borderBottomLeftRadius);
          uiPaintStyles.mBorderBottomLeftRadius = borderBottomLeftRadius;
          break;
        case Overflow:
          int overFlow = entry.getInt();
          lynxUI.setOverflow(overFlow);
          uiPaintStyles.mOverflow = overFlow;
          break;
        case OverflowX:
          int overflowX = entry.getInt();
          lynxUI.setOverflowX(overflowX);
          uiPaintStyles.mOverflowX = overflowX;
          break;
        case OverflowY:
          int overflowY = entry.getInt();
          lynxUI.setOverflowY(overflowY);
          uiPaintStyles.mOverflowY = overflowY;
          break;
        case BorderStyle:
          int borderStyle = entry.getInt();
          lynxUI.setBorderStyle(0, borderStyle);
          uiPaintStyles.mBorderStyle = borderStyle;
          break;
        case BorderLeftStyle:
          int borderLeftStyle = entry.getInt();
          lynxUI.setBorderStyle(1, borderLeftStyle);
          uiPaintStyles.mBorderLeftStyle = borderLeftStyle;
          break;
        case BorderRightStyle:
          int borderRightStyle = entry.getInt();
          lynxUI.setBorderStyle(2, borderRightStyle);
          uiPaintStyles.mBorderRightStyle = borderRightStyle;
          break;
        case BorderTopStyle:
          int borderTopStyle = entry.getInt();
          lynxUI.setBorderStyle(3, borderTopStyle);
          uiPaintStyles.mBorderTopStyle = borderTopStyle;
          break;
        case BorderBottomStyle:
          int borderBottomStyle = entry.getInt();
          lynxUI.setBorderStyle(4, borderBottomStyle);
          uiPaintStyles.mBorderBottomStyle = borderBottomStyle;
          break;
        case BorderWidth:
          int borderWidth = (int) entry.getDouble();
          lynxUI.setBorderWidth(0, borderWidth);
          uiPaintStyles.mBorderWidth = borderWidth;
          break;
        case BorderLeftWidth:
          int borderLeftWidth = (int) entry.getDouble();
          lynxUI.setBorderWidth(1, borderLeftWidth);
          uiPaintStyles.mBorderLeftWidth = borderLeftWidth;
          break;
        case BorderRightWidth:
          int borderRightWidth = (int) entry.getDouble();
          lynxUI.setBorderWidth(2, borderRightWidth);
          uiPaintStyles.mBorderRightWidth = borderRightWidth;
          break;
        case BorderTopWidth:
          int borderTopWidth = (int) entry.getDouble();
          lynxUI.setBorderWidth(3, borderTopWidth);
          uiPaintStyles.mBorderTopWidth = borderTopWidth;
          break;
        case BorderBottomWidth:
          int borderBottomWidth = (int) entry.getDouble();
          lynxUI.setBorderWidth(4, borderBottomWidth);
          uiPaintStyles.mBorderBottomWidth = borderBottomWidth;
          break;
        // TODO(nihao.royal) case BorderColor
        case BorderLeftColor:
          int borderLeftColor = entry.getInt();
          lynxUI.setBorderColor(0, borderLeftColor);
          uiPaintStyles.mBorderLeftColor = borderLeftColor;
          break;
        case BorderRightColor:
          int borderRightColor = entry.getInt();
          lynxUI.setBorderColor(1, borderRightColor);
          uiPaintStyles.mBorderRightColor = borderRightColor;
          break;
        case BorderTopColor:
          int borderTopColor = entry.getInt();
          lynxUI.setBorderColor(2, borderTopColor);
          uiPaintStyles.mBorderTopColor = borderTopColor;
          break;
        case BorderBottomColor:
          int borderBottomColor = entry.getInt();
          lynxUI.setBorderColor(3, borderBottomColor);
          uiPaintStyles.mBorderBottomColor = borderBottomColor;
          break;
        case OutlineColor:
          int outlineColor = entry.getInt();
          lynxUI.setOutlineColor(entry.getInt());
          uiPaintStyles.mOutlineColor = outlineColor;
          break;
        case OutlineWidth:
          float outlineWidth = (float) entry.getDouble();
          lynxUI.setOutlineWidth(outlineWidth);
          uiPaintStyles.mOutlineWidth = outlineWidth;
          break;
        case OutlineStyle:
          int outlineStyle = entry.getInt();
          lynxUI.setOutlineStyle(outlineStyle);
          uiPaintStyles.mOutlineStyle = outlineStyle;
          break;
        case FontSize:
          float fontSize = (float) entry.getInt();
          lynxUI.setFontSize(fontSize);
          uiPaintStyles.mFontSize = fontSize;
          break;
        case CaretColor:
          lynxUI.setCaretColor(entry.getString());
          break;
        case Position:
          lynxUI.setCSSPosition(entry.getInt());
          break;
        case Direction:
          int direction = entry.getInt();
          lynxUI.setLynxDirection(direction);
          uiPaintStyles.mDirection = direction;
          break;
        case TransformOrigin:
          ReadableArray transformOrigin = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setTransformOrigin(transformOrigin);
          uiPaintStyles.mTransformOrigin = transformOrigin;
          break;
        case Perspective:
          ReadableArray perspective = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setPerspective(perspective);
          uiPaintStyles.mPerspective = perspective;
          break;
        case BoxShadow:
          ReadableArray boxShadow = new ReadableMapBufferWrapper(entry.getMapBuffer());
          lynxUI.setBoxShadow(boxShadow);
          uiPaintStyles.mBoxShadow = boxShadow;
          break;
        case ImageRendering:
          int imageRendering = entry.getInt();
          lynxUI.setImageRendering(imageRendering);
          uiPaintStyles.mImageRendering = imageRendering;
          break;
        // from lynxUI
        case ClipPath:
          if (lynxUI instanceof LynxUI) {
            ReadableArray clipPath = new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI).setClipPath(clipPath);
            uiPaintStyles.mClipPath = clipPath;
          }
          break;
        case EnterTransitionName:
          if (lynxUI instanceof LynxUI) {
            ((LynxUI) lynxUI)
                .setEnterTransitionName(new ReadableMapBufferWrapper(entry.getMapBuffer()));
          }
          break;
        case ExitTransitionName:
          if (lynxUI instanceof LynxUI) {
            ((LynxUI) lynxUI)
                .setExitTransitionName(new ReadableMapBufferWrapper(entry.getMapBuffer()));
          }
          break;
        case Filter:
          if (lynxUI instanceof LynxUI) {
            ReadableArray filter = new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI).setFilter(filter);
            uiPaintStyles.mFilter = filter;
          }
          break;
        case LayoutAnimationCreateDelay:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationCreateDelay = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationCreateDelay(layoutAnimationCreateDelay);
            uiPaintStyles.mLayoutAnimationCreateDelay = layoutAnimationCreateDelay;
          }
          break;
        case LayoutAnimationCreateDuration:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationCreateDuration = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationCreateDuration(layoutAnimationCreateDuration);
            uiPaintStyles.mLayoutAnimationCreateDuration = layoutAnimationCreateDuration;
          }
          break;
        case LayoutAnimationCreateProperty:
          if (lynxUI instanceof LynxUI) {
            int layoutAnimationCreateProperty = entry.getInt();
            ((LynxUI) lynxUI).setLayoutAnimationCreateProperty(layoutAnimationCreateProperty);
            uiPaintStyles.mLayoutAnimationCreateProperty = layoutAnimationCreateProperty;
          }
          break;
        case LayoutAnimationCreateTimingFunction:
          if (lynxUI instanceof LynxUI) {
            ReadableArray layoutAnimationCreateTimingFunction =
                new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI)
                .setLayoutAnimationCreateTimingFunc(layoutAnimationCreateTimingFunction);
            uiPaintStyles.mLayoutAnimationCreateTimingFunction =
                layoutAnimationCreateTimingFunction;
          }
          break;
        case LayoutAnimationDeleteDelay:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationDeleteDelay = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationDeleteDelay(layoutAnimationDeleteDelay);
            uiPaintStyles.mLayoutAnimationDeleteDelay = layoutAnimationDeleteDelay;
          }
          break;
        case LayoutAnimationDeleteDuration:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationDeleteDuration = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationDeleteDuration(layoutAnimationDeleteDuration);
            uiPaintStyles.mLayoutAnimationDeleteDuration = layoutAnimationDeleteDuration;
          }
          break;
        case LayoutAnimationDeleteProperty:
          if (lynxUI instanceof LynxUI) {
            int layoutAnimationDeleteProperty = entry.getInt();
            ((LynxUI) lynxUI).setLayoutAnimationDeleteProperty(layoutAnimationDeleteProperty);
            uiPaintStyles.mLayoutAnimationDeleteProperty = layoutAnimationDeleteProperty;
          }
          break;
        case LayoutAnimationDeleteTimingFunction:
          if (lynxUI instanceof LynxUI) {
            ReadableArray layoutAnimationDeleteTimingFunction =
                new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI)
                .setLayoutAnimationDeleteTimingFunc(layoutAnimationDeleteTimingFunction);
            uiPaintStyles.mLayoutAnimationDeleteTimingFunction =
                layoutAnimationDeleteTimingFunction;
          }
          break;
        case LayoutAnimationUpdateDelay:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationUpdateDelay = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationUpdateDelay(layoutAnimationUpdateDelay);
            uiPaintStyles.mLayoutAnimationUpdateDelay = layoutAnimationUpdateDelay;
          }
          break;
        case LayoutAnimationUpdateDuration:
          if (lynxUI instanceof LynxUI) {
            double layoutAnimationUpdateDuration = entry.getDouble();
            ((LynxUI) lynxUI).setLayoutAnimationUpdateDuration(layoutAnimationUpdateDuration);
            uiPaintStyles.mLayoutAnimationUpdateDuration = layoutAnimationUpdateDuration;
          }
          break;
          // layoutAnimationUpdateProperty;
        case LayoutAnimationUpdateTimingFunction:
          if (lynxUI instanceof LynxUI) {
            ReadableArray layoutAnimationUpdateTimingFunction =
                new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI)
                .setLayoutAnimationUpdateTimingFunc(layoutAnimationUpdateTimingFunction);
            uiPaintStyles.mLayoutAnimationUpdateTimingFunction =
                layoutAnimationUpdateTimingFunction;
          }
          break;
        case Opacity:
          if (lynxUI instanceof LynxUI) {
            float opacity = (float) entry.getInt();
            ((LynxUI) lynxUI).setAlpha(opacity);
            uiPaintStyles.mOpacity = opacity;
          }
          break;
        case Visibility:
          if (lynxUI instanceof LynxUI) {
            int visibility = entry.getInt();
            ((LynxUI) lynxUI).setVisibility(visibility);
            uiPaintStyles.mVisibility = visibility;
          }
          break;
        case Transform:
          if (lynxUI instanceof LynxUI) {
            ReadableArray transform = new ReadableMapBufferWrapper(entry.getMapBuffer());
            ((LynxUI) lynxUI).setTransform(transform);
            uiPaintStyles.mTransform = transform;
          }
          break;
        case ZIndex:
          if (lynxUI instanceof UIComponent) {
            int zIndex = entry.getInt();
            ((UIComponent) lynxUI).setZIndex(zIndex);
          }
          break;
        case ListMainAxisGap:
          if (lynxUI instanceof AbsLynxList) {
            ((AbsLynxList<?>) lynxUI).setMainAxisGap((float) entry.getDouble());
            break;
          }
          break;
        case ListCrossAxisGap:
          if (lynxUI instanceof AbsLynxList) {
            ((AbsLynxList<?>) lynxUI).setCrossAxisGap((float) entry.getDouble());
            break;
          }
          break;
        default:
          break;
      }
    }
  }

  public static void updateUIPaintStyle(LynxBaseUI lynxUI, UIPaintStyles uiPaintStyles) {
    if (lynxUI == null || uiPaintStyles == null) {
      return;
    }

    // case BackgroundClip:
    lynxUI.setBackgroundClip(uiPaintStyles.mBackgroundClip);

    // case BackgroundColor:
    lynxUI.setBackgroundColor(uiPaintStyles.mBackgroundColor);

    // case BackgroundImage:
    lynxUI.setBackgroundImage(uiPaintStyles.mBackgroundImage);

    // case BackgroundOrigin:
    lynxUI.setBackgroundOrigin(uiPaintStyles.mBackgroundOrigin);

    // case BackgroundPosition:
    lynxUI.setBackgroundPosition(uiPaintStyles.mBackgroundPosition);

    // case BackgroundRepeat:
    lynxUI.setBackgroundRepeat(uiPaintStyles.mBackgroundRepeat);

    // case BackgroundSize:
    lynxUI.setBackgroundSize(uiPaintStyles.mBackgroundSize);

    // case MaskImage:
    lynxUI.setMaskImage(uiPaintStyles.mMaskImage);

    // case MaskOrigin:
    lynxUI.setMaskOrigin(uiPaintStyles.mMaskOrigin);

    // case MaskRepeat:
    lynxUI.setMaskRepeat(uiPaintStyles.mMaskRepeat);

    // case MaskSize:
    lynxUI.setMaskSize(uiPaintStyles.mMaskSize);

    // case MaskPosition:
    lynxUI.setMaskPosition(uiPaintStyles.mMaskPosition);

    // case MaskClip:
    lynxUI.setMaskClip(uiPaintStyles.mMaskClip);

    // case BorderRadius:
    lynxUI.setBorderRadius(0, uiPaintStyles.mBorderRadius);

    // case BorderTopLeftRadius:
    lynxUI.setBorderRadius(1, uiPaintStyles.mBorderTopLeftRadius);

    // case BorderTopRightRadius:
    lynxUI.setBorderRadius(2, uiPaintStyles.mBorderTopRightRadius);

    // case BorderBottomRightRadius:
    lynxUI.setBorderRadius(3, uiPaintStyles.mBorderBottomRightRadius);

    // case BorderBottomLeftRadius:
    lynxUI.setBorderRadius(4, uiPaintStyles.mBorderBottomLeftRadius);

    // case Overflow:
    lynxUI.setOverflow(uiPaintStyles.mOverflow);

    // case OverflowX:
    lynxUI.setOverflowX(uiPaintStyles.mOverflowX);

    // case OverflowY:
    lynxUI.setOverflowY(uiPaintStyles.mOverflowY);

    // case BorderStyle:
    lynxUI.setBorderStyle(0, uiPaintStyles.mBorderStyle);

    // case BorderLeftStyle:
    lynxUI.setBorderStyle(1, uiPaintStyles.mBorderLeftStyle);

    // case BorderRightStyle:
    lynxUI.setBorderStyle(2, uiPaintStyles.mBorderRightStyle);

    // case BorderTopStyle:
    lynxUI.setBorderStyle(3, uiPaintStyles.mBorderTopStyle);

    // case BorderBottomStyle:
    lynxUI.setBorderStyle(4, uiPaintStyles.mBorderBottomStyle);

    // case BorderWidth:
    lynxUI.setBorderWidth(0, uiPaintStyles.mBorderWidth);

    // case BorderLeftWidth:
    lynxUI.setBorderWidth(1, uiPaintStyles.mBorderLeftWidth);

    // case BorderRightWidth:
    lynxUI.setBorderWidth(2, uiPaintStyles.mBorderRightWidth);

    // case BorderTopWidth:
    lynxUI.setBorderWidth(3, uiPaintStyles.mBorderTopWidth);

    // case BorderBottomWidth:
    lynxUI.setBorderWidth(4, uiPaintStyles.mBorderBottomWidth);

    // TODO(nihao.royal) case BorderColor
    // case BorderLeftColor:
    lynxUI.setBorderColor(0, uiPaintStyles.mBorderLeftColor);

    // case BorderRightColor:
    lynxUI.setBorderColor(1, uiPaintStyles.mBorderRightColor);

    // case BorderTopColor:
    lynxUI.setBorderColor(2, uiPaintStyles.mBorderTopColor);

    // case BorderBottomColor:
    lynxUI.setBorderColor(3, uiPaintStyles.mBorderBottomColor);

    // case OutlineColor:
    lynxUI.setOutlineColor(uiPaintStyles.mOutlineColor);

    // case OutlineWidth:
    lynxUI.setOutlineWidth(uiPaintStyles.mOutlineWidth);

    // case OutlineStyle:
    lynxUI.setOutlineStyle(uiPaintStyles.mOutlineStyle);

    // case FontSize:
    lynxUI.setFontSize(uiPaintStyles.mFontSize);

    // case Direction:
    lynxUI.setLynxDirection(uiPaintStyles.mDirection);

    // case TransformOrigin:
    lynxUI.setTransformOrigin(uiPaintStyles.mTransformOrigin);

    // case Perspective:
    lynxUI.setPerspective(uiPaintStyles.mPerspective);

    // case BoxShadow:
    lynxUI.setBoxShadow(uiPaintStyles.mBoxShadow);

    // case ImageRendering:
    lynxUI.setImageRendering(uiPaintStyles.mImageRendering);

    // from lynxUI
    if (lynxUI instanceof LynxUI) {
      // case ClipPath:
      ((LynxUI) lynxUI).setClipPath(uiPaintStyles.mClipPath);

      // case Filter:
      ((LynxUI) lynxUI).setFilter(uiPaintStyles.mFilter);

      // case LayoutAnimationCreateDelay:
      ((LynxUI) lynxUI).setLayoutAnimationCreateDelay(uiPaintStyles.mLayoutAnimationCreateDelay);

      // case LayoutAnimationCreateDuration:
      ((LynxUI) lynxUI)
          .setLayoutAnimationCreateDuration(uiPaintStyles.mLayoutAnimationCreateDuration);

      // case LayoutAnimationCreateProperty:
      ((LynxUI) lynxUI)
          .setLayoutAnimationCreateProperty(uiPaintStyles.mLayoutAnimationCreateProperty);

      // case LayoutAnimationCreateTimingFunction:
      ((LynxUI) lynxUI)
          .setLayoutAnimationCreateTimingFunc(uiPaintStyles.mLayoutAnimationCreateTimingFunction);

      // case LayoutAnimationDeleteTimingFunctionDelay:
      ((LynxUI) lynxUI).setLayoutAnimationDeleteDelay(uiPaintStyles.mLayoutAnimationDeleteDelay);

      // case LayoutAnimationDeleteDuration:
      ((LynxUI) lynxUI)
          .setLayoutAnimationDeleteDuration(uiPaintStyles.mLayoutAnimationDeleteDuration);

      // case LayoutAnimationDeleteProperty:
      ((LynxUI) lynxUI)
          .setLayoutAnimationDeleteProperty(uiPaintStyles.mLayoutAnimationDeleteProperty);

      // case LayoutAnimationDeleteTimingFunction:
      ((LynxUI) lynxUI)
          .setLayoutAnimationDeleteTimingFunc(uiPaintStyles.mLayoutAnimationDeleteTimingFunction);

      // case LayoutAnimationUpdateDelay:
      ((LynxUI) lynxUI).setLayoutAnimationUpdateDelay(uiPaintStyles.mLayoutAnimationUpdateDelay);

      // case LayoutAnimationUpdateDuration:
      ((LynxUI) lynxUI)
          .setLayoutAnimationUpdateDuration(uiPaintStyles.mLayoutAnimationUpdateDuration);

      // layoutAnimationUpdateProperty;
      // case LayoutAnimationUpdateTimingFunction:
      ((LynxUI) lynxUI)
          .setLayoutAnimationUpdateTimingFunc(uiPaintStyles.mLayoutAnimationUpdateTimingFunction);

      // case Opacity:
      ((LynxUI) lynxUI).setAlpha(uiPaintStyles.mOpacity);

      // case Visibility:
      ((LynxUI) lynxUI).setVisibility(uiPaintStyles.mVisibility);

      // case Transform:
      ((LynxUI) lynxUI).setTransform(uiPaintStyles.mTransform);
    }
  }

  public static <T extends ShadowNode> void updateStyles(T node, MapBuffer initialStyles) {
    if (initialStyles == null || !(node instanceof BaseTextShadowNode)) {
      return;
    }
    BaseTextShadowNode shadowNode = (BaseTextShadowNode) node;
    Iterator<MapBuffer.Entry> iter = initialStyles.iterator();
    while (iter.hasNext()) {
      MapBuffer.Entry entry = iter.next();
      int key = entry.getKey();
      switch (key) {
        case Color:
          shadowNode.setColor(
              new DynamicFromMapBuffer(new ReadableMapBufferWrapper(initialStyles), Color));
          break;
        case Direction:
          shadowNode.setDirection(entry.getInt());
          break;
        case FontFamily:
          shadowNode.setFontFamily(entry.getString());
          break;
        case FontSize:
          shadowNode.setFontSize((float) entry.getDouble());
          break;
        case FontStyle:
          shadowNode.setFontStyle(entry.getInt());
          break;
        case FontWeight:
          shadowNode.setFontWeight(entry.getInt());
          break;
        case LetterSpacing:
          shadowNode.setLetterSpacing((float) entry.getDouble());
          break;
        case LineHeight:
          shadowNode.setLineHeight((float) entry.getDouble());
          break;
        case LineSpacing:
          shadowNode.setLineSpacing((float) entry.getDouble());
          break;
        case TextAlign:
          shadowNode.setTextAlign(entry.getInt());
          break;
        case TextDecoration:
          shadowNode.setTextDecoration(new ReadableMapBufferWrapper(entry.getMapBuffer()));
          break;
        case TextIndent:
          shadowNode.setTextIndent(new ReadableMapBufferWrapper(entry.getMapBuffer()));
          break;
        case TextOverflow:
          shadowNode.setTextOverflow(entry.getInt());
          break;
        case TextShadow:
          shadowNode.setTextShadow(new ReadableMapBufferWrapper(entry.getMapBuffer()));
          break;
        case TextStrokeColor:
          shadowNode.setTextStrokeColor(new DynamicFromMapBuffer(
              new ReadableMapBufferWrapper(initialStyles), TextStrokeColor));
          break;
        case TextStrokeWidth:
          shadowNode.setTextStrokeWidth((float) entry.getDouble());
          break;
        case WhiteSpace:
          shadowNode.setWhiteSpace(entry.getInt());
          break;
        case WordBreak:
          shadowNode.setWordBreakStrategy(entry.getInt());
          break;
        default:
          break;
      }
    }
  }
}
