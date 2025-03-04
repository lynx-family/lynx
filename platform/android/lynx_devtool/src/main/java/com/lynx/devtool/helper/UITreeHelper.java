// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static com.lynx.tasm.behavior.StyleConstants.VISIBILITY_HIDDEN;
import static com.lynx.tasm.behavior.StyleConstants.VISIBILITY_VISIBLE;

import android.graphics.Rect;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.text.FlattenUIText;
import com.lynx.tasm.behavior.ui.text.UIText;
import com.lynx.tasm.event.EventsListener;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

@Keep
public class UITreeHelper {
  private static final String TAG = UITreeHelper.class.getSimpleName();
  private WeakReference<LynxUIOwner> mUIOwner;

  public void attachLynxUIOwner(LynxUIOwner uiOwner) {
    mUIOwner = new WeakReference<>(uiOwner);
  }

  public float[] getRectToWindow() {
    float[] res = new float[] {0, 0, 0, 0};
    LynxUIOwner uiOwner = (mUIOwner != null) ? mUIOwner.get() : null;
    if (uiOwner == null || uiOwner.getRootSign() == -1) {
      return res;
    }
    LynxBaseUI ui = uiOwner.getNode(uiOwner.getRootSign());
    if (ui != null) {
      Rect re = ui.getRectToWindow();
      res[0] = re.left;
      res[1] = re.top;
      res[2] = re.width();
      res[3] = re.height();
    }
    return res;
  }

  public void getViewLocationOnScreen(int[] res) {
    LynxUIOwner uiOwner = (mUIOwner != null) ? mUIOwner.get() : null;
    if (res == null || res.length < 2 || uiOwner == null || uiOwner.getContext() == null) {
      return;
    }
    LynxView view = uiOwner.getContext().getLynxView();
    if (view != null) {
      view.getLocationOnScreen(res);
      LynxContext context = view.getLynxContext();
      if (context != null) {
        float scale = context.getResources().getDisplayMetrics().density;
        res[0] /= scale;
        res[1] /= scale;
      }
    }
  }

  /*
   * get LynxBaseUI Tree rooted at RootUI
   *
   * return value:
   * the string converted from jsonObject of LynxBaseUI Tree
   */
  public String getLynxUITree() {
    LynxUIOwner uiOwner = (mUIOwner != null) ? mUIOwner.get() : null;
    if (uiOwner == null) {
      return "";
    }
    LynxBaseUI root = uiOwner.getRootUI();
    if (root == null) {
      return "";
    }

    Map<String, Object> map = getLynxUITreeRecursive(root);
    try {
      JSONObject jsonObject = getJsonFromMap(map);
      return jsonObject.toString();
    } catch (JSONException e) {
      LLog.e(TAG, "UITree: getLynxUITree fail");
      return "";
    }
  }

  /*
   * get LynxBaseUITree Recursively
   *
   * parameter:
   *  ui:LynxBaseUI, the ui to traverse from
   *
   * return value:
   * the Map<String, Object> of ui, store ui's name/id/frame/children, and it's
   * children is also Map<String, Object> like itself
   */
  private Map<String, Object> getLynxUITreeRecursive(@NonNull LynxBaseUI ui) {
    HashMap<String, Object> map = new HashMap<>();
    map.put("name", ui.getClass().getName());
    map.put("id", ui.getSign());
    map.put("frame",
        new JSONArray(
            Arrays.asList(ui.getOriginLeft(), ui.getOriginTop(), ui.getWidth(), ui.getHeight())));
    List<Object> list = new ArrayList<>();
    for (LynxBaseUI child : ui.getChildren()) {
      list.add(getLynxUITreeRecursive(child));
    }
    map.put("children", new JSONArray(list));
    return map;
  }

  /*
   * transfer Map<String, Object> to jsonObject.
   * if key's value is Map, then call getJsonFromMap recursively
   */
  private static JSONObject getJsonFromMap(Map<String, Object> map) throws JSONException {
    JSONObject jsonData = new JSONObject();
    for (String key : map.keySet()) {
      Object value = map.get(key);
      if (value instanceof Map<?, ?>) {
        value = getJsonFromMap((Map<String, Object>) value);
      }
      jsonData.put(key, value);
    }
    return jsonData;
  }

