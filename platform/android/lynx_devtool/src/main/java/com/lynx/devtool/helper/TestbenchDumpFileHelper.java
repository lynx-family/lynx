// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import android.text.Layout;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import java.lang.reflect.Field;
import java.util.List;
import org.json.JSONArray;
import org.json.JSONObject;

public class TestbenchDumpFileHelper {
  private static final String TAG = "TestbenchDumpFileHelper";

  public static String getViewTree(View rootView) {
    return getViewTreeRecursive(rootView).toString();
  }

  private static JSONObject getViewTreeRecursive(View view) {
    JSONObject node = new JSONObject();
    try {
      Class clz = view.getClass();
      String name = clz.getSimpleName();
      // CQ check class name
      switch (name) {
        case "LynxImageView":
          try {
            Field field = clz.getDeclaredField("mTagName");
            field.setAccessible(true);
            String tagName = (String) field.get(view);
            if ("filter-image".equals(tagName)) {
              name = "FrescoFilterImageView";
            } else {
              name = "FrescoImageView";
            }
          } catch (Throwable e) {
            String err = Log.getStackTraceString(e);
            LLog.e(TAG, err);
            name = err;
          }
          break;
      }
      node.put("name", name);

      node.put("left", view.getLeft());
      node.put("top", view.getTop());
      node.put("width", view.getWidth());
      node.put("height", view.getHeight());

      if (view instanceof ViewGroup) {
        int count = ((ViewGroup) view).getChildCount();
        if (count > 0) {
          JSONArray childrenNodes = new JSONArray();
          for (int i = 0; i < count; i++) {
            childrenNodes.put(getViewTreeRecursive(((ViewGroup) view).getChildAt(i)));
          }
          node.put("children", childrenNodes);
        }
      }
    } catch (Exception e) {
      LLog.e(TAG, e.toString());
    }

    return node;
  }

  public static String getUITree(LynxBaseUI rootUI) {
    return getUITreeRecursive(rootUI).toString();
  }

  private static JSONObject getUITreeRecursive(LynxBaseUI ui) {
    JSONObject node = new JSONObject();
    try {
      String name = ui.getClass().getSimpleName();
      switch (name) {
        case "LynxImageUI":
          if ("filter-image".equals(ui.getTagName())) {
            name = "UIFilterImage";
          } else {
            name = "UIImage";
          }
          break;
        case "LynxFlattenImageUI":
          name = "FlattenUIImage";
          break;
      }
      node.put("name", name);
      node.put("left", ui.getLeft());
      node.put("top", ui.getTop());
      node.put("width", ui.getWidth());
      node.put("height", ui.getHeight());

      getSpecificInfo(ui, node);

      List<LynxBaseUI> children = ui.getChildren();
      if (children.size() > 0) {
        JSONArray childrenNodes = new JSONArray();
        for (LynxBaseUI child : children) {
          childrenNodes.put(getUITreeRecursive(child));
        }
        node.put("children", childrenNodes);
      }
    } catch (Exception e) {
      LLog.e(TAG, e.toString());
    }

    return node;
  }

  private static void getSpecificInfo(LynxBaseUI ui, JSONObject node) throws Exception {
    Class clz = ui.getClass();
    String name = clz.getSimpleName();
    if (name.equals("LynxImageUI") || name.equals("LynxFlattenImageUI")) {
      setLynxImageUIInfo(ui, node);
    } else if (name.equals("FlattenUIImage")) {
      setFlattenUIImageInfo(clz, ui, node);
    } else if (name.equals("UIText")) {
      setTextInfo(((LynxUI) ui).getView(), node);
    } else if (name.equals("FlattenUIText")) {
      setTextInfo(ui, node);
    }
  }

  private static void setFlattenUIImageInfo(Class imageClz, LynxBaseUI ui, JSONObject node)
      throws Exception {
    Field lynxImageManagerField = imageClz.getDeclaredField("mLynxImageManager");
    lynxImageManagerField.setAccessible(true);
    Object lynxImageManager = lynxImageManagerField.get(ui);
    Class lynxImageManagerClz = lynxImageManager.getClass();
    Field imageSourceField = lynxImageManagerClz.getDeclaredField("mImageSource");
    imageSourceField.setAccessible(true);

    Object imageSource = imageSourceField.get(lynxImageManager);
    Class imageSourceClz = imageSource.getClass();
    Field sourceField = imageSourceClz.getDeclaredField("mSource");
    sourceField.setAccessible(true);

    node.put("src", sourceField.get(imageSource));
  }

  private static void setLynxImageUIInfo(Object imageUI, JSONObject node) throws Exception {
    Class imageClz = imageUI.getClass();

    Field imageDelegateField = imageClz.getDeclaredField("mImageDelegate");
    imageDelegateField.setAccessible(true);
    Object imageDelegate = imageDelegateField.get(imageUI);

    Class imageDelegateClz = imageDelegate.getClass();
    Field src = imageDelegateClz.getDeclaredField("mSrc");
    src.setAccessible(true);

    node.put("src", src.get(imageDelegate));
  }

  private static void setTextInfo(Object androidView, JSONObject node) throws Exception {
    Class androidTextClz = androidView.getClass();
    Field layoutField = androidTextClz.getDeclaredField("mTextLayout");
    layoutField.setAccessible(true);

    Layout layout = (Layout) layoutField.get(androidView);
    node.put("alignment", layout.getAlignment().toString());
    node.put("textSize", layout.getPaint().getTextSize());
  }
}