  /*
   * get ui's editable property
   * actually there are 6 editable properties. In addition to frame/border/margin/visible,
   * background-color/border-color are also editable, which don't need show value on the frontend
   * just show hint about input format.
   */
  private static HashMap<String, Object> getEditablePropertyValueOfUI(@NonNull LynxBaseUI ui) {
    HashMap<String, Object> map = new HashMap<>();
    map.put("border",
        new JSONArray(Arrays.asList(ui.getBorderTopWidth(), ui.getBorderRightWidth(),
            ui.getBorderBottomWidth(), ui.getBorderLeftWidth())));
    map.put("margin",
        new JSONArray(Arrays.asList(
            ui.getMarginTop(), ui.getMarginRight(), ui.getMarginBottom(), ui.getMarginLeft())));
    map.put("frame",
        new JSONArray(
            Arrays.asList(ui.getOriginLeft(), ui.getOriginTop(), ui.getWidth(), ui.getHeight())));
    map.put("visible", ui.getVisibility());
    return map;
  }

  // get ui's main readonly property
  private static HashMap<String, Object> getReadPropertyValueOfUI(@NonNull LynxBaseUI ui) {
    HashMap<String, Object> map = new HashMap<>();
    // get ui's location relative to screen
    LynxBaseUI.TransOffset transOffset = ui.getTransformValue(0, 0, 0, 0);
    map.put("locationToScreen",
        new JSONArray(Arrays.asList(transOffset.left_top[0], transOffset.left_top[1],
            transOffset.right_bottom[0], transOffset.right_bottom[1])));
    map.put("clickable", ui.isClickable());
    map.put("long-clickable", ui.isLongClickable());
    map.put("scrollable", ui.isScrollable());
    map.put("focusable", ui.isFocusable());

    Map<String, EventsListener> events = ui.getEvents();
    if (events != null) {
      HashMap<String, String> eventMap = new HashMap<>();
      for (EventsListener event : events.values()) {
        String name = event.name;
        if (event.type != null) {
          if (event.type.equals("bindEvent")) {
            name = "bind" + name;
          } else if (event.type.equals("catchEvent")) {
            name = "catch" + name;
          } else if (event.type.equals("capture-bindEvent")) {
            name = "capture-bind" + name;
          } else if (event.type.equals("capture-catchEvent")) {
            name = "capture-catch" + name;
          } else {
            name = event.type + name;
          }
        }
        eventMap.put(name, event.functionName);
      }
      map.put("events", eventMap);
    }

    String text = "";
    if (ui instanceof FlattenUIText) {
      if (((FlattenUIText) ui).getText() != null) {
        text = ((FlattenUIText) ui).getText().toString();
      }
      map.put("text", text);
    } else if (ui instanceof UIText) {
      if (((UIText) ui).getView().getText() != null) {
        text = ((UIText) ui).getView().getText().toString();
      }
      map.put("text", text);
    }

    View view = null;
    if (ui.isFlatten() && ui.getDrawParent() != null) {
      view = ((LynxUI) ui.getDrawParent()).getView();
    } else {
      view = ((LynxUI) ui).getView();
    }

    map.put("lynx-test-tag", ui.getTestID());

    if (view == null) {
      LLog.e(TAG, "getReadPropertyValueOfUI: view is null");
      return map;
    }

    map.put("activated", view.isActivated());
    map.put("alpha", view.getAlpha());
    map.put("has-window-focus", view.hasWindowFocus());

    if (ui.isScrollable()) {
      String scroll_left = view.canScrollHorizontally(-1) ? "left " : "";
      String scroll_right = view.canScrollHorizontally(1) ? "right " : "";
      String scroll_up = view.canScrollVertically(-1) ? "up " : "";
      String scroll_down = view.canScrollVertically(1) ? "down " : "";
      String scroll = scroll_left + scroll_right + scroll_up + scroll_down;
      if (scroll != "") {
        scroll = scroll.substring(0, scroll.length() - 1);
      }
      map.put("scroll-direction", scroll);
    }
    return map;
  }

  void getAllDeclaredFieldForCurrentClass(Class currentClass, Object obj, Map<String, Object> map) {
    if (currentClass == null) {
      LLog.e(TAG, "getAllDeclaredFieldForCurrentClass: currentClass is null");
      return;
    }
    Field[] fields = currentClass.getDeclaredFields();
    for (Field field : fields) {
      try {
        field.setAccessible(true);
        String name = field.getName();
        Object value = field.get(obj);
        map.put(name, value.toString());
      } catch (Exception e) {
        LLog.e(TAG, "UITree: getAllDeclaredFieldForCurrentClass fail when getUINodeInfo");
      }
    }
  }

  /*
   * get all properties key/value from current class to given super class
   *
   * parameters:
   * obj : the object to get all properties
   * type: "view"/"ui",for View, traverse from current class to View class
   * for "ui" , traverse from current class to LynxBaseUI
   *
   * return value:
   * the HashMap<String, Object> which store properties key-value
   */
  private HashMap<String, Object> getAllPropertyAndValues(Object obj, String type) {
    HashMap<String, Object> map = new HashMap<>();

    if (obj == null || type == null) {
      LLog.e(TAG, "getAllPropertyAndValues: obj or type is null");
      return map;
    }

    // get all properties from [currentClass, stopClass)
    Class currentClass = obj.getClass();
    Class stopClass;
    if (type.equals("view")) {
      stopClass = View.class;
    } else { //"ui"
      stopClass = LynxBaseUI.class;
    }

    getAllDeclaredFieldForCurrentClass(currentClass, obj, map);
    while (currentClass != stopClass) {
      currentClass = currentClass.getSuperclass();
      getAllDeclaredFieldForCurrentClass(currentClass, obj, map);
    }
    return map;
  }

  /*
   * get the information of the specified LynxBaseUI node
   *
   * parameter:
   * id : id of the LynxBaseUI node you want to get information
   *
   * return value:
   * the string converted from jsonObject of LynxBaseUI information
   */
  public String getUINodeInfo(int id) {
    String res = new String();
    LynxUIOwner uiOwner = (mUIOwner != null) ? mUIOwner.get() : null;
    if (uiOwner == null) {
      return "";
    }

    LynxBaseUI ui = uiOwner.getNode(id);
    if (ui == null) {
      return "";
    }
    try {
      HashMap<String, Object> map = new HashMap<>();
      map.put("id", ui.getSign());
      map.put("isFlatten", ui.isFlatten());
      map.put("editableProps", getEditablePropertyValueOfUI(ui));
      map.put("readonlyProps", getReadPropertyValueOfUI(ui));

      HashMap<String, Object> ui_map = new HashMap<>();
      ui_map.put("name", ui.getClass().getName());
      ui_map.put("readonlyProps", getAllPropertyAndValues(ui, "ui"));
      map.put("ui", ui_map);

      if (!ui.isFlatten()) {
        LynxUI lynxUI = (LynxUI) ui;
        HashMap<String, Object> view_map = new HashMap<>();
        view_map.put("name", lynxUI.getView().getClass().getName());
        view_map.put("readonlyProps", getAllPropertyAndValues(lynxUI.getView(), "view"));
        map.put("view", view_map);
      }

      JSONObject jsonObject = getJsonFromMap(map);
      return jsonObject.toString();
    } catch (JSONException e) {
      LLog.e(TAG, "UITree: getUINodeInfo fail!");
      return "";
    }
  }

  /*stringToArrayValue: decode value string of frame/border/margin style to float array
   *
   *parameter:
   *4 num string, such as "3,2, 3, 5", split num by "," and there may be whitespace
   *
   *
   *return value: array
   *[num1, num2, num3, num4]
   */
  float[] stringToArrayValue(String strValue) {
    try {
      float[] res = new float[4];
      strValue = strValue.replace(" ", "");
      String[] arr = strValue.split(",");
      res[0] = Float.parseFloat(arr[0]);
      res[1] = Float.parseFloat(arr[1]);
      res[2] = Float.parseFloat(arr[2]);
      res[3] = Float.parseFloat(arr[3]);
      return res;
    } catch (Exception e) {
      LLog.e(TAG, "UITree Debug:parse frame value string fail");
    }
    return new float[0];
  }

  private int setUIFrame(LynxBaseUI baseUI, String styleContent) {
    if (baseUI == null) {
      return -1;
    }
    float[] frame_value = stringToArrayValue(styleContent);
    if (frame_value.length != 0) {
      baseUI.setLeft((int) frame_value[0]);
      baseUI.setTop((int) frame_value[1]);
      baseUI.setWidth((int) frame_value[2]);
      baseUI.setHeight((int) frame_value[3]);
      return 0;
    } else {
      return -1;
    }
  }

  private int setUIBorder(LynxBaseUI baseUI, String styleContent) {
    if (baseUI == null) {
      return -1;
    }
    float[] border_value = stringToArrayValue(styleContent);
    if (border_value.length != 0) {
      // leftWidth
      baseUI.setBorderWidth(1, (int) border_value[3]);
      // rightWidth
      baseUI.setBorderWidth(2, (int) border_value[1]);
      // topWidth
      baseUI.setBorderWidth(3, (int) border_value[0]);
      // bottomWidth
      baseUI.setBorderWidth(4, (int) border_value[2]);

      baseUI.updateLayout(baseUI.getLeft(), baseUI.getTop(), baseUI.getWidth(), baseUI.getHeight(),
          baseUI.getPaddingLeft(), baseUI.getPaddingTop(), baseUI.getPaddingRight(),
          baseUI.getPaddingBottom(), baseUI.getMarginLeft(), baseUI.getMarginTop(),
          baseUI.getMarginRight(), baseUI.getMarginBottom(), (int) border_value[3],
          (int) border_value[0], (int) border_value[1], (int) border_value[2], baseUI.getBound());
      return 0;
    } else {
      return -1;
    }
  }

  private int setUIMargin(LynxBaseUI baseUI, String styleContent) {
    if (baseUI == null) {
      return -1;
    }
    float[] margin_value = stringToArrayValue(styleContent);
    if (margin_value.length != 0) {
      int topWidth = (int) margin_value[0];
      int rightWidth = (int) margin_value[1];
      int bottomWidth = (int) margin_value[2];
      int leftWidth = (int) margin_value[3];

      baseUI.updateLayout(baseUI.getLeft() - baseUI.getMarginLeft() + leftWidth,
          baseUI.getTop() - baseUI.getMarginTop() + topWidth, baseUI.getWidth(), baseUI.getHeight(),
          baseUI.getPaddingLeft(), baseUI.getPaddingTop(), baseUI.getPaddingRight(),
          baseUI.getPaddingBottom(), leftWidth, topWidth, rightWidth, bottomWidth,
          baseUI.getBorderLeftWidth(), baseUI.getBorderTopWidth(), baseUI.getBorderRightWidth(),
          baseUI.getBorderBottomWidth(), baseUI.getBound());
      return 0;
    } else {
      return -1;
    }
  }

  private int setUIBackgroundColor(LynxBaseUI baseUI, String styleContent) {
    try {
      // #RRGGBBAA -> RRGGBBAA
      styleContent = styleContent.substring(1);
      // RRGGBBAA -> AARRGGBB
      styleContent = styleContent.substring(6) + styleContent.substring(0, 6);
      int color = Integer.parseUnsignedInt(styleContent, 16);
      baseUI.setBackgroundColor(color);
      return 0;
    } catch (Exception e) {
      LLog.e(TAG, "UITree Debug:parse color value string fail");
      return -1;
    }
  }

  private int setUIBorderColor(LynxBaseUI baseUI, String styleContent) {
    try {
      // #RRGGBBAA -> RRGGBBAA
      styleContent = styleContent.substring(1);
      // RRGGBBAA -> AARRGGBB
      styleContent = styleContent.substring(6) + styleContent.substring(0, 6);
      int color = Integer.parseUnsignedInt(styleContent, 16);
      baseUI.setBorderColor(0, color);
      baseUI.setBorderColor(1, color);
      baseUI.setBorderColor(2, color);
      baseUI.setBorderColor(3, color);
      return 0;
    } catch (Exception e) {
      LLog.e(TAG, "UITree Debug:parse color value string fail");
      return -1;
    }
  }

  private int setUIVisibility(LynxBaseUI baseUI, String styleContent) {
    // Flatten UI don't support set visibility
    if (baseUI == null || baseUI.isFlatten() || styleContent == null)
      return -1;
    LynxUI ui = (LynxUI) baseUI;
    if (styleContent.equals("true")) {
      ui.setVisibility(VISIBILITY_VISIBLE);
      return 0;
    } else if (styleContent.equals("false")) {
      ui.setVisibility(VISIBILITY_HIDDEN);
      return 0;
    } else {
      return -1;
    }
  }

  /*
   *set UI given style
   *
   *Parameter:
   * id: id of ui
   * name: style name which you wan't to set. only support
   *frame/border/margin/border-color/background-color/visible content: the style value you wan't
   *to set
   *
   *Return Value:
   * if name is supported editable style and content's format is right, then set style success and
   *return 0 otherwise, set style fail, and return -1
   */
  public int setUIStyle(int id, String name, String content) {
    LynxUIOwner uiOwner = (mUIOwner != null) ? mUIOwner.get() : null;
    if (uiOwner != null && name != null && content != null) {
      LynxBaseUI baseUI = uiOwner.getNode(id);
      if (baseUI != null) {
        content = content.trim();
        if (name.equals("frame")) {
          return setUIFrame(baseUI, content);
        } else if (name.equals("margin")) {
          return setUIMargin(baseUI, content);
        } else if (name.equals("border")) {
          return setUIBorder(baseUI, content);
        } else if (name.equals("visible")) {
          return setUIVisibility(baseUI, content);
        } else if (name.equals("background-color")) {
          return setUIBackgroundColor(baseUI, content);
        } else if (name.equals("border-color")) {
          return setUIBorderColor(baseUI, content);
        }
      }
    }
    return 0;
  }

  @Keep
  public static String getLynxFastbotTweakTree(LynxView lynxView) {
    if (lynxView == null || lynxView.getLynxContext() == null
        || lynxView.getLynxContext().getLynxUIOwner() == null) {
      return "";
    }

    LynxUIOwner uiOwner = lynxView.getLynxContext().getLynxUIOwner();

    LynxBaseUI root = uiOwner.getRootUI();
    if (root == null) {
      return "";
    }

    Map<String, Object> map = getLynxFastbotTweakTreeRecursive(root);
    try {
      JSONObject jsonObject = getJsonFromMap(map);
      return jsonObject.toString();
    } catch (JSONException e) {
      LLog.e(TAG, "UITree: getLynxUITree fail");
      return "";
    }
  }

  private static Map<String, Object> getLynxFastbotTweakTreeRecursive(@NonNull LynxBaseUI ui) {
    HashMap<String, Object> map = new HashMap<>();
    map.put("id", ui.getSign());
    map.put("class", ui.getClass().getName());

    HashMap<String, Object> editableProps = getEditablePropertyValueOfUI(ui);
    HashMap<String, Object> readProps = getReadPropertyValueOfUI(ui);

    map.putAll(editableProps);
    map.putAll(readProps);

    List<Object> list = new ArrayList<>();
    for (LynxBaseUI child : ui.getChildren()) {
      list.add(getLynxFastbotTweakTreeRecursive(child));
    }
    map.put("children", new JSONArray(list));
    return map;
  }
}
