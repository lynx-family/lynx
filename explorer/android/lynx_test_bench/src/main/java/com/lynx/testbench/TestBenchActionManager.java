// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.google.gson.Gson;
import com.lynx.BuildConfig;
import com.lynx.devtoolwrapper.LynxBaseInspectorOwner;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxGroup.LynxGroupBuilder;
import com.lynx.tasm.LynxUpdateMeta;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.TemplateBundle;
import com.lynx.tasm.TemplateBundleOption;
import com.lynx.tasm.TemplateData;
import com.lynx.tasm.ThreadStrategyForRendering;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BuiltInBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.text.RawTextShadowNode;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.provider.AbsTemplateProvider;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.utils.UIThreadUtils;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TimeZone;
import java.util.zip.Inflater;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

public class TestBenchActionManager {
  private static final String TAG = "TestBenchActionManager";
  private static final int UPDATE_PRE_DATA = 0;
  private static final int UPDATE_VIEW_PORT = 1;
  private static final int SET_THREAD_STRATEGY = 2;
  private static final int SET_GLOBAL_PROPS = 3;
  private static final int LOAD_TEMPLATE = 4;
  private static final int LOAD_TEMPLATE_BUNDLE = 9;
  private static final int SEND_GLOBAL_EVENT = 5;
  private static final int SEND_EVENT_ANDROID = 6;
  private static final int RELOAD_TEMPLATE = 7;
  private static final int UPDATE_FONT_SCALE = 11;
  private static final int END_TESTBENCH = 10;
  private static final int UPDATE_META_DATA = 13;
  private static final int ON_TESTBENCH_COMPLETE = 14;
  private static final int IS_VIRTUAL_NODE = 1 << 1;
  private static final int IS_FLATTEN_NODE = 1 << 3;
  private final static HashMap<String, Integer> mCanMockFuncMap = new HashMap<>();
  static {
    mCanMockFuncMap.put("setGlobalProps", SET_GLOBAL_PROPS);
    mCanMockFuncMap.put("updateViewPort", UPDATE_VIEW_PORT);
    mCanMockFuncMap.put("loadTemplate", LOAD_TEMPLATE);
    mCanMockFuncMap.put("loadTemplateBundle", LOAD_TEMPLATE_BUNDLE);
    mCanMockFuncMap.put("setThreadStrategy", SET_THREAD_STRATEGY);
    mCanMockFuncMap.put("updateDataByPreParsedData", UPDATE_PRE_DATA);
    mCanMockFuncMap.put("sendGlobalEvent", SEND_GLOBAL_EVENT);
    mCanMockFuncMap.put("sendEventAndroid", SEND_EVENT_ANDROID);
    mCanMockFuncMap.put("reloadTemplate", RELOAD_TEMPLATE);
    mCanMockFuncMap.put("updateFontScale", UPDATE_FONT_SCALE);
    mCanMockFuncMap.put("updateMetaData", UPDATE_META_DATA);
  }

  // these functions should be call again, when push reload button
  private final static HashSet<String> mReloadFuncSet = new HashSet<>();
  static {
    mReloadFuncSet.add("sendGlobalEvent");
    mReloadFuncSet.add("updateDataByPreParsedData");
    mReloadFuncSet.add("sendEventAndroid");
  }

  private class TestBenchLynxViewClient extends LynxViewClient {
    @Override
    public void onFirstScreen() {
      super.onFirstScreen();
      reloadAction();
      onReplayFinish(sEndForFirstScreen);
    }
  }

  // store information of some function, which will be called when page reload
  private class ReloadAction {
    private JSONObject mParams;
    private int mFunctionID;
    private long mDelayTime;

    public ReloadAction(JSONObject params, int functionID, long delayTime) {
      mParams = params;
      mFunctionID = functionID;
      mDelayTime = delayTime;
    }

    public long getDelayTime() {
      return mDelayTime;
    }

    public void run() {
      Message msg = new Message();
      msg.obj = mParams;
      msg.what = mFunctionID;
      mHandler.sendMessageDelayed(msg, mDelayTime);
    }
  }

  private long mStartTime;
  private ArrayList<ReloadAction> mReloadList;
  private final ViewGroup mViewGroup;
  private final Intent mIntent;
  private final ArrayList<TestBenchActionCallback> mCallbacks;
  private final Handler mHandler;
  private final Context mContext;
  private String mUrl;
  private byte[] templateSource;
  private byte[] mPreloadedTemplateSource;
  private JSONObject mThreadStrategyData;

  private int mThreadMode;
  private JSONObject mGlobalPropsCache;
  private JSONArray mComponentList;
  private JSONObject mConfig;
  private String mLoadTemplateURL;
  private TemplateData mLoadTemplateData;
  private TemplateData mGlobalPropCacheCloned;
  private TemplateData mGlobalProps;
  private TestBenchFetcher mDynamicFetcher;
  private TestBenchLynxViewClient mViewClient;
  private boolean mReplayGesture;
  private boolean mPreDecode;
  private int mDelayEndInterval;
  private TestBenchReplayStateView mStateView;
  private int mScreenWidth;
  private int mScreenHeight;
  private LynxView mLynxView;
  private boolean mSSRLoaded;
  private boolean mDisableUpdateViewport;
  private boolean mLandScape;
  private boolean mEnableAirStrictMode;
  private LynxGroup mLynxGroup;
  private float mRawFontScale;
  private JSONObject mTemplateBundleParams;
  private TemplateBundle mTemplateBundle;
  private TemplateBundleOption mTemplateBundleOptions;

  public static final int sEndForFirstScreen = 0;
  public static final int sEndForAll = 1;

  public static byte[] decompress(byte[] data) {
    byte[] output;

    Inflater decompresser = new Inflater();
    decompresser.reset();
    decompresser.setInput(data);

    ByteArrayOutputStream o = new ByteArrayOutputStream(data.length);
    try {
      byte[] buf = new byte[1024];
      while (!decompresser.finished()) {
        int i = decompresser.inflate(buf);
        o.write(buf, 0, i);
      }
      output = o.toByteArray();
    } catch (Exception e) {
      output = data;
      e.printStackTrace();
    } finally {
      try {
        o.close();
      } catch (IOException e) {
        e.printStackTrace();
      }
    }

    decompresser.end();
    return output;
  }

  private boolean checkFile(JSONArray actionList) {
    for (int index = 0; index < actionList.length(); ++index) {
      try {
        JSONObject action = actionList.getJSONObject(index);
        String functionName = action.getString("Function Name");
        if (functionName.equals("loadTemplate") || functionName.equals("loadTemplateBundle")) {
          mTemplateBundleParams = action.getJSONObject("Params");
          return true;
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
    return false;
  }

  private class InnerCallback implements AbsTemplateProvider.Callback {
    @Override
    public void onSuccess(byte[] body) {
      mainThreadChecker("onSuccess");
      try {
        Gson gson = new Gson();

        // net data -> local data == base64ed Str
        String str = new String(body, StandardCharsets.UTF_8);

        Map map = null;
        mStateView.setReplayState(TestBenchReplayStateView.PARSING_JSON_FILE);
        try {
          if (str.startsWith("{")) {
            map = gson.fromJson(str, Map.class);
          } else {
            // decoder
            byte[] base64decodedBytes = Base64.decode(str, Base64.DEFAULT);

            String str2 = new String(decompress(base64decodedBytes), StandardCharsets.UTF_8);
            map = gson.fromJson(str2, Map.class);
          }
        } catch (Exception e) {
          mStateView.setReplayState(TestBenchReplayStateView.INVALID_JSON_FILE);
          return;
        }

        JSONObject json = new JSONObject(map);
        if (json.has("Config") && !(json.get("Config").toString().equals("null"))) {
          mConfig = json.getJSONObject("Config");
          if (mConfig.has("jsbIgnoredInfo")) {
            TestBenchReplayDataModule.setJsbIgnoredInfo(mConfig.getJSONArray("jsbIgnoredInfo"));
          }

          if (mConfig.has("jsbSettings")) {
            TestBenchReplayDataModule.setJsbSettings(mConfig.getJSONObject("jsbSettings"));
          }
        }
        if (json.has("Invoked Method Data")) {
          mockInvokeMethod(json.getJSONArray("Invoked Method Data"));
        }
        if (json.has("Callback")) {
          mockCallback(json.getJSONObject("Callback"));
        }
        if (json.has("Component List")) {
          mockComponent(json.getJSONArray("Component List"));
        }
        if (json.has("Action List")) {
          if (checkFile(json.getJSONArray("Action List"))) {
            mDynamicFetcher.parse(json.getJSONArray("Action List"));
            mStateView.setReplayState(TestBenchReplayStateView.HANDLE_ACTION_LIST);
            handleActionList(json.getJSONArray("Action List"));
          } else {
            mStateView.setReplayState(TestBenchReplayStateView.RECORD_ERROR_MISS_TEMPLATEJS);
            return;
          }
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }

    @Override
    public void onFailed(String msg) {
      mainThreadChecker("onFailed");
      Log.e("TestBenchActionManager", "Read recorded file failed!");
    }

    private void mainThreadChecker(String methodName) {
      if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
        throw new IllegalThreadStateException(
            "Callback " + methodName + "must be fired on main thread.");
      }
    }
  }

  public TestBenchActionManager(@NonNull Intent intent, @NonNull Context context,
      @NonNull ViewGroup viewGroup, TestBenchReplayStateView stateView,
      @Nullable LynxGroup lynxGroup) {
    mStartTime = 0;
    mContext = context;
    mLynxView = null;
    mIntent = intent;
    mUrl = null;
    mViewGroup = viewGroup;
    mCallbacks = new ArrayList<>();
    mThreadStrategyData = null;
    mGlobalPropsCache = null;
    mConfig = null;
    mReloadList = new ArrayList<>();
    mLoadTemplateData = null;
    mLoadTemplateURL = null;
    mViewClient = null;
    mTemplateBundle = null;
    mTemplateBundleOptions = new TemplateBundleOption.Builder()
                                 .setContextPoolSize(5)
                                 .setEnableContextAutoRefill(true)
                                 .build();
    mTemplateBundleParams = null;
    mReplayGesture = false;
    mPreDecode = false;
    mThreadMode = -1;
    mStateView = stateView;
    mDelayEndInterval = 3500;
    mRawFontScale = -1;
    mDynamicFetcher = new TestBenchFetcher();
    mLynxGroup = lynxGroup;
    Resources resources = mContext.getResources();
    DisplayMetrics dm = resources.getDisplayMetrics();
    mScreenWidth = dm.widthPixels;
    mScreenHeight = dm.heightPixels;
    mHandler = new Handler(Looper.getMainLooper()) {
      @Override
      public void handleMessage(Message msg) {
        switch (msg.what) {
          case SET_THREAD_STRATEGY:
            initialLynxView((JSONObject) msg.obj);
            break;
          case UPDATE_VIEW_PORT:
            initialLynxView((JSONObject) msg.obj);
            break;
          case SET_GLOBAL_PROPS:
            setGlobalProps((JSONObject) msg.obj);
            break;
          case LOAD_TEMPLATE:
            loadTemplate((JSONObject) msg.obj);
            break;
          case LOAD_TEMPLATE_BUNDLE:
            loadTemplateBundle((JSONObject) msg.obj);
            break;
          case END_TESTBENCH:
            endTestbench();
            break;
          case UPDATE_PRE_DATA:
            updateDataPreData((JSONObject) msg.obj);
            break;
          case SEND_GLOBAL_EVENT:
            sendGlobalEvent((JSONObject) msg.obj);
            break;
          case SEND_EVENT_ANDROID:
            sendEventAndroid((JSONObject) msg.obj);
            break;
          case RELOAD_TEMPLATE:
            reloadTemplate((JSONObject) msg.obj);
            break;
          case UPDATE_FONT_SCALE:
            updateFontScale((JSONObject) msg.obj);
            break;
          case UPDATE_META_DATA:
            updateMetaData((JSONObject) msg.obj);
            break;
          case ON_TESTBENCH_COMPLETE:
            onTestBenchComplete();
            break;
          default:
            break;
        }
      }
    };
  }

  public void registerCallback(TestBenchActionCallback callback) {
    mCallbacks.add(callback);
  }

  public void onLynxViewWillBuild(TestBenchActionManager manager, LynxViewBuilder builder) {
    for (TestBenchActionCallback callback : mCallbacks) {
      callback.onLynxViewWillBuild(manager, builder);
    }
  }

  public void onReplayFinish(int endType) {
    for (TestBenchActionCallback callback : mCallbacks) {
      callback.onReplayFinish(endType);
    }
  }

  public void onTestBenchComplete() {
    for (TestBenchActionCallback callback : mCallbacks) {
      callback.onTestBenchComplete();
    }
  }

  private void onLynxViewDidBuild(@NonNull LynxView kitView, @NonNull Intent intent,
      @NonNull Context context, @NonNull ViewGroup view) {
    for (TestBenchActionCallback callback : mCallbacks) {
      callback.onLynxViewDidBuild(kitView, intent, context, view);
    }
  }

  public void setLynxGroup(LynxGroup lynxGroup) {
    mLynxGroup = lynxGroup;
  }

  public LynxGroup getLynxGroup() {
    return mLynxGroup;
  }

  public Context getContext() {
    return mContext;
  }

  public String getUrl() {
    return mUrl;
  }

  public JSONArray getComponentList() {
    return mComponentList;
  }

  public void startWithUrl(@NonNull String url) {
    if (!url.startsWith(TestBenchEnv.getInstance().mTestBenchUrlPrefix)) {
      return;
    }
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    mUrl = queryMap.getString("url");
    String source = queryMap.getString("source");
    if (null == mUrl) {
      Log.e(TAG, "Testbench url is Null!");
      return;
    }

    mReplayGesture = queryMap.getBoolean("gesture", false);

    mPreDecode = queryMap.getBoolean("enablePreDecode", false);

    mThreadMode = queryMap.getInt("thread_mode", -1);

    mDelayEndInterval = queryMap.getInt("delayEndInterval", 3500);

    mEnableAirStrictMode = queryMap.getBoolean("enableAirStrict", false);

    mLandScape = queryMap.getBoolean("landscape", false);

    mDisableUpdateViewport = queryMap.getBoolean("disableUpdateViewport", false);

    mStateView.setReplayState(TestBenchReplayStateView.DOWNLOAD_JSON_FILE);
    if (null != source) {
      LynxEnv.inst().getTemplateProvider().loadTemplate(source, new AbsTemplateProvider.Callback() {
        @Override
        public void onSuccess(byte[] template) {
          mPreloadedTemplateSource = template;
          downloadRecordedFile();
        }

        @Override
        public void onFailed(String msg) {
          Log.e(TAG, "Load source template js fail!");
        }
      });
    } else {
      downloadRecordedFile();
    }
  }

  private void downloadRecordedFile() {
    if (mUrl.startsWith("asset:///")) {
      try {
        InputStream inputStream = mContext.getAssets().open(mUrl.replace("asset:///", ""));
        final byte[] bytes = Utils.inputStreamToByteArray(inputStream);
        UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
          @Override
          public void run() {
            new InnerCallback().onSuccess(bytes);
          }
        });
      } catch (final IOException e) {
        UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
          @Override
          public void run() {
            new InnerCallback().onFailed(e.getMessage());
          }
        });
      }
    } else {
      LynxEnv.inst().getTemplateProvider().loadTemplate(mUrl, new InnerCallback());
    }
  }

  public void onResume() {
    mLynxView.onEnterForeground();
  }

  public void onPause() {
    mLynxView.onEnterBackground();
  }

  public void updateScreenSize(int width, int height) {
    mLynxView.updateScreenMetrics(width, height);
  }

  public void updateFontScale(JSONObject params) {
    String type = params.optString("type", "unknown");
    float scale = (float) params.optDouble("scale", -1);
    if (type.equals("updateFontScale") && mLynxView != null) {
      mLynxView.updateFontScale(scale);
    } else {
      mRawFontScale = scale;
    }
  }

  public void onExceptionOccurred(String errorMessage) {
    mLynxView.getLynxContext().handleException(new Exception(errorMessage));
  }

  private void handleActionList(JSONArray actionList) {
    long recordTime = 0;
    boolean hasSetReplayStartTime = false;
    if (mPreDecode) {
      preDecodeTemplate(mTemplateBundleParams);
    }
    for (int i = 0; i < actionList.length(); ++i) {
      try {
        JSONObject action = actionList.getJSONObject(i);
        String functionName = action.getString("Function Name");

        if (functionName.equals("loadTemplate") && mPreDecode) {
          functionName = "loadTemplateBundle";
        }
        recordTime = action.getLong("Record Time") * 1000;
        if (action.has("RecordMillisecond")) {
          recordTime = action.getLong("RecordMillisecond");
        }

        if (!hasSetReplayStartTime) {
          TestBenchReplayDataModule.setTime(recordTime);
          hasSetReplayStartTime = true;
        }
        JSONObject params = action.getJSONObject("Params");
        if (!mCanMockFuncMap.containsKey(functionName)) {
          continue;
        }

        if (functionName.equals("sendEventAndroid") && !mReplayGesture) {
          continue;
        }

        if (mStartTime == 0) {
          mStartTime = recordTime;
        }
        // convert delay time from s to ms
        long delay = recordTime - mStartTime;
        // if this function will be called when reload, save to the ArrayList: mReloadFuncSet
        if (mReloadFuncSet.contains(functionName)) {
          mReloadList.add(new ReloadAction(params, mCanMockFuncMap.get(functionName), delay));
        } else {
          dispatchAction(functionName, params, delay);
        }
        if (i == actionList.length() - 1) {
          Message completeMsg = Message.obtain();
          completeMsg.what = ON_TESTBENCH_COMPLETE;
          mHandler.sendMessageDelayed(completeMsg, delay);
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
  }

  public void dispatchAction(String functionName, JSONObject params, long delay) {
    Integer functionID = mCanMockFuncMap.get(functionName);
    if (functionID == null) {
      return;
    }
    Message msg = Message.obtain();
    msg.obj = params;
    msg.what = functionID;
    mHandler.sendMessageDelayed(msg, delay);
  }

  private void mockInvokeMethod(JSONArray invokedData) {
    TestBenchReplayDataModule.addFunctionCallArray(invokedData);
  }

  private void mockCallback(JSONObject callbackData) {
    TestBenchReplayDataModule.addCallbackDictionary(callbackData);
  }

  private void mockComponent(JSONArray componentList) {
    mComponentList = componentList;
  }

  private void addExtraComponent(LynxViewBuilder builder) {
    if (mComponentList == null) {
      return;
    }
    List<Behavior> behaviors = new ArrayList<>();
    behaviors.addAll(new BuiltInBehavior().create());
    if (LynxEnv.inst().getBehaviorBundle() != null) {
      behaviors.addAll(LynxEnv.inst().getBehaviorBundle().create());
    }
    Map<String, Behavior> mBehaviorMap = new HashMap<>();
    for (Behavior behavior : behaviors) {
      mBehaviorMap.put(behavior.getName(), behavior);
    }
    for (int i = 0; i < mComponentList.length(); i++) {
      try {
        JSONObject componentInfo = mComponentList.getJSONObject(i);
        String name = componentInfo.getString("Name");
        int type = componentInfo.getInt("Type");
        if (mBehaviorMap.containsKey(name)) {
          continue;
        }
        if ((type & IS_VIRTUAL_NODE) != 0) {
          builder.addBehavior(new Behavior(name) {
            @Override
            public ShadowNode createShadowNode() {
              return new RawTextShadowNode();
            }
          });
        } else {
          builder.addBehavior(new Behavior(name, (type & IS_FLATTEN_NODE) != 0) {
            @Override
            public LynxUI createUI(LynxContext context) {
              return new UIView(context);
            }
          });
        }

      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
  }

  private void setGlobalProps(JSONObject params) {
    try {
      if (mLynxView != null) {
        mGlobalPropsCache = null;
        mGlobalProps = TemplateData.fromString(params.getJSONObject("global_props").toString());
        LynxUpdateMeta.Builder metaBuilder = new LynxUpdateMeta.Builder();
        metaBuilder.setUpdatedGlobalProps(mGlobalProps);
        mLynxView.updateMetaData(metaBuilder.build());
      } else {
        mGlobalPropsCache = params.getJSONObject("global_props");
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void updateDataPreData(JSONObject params) {
    try {
      String value = params.getJSONObject("value").toString();
      String preprocessorName = params.getString("preprocessorName");
      if (params.has("updatePageOption")) {
        JSONObject option = params.getJSONObject("updatePageOption");
        boolean resetPageData = option.getBoolean("reset_page_data");
        if (resetPageData) {
          TemplateData data = TemplateData.fromString(value);
          mLynxView.resetData(data);
        }
      }
      mLynxView.updateData(value, preprocessorName);
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void reloadTemplate(JSONObject params) {
    try {
      String value = params.getJSONObject("value").toString();
      String preprocessorName = params.getString("preprocessorName");
      TemplateData templateData = TemplateData.fromString(value);
      templateData.markState(preprocessorName);
      templateData.markReadOnly();
      mLynxView.reloadTemplate(templateData);
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private ThreadStrategyForRendering getThreadStrategy(
      int schemaThreadStrategy, int testBenchThreadStrategy) {
    int threadStrategy = testBenchThreadStrategy;
    if (schemaThreadStrategy >= ThreadStrategyForRendering.ALL_ON_UI.id()
        && schemaThreadStrategy <= ThreadStrategyForRendering.MULTI_THREADS.id()) {
      // prioritize thread strategy from schema over threadStrategyData
      threadStrategy = schemaThreadStrategy;
    }
    switch (threadStrategy) {
      case 1:
        return ThreadStrategyForRendering.MOST_ON_TASM;
      case 2:
        return ThreadStrategyForRendering.PART_ON_LAYOUT;
      case 3:
        return ThreadStrategyForRendering.MULTI_THREADS;
      default:
        return ThreadStrategyForRendering.ALL_ON_UI;
    }
  }

  //  @IntDef({UNSPECIFIED, EXACTLY, AT_MOST})

  private int getMeasureMode(int param) {
    switch (param) {
      case 1:
        return View.MeasureSpec.EXACTLY;
      case 2:
        return View.MeasureSpec.AT_MOST;
      default:
        return View.MeasureSpec.UNSPECIFIED;
    }
  }

  private boolean enableJSRuntime() {
    boolean enableJSRuntime = true;
    if (mThreadStrategyData.has("enableJSRuntime")) {
      try {
        enableJSRuntime =
            mThreadStrategyData.getBoolean("enableJSRuntime") && !mEnableAirStrictMode;
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
    return enableJSRuntime;
  }

  private boolean enableAir() {
    return !enableJSRuntime();
  }

  private int[] getMeasureSpec(JSONObject params) {
    try {
      int widthMode = params.getInt("layoutWidthMode");
      int heightMode = params.getInt("layoutHeightMode");
      int preferredLayoutHeight = params.getInt("preferredLayoutHeight");
      int preferredLayoutWidth = params.getInt("preferredLayoutWidth");
      // Get the size information of the recorded phone screen。
      // preferredLayoutHeight = (preferredLayoutHeight / recordScreenHeight) * mScreenHeight;
      if (mConfig != null && mConfig.has("screenWidth") && mConfig.has("screenHeight")) {
        int recordScreenWidth = mConfig.getInt("screenWidth");
        int recordScreenHeight = mConfig.getInt("screenHeight");
        if (recordScreenWidth != 0 && recordScreenHeight != 0) {
          preferredLayoutHeight =
              (int) (((double) preferredLayoutHeight / recordScreenHeight) * mScreenHeight);
          preferredLayoutWidth =
              (int) (((double) preferredLayoutWidth / recordScreenWidth) * mScreenWidth);
        }
      }
      int heightMeasureSpec =
          View.MeasureSpec.makeMeasureSpec(preferredLayoutHeight, getMeasureMode(heightMode));
      int widthMeasureSpec =
          View.MeasureSpec.makeMeasureSpec(preferredLayoutWidth, getMeasureMode(widthMode));
      if (mLandScape) {
        return new int[] {heightMeasureSpec, widthMeasureSpec};
      }
      return new int[] {widthMeasureSpec, heightMeasureSpec};
    } catch (Exception e) {
      e.printStackTrace();
      return null;
    }
  }

  private void initialLynxView(JSONObject params) {
    if (BuildConfig.enable_frozen_mode) {
      LynxEnv.inst().setCreateViewAsync(false);
    }
    if (params.has("threadStrategy")) {
      mThreadStrategyData = params;
      return;
    } else {
      if (mThreadStrategyData == null && mLynxView == null) {
        return;
      }
    }
    try {
      int[] measureSpec = getMeasureSpec(params);
      if (mLynxView == null) {
        LynxViewBuilder builder = new LynxViewBuilder();
        builder.setEnableJSRuntime(enableJSRuntime());
        builder.setEnableAirStrictMode(enableAir());
        builder.setPresetMeasuredSpec(measureSpec[0], measureSpec[1]);
        addExtraComponent(builder);
        builder.registerModule("TestBenchReplayDataModule", TestBenchReplayDataModule.class);
        builder.registerModule("TestBenchOpenUrlModule", TestBenchOpenUrlModule.class);
        if (BuildConfig.enable_frozen_mode) {
          builder.setThreadStrategyForRendering(ThreadStrategyForRendering.ALL_ON_UI);
        } else {
          builder.setThreadStrategyForRendering(
              getThreadStrategy(mThreadMode, mThreadStrategyData.getInt("threadStrategy")));
        }

        TestBenchSourceProvider provider = new TestBenchSourceProvider();
        if (mConfig != null && mConfig.has("urlRedirect")) {
          provider.setUrlRedirect(mConfig.getJSONObject("urlRedirect"));
        }
        builder.setResourceProvider(LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS, provider);
        builder.setDynamicComponentFetcher(mDynamicFetcher);
        if (mRawFontScale != -1) {
          builder.setFontScale(mRawFontScale);
        }
        onLynxViewWillBuild(this, builder);
        mLynxView = builder.build(mContext);
        mLynxView.getLynxContext().setLynxView(mLynxView);
        mViewClient = new TestBenchLynxViewClient();
        mLynxView.addLynxViewClient(mViewClient);
        mThreadStrategyData = null;
        onLynxViewDidBuild(mLynxView, mIntent, mContext, mViewGroup);
      } else {
        if (!mDisableUpdateViewport) {
          mLynxView.updateViewport(measureSpec[0], measureSpec[1]);
        }
        if (needSetContainerSize()) {
          int[] containerSize = getContainerSize(new int[] {
              View.MeasureSpec.getSize(measureSpec[0]), View.MeasureSpec.getSize(measureSpec[1])});
          setViewLayoutParams(mViewGroup, containerSize[0], containerSize[1]);
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
  }

  private int[] getContainerSize(int[] defaultSize) {
    JSONObject containerSize = mConfig.optJSONObject("setContainerSize");
    if (containerSize == null) {
      Log.i(TAG, "Miss setContainerSize property in config field");
      return defaultSize;
    }
    return new int[] {containerSize.optInt("width", defaultSize[0]),
        containerSize.optInt("height", defaultSize[1])};
  }

  private boolean needSetContainerSize() {
    return mConfig != null && mConfig.has("setContainerSize");
  }

  private void setViewLayoutParams(View view, int width, int height) {
    ViewGroup.LayoutParams layoutParams = view.getLayoutParams();
    if (layoutParams.width != width || layoutParams.height != height) {
      layoutParams.width = width;
      layoutParams.height = height;
      view.setLayoutParams(layoutParams);
    }
  }

  private TemplateData buildTemplateData(JSONObject templateData) {
    TemplateData templateInitData = TemplateData.fromString(templateData.toString());
    try {
      String processorName = templateData.getString("preprocessorName");
      boolean readOnly = templateData.getBoolean("readOnly");
      if (!processorName.isEmpty()) {
        templateInitData.markState(processorName);
      }
      if (readOnly) {
        templateInitData.markReadOnly();
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
    return templateInitData;
  }

  private void loadTemplate(JSONObject params) {
    try {
      String url = TestBenchEnv.getInstance().mTestBenchUrlPrefix;
      if (mGlobalPropsCache != null) {
        mGlobalPropCacheCloned = TemplateData.fromString(mGlobalPropsCache.toString());
        LynxUpdateMeta.Builder metaBuilder = new LynxUpdateMeta.Builder();
        metaBuilder.setUpdatedGlobalProps(mGlobalPropCacheCloned);
        mLynxView.updateMetaData(metaBuilder.build());
      }
      if (mPreloadedTemplateSource != null) {
        templateSource = mPreloadedTemplateSource;
      } else {
        templateSource = Base64.decode(params.getString("source"), Base64.DEFAULT);
      }
      JSONObject templateData = params.getJSONObject("templateData");
      TemplateData templateInitData = buildTemplateData(templateData);
      mLoadTemplateURL = url;
      // remove loading view when loadTemplate
      if (mStateView != null && mViewGroup != null) {
        mViewGroup.removeView(mStateView);
      }
      boolean isCSR = true;
      if (params.has("isCSR")) {
        isCSR = params.getBoolean("isCSR");
      }
      if (isCSR) {
        mLoadTemplateData = templateInitData.deepClone();
        templateInitData.recycle();
        if (!mSSRLoaded) {
          mLynxView.renderTemplateWithBaseUrl(templateSource, mLoadTemplateData, url);
        } else {
          mLynxView.ssrHydrate(templateSource, url, mLoadTemplateData);
        }
      } else {
        Map<String, Object> dataMap = (Map) (templateInitData.toMap());
        mLynxView.renderSSR(templateSource, url, dataMap);
        mSSRLoaded = true;
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void preDecodeTemplate(JSONObject params) {
    try {
      if (mPreloadedTemplateSource != null) {
        templateSource = mPreloadedTemplateSource;
      } else {
        templateSource = Base64.decode(params.getString("source"), Base64.DEFAULT);
      }
      mTemplateBundle = TemplateBundle.fromTemplate(templateSource, mTemplateBundleOptions);
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void updateMetaData(JSONObject params) {
    try {
      if (mLynxView == null) {
        return;
      }
      JSONObject templateDataJson = params.getJSONObject("templateData");
      JSONObject globalPropsJson = params.getJSONObject("global_props");

      String preprocessorName = templateDataJson.getString("preprocessorName");
      boolean readOnly = templateDataJson.getBoolean("readOnly");

      TemplateData templateData =
          TemplateData.fromString(templateDataJson.getJSONObject("value").toString());

      if (!preprocessorName.equals("")) {
        templateData.markState(preprocessorName);
      }
      if (readOnly) {
        templateData.markReadOnly();
      }
      TemplateData globalProps = TemplateData.fromString(globalPropsJson.toString());
      LynxUpdateMeta.Builder metaBuilder = new LynxUpdateMeta.Builder();
      metaBuilder.setUpdatedGlobalProps(globalProps).setUpdatedData(templateData);
      mLynxView.updateMetaData(metaBuilder.build());
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void loadTemplateBundle(JSONObject params) {
    try {
      String url = TestBenchEnv.getInstance().mTestBenchUrlPrefix;
      if (mGlobalPropsCache != null) {
        mGlobalPropCacheCloned = TemplateData.fromString(mGlobalPropsCache.toString());
        LynxUpdateMeta.Builder metaBuilder = new LynxUpdateMeta.Builder();
        metaBuilder.setUpdatedGlobalProps(mGlobalPropCacheCloned);
        mLynxView.updateMetaData(metaBuilder.build());
      }
      JSONObject templateData = params.getJSONObject("templateData");
      TemplateData templateInitData = buildTemplateData(templateData);
      mLoadTemplateURL = url;
      // remove loading view when loadTemplate
      if (mStateView != null && mViewGroup != null) {
        mViewGroup.removeView(mStateView);
      }
      mLoadTemplateData = templateInitData.deepClone();
      templateInitData.recycle();

      if (mTemplateBundle == null) {
        if (mPreloadedTemplateSource != null) {
          templateSource = mPreloadedTemplateSource;
        } else {
          templateSource = Base64.decode(params.getString("source"), Base64.DEFAULT);
        }
        mLynxView.renderTemplateBundle(
            TemplateBundle.fromTemplate(templateSource, mTemplateBundleOptions), mLoadTemplateData,
            url);
      } else {
        mLynxView.renderTemplateBundle(mTemplateBundle, mLoadTemplateData, url);
      }
    } catch (JSONException e) {
      e.printStackTrace();
    }
  }

  private void sendEventAndroid(JSONObject params) {
    if (mConfig != null && mConfig.has("screenWidth") && mConfig.has("screenHeight")) {
      try {
        int recordScreenWidth = mConfig.getInt("screenWidth");
        int recordScreenHeight = mConfig.getInt("screenHeight");
        if (recordScreenWidth != 0 && recordScreenHeight != 0) {
          float xScaling = (float) mScreenWidth / recordScreenWidth;
          float yScaling = (float) mScreenHeight / recordScreenHeight;
          TestBenchEventSend.sendEventAndroid(params, mLynxView, xScaling, yScaling);
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    } else {
      TestBenchEventSend.sendEventAndroid(params, mLynxView, 1, 1);
    }
  }

  private void sendGlobalEvent(JSONObject params) {
    TestBenchEventSend.sendGlobalEvent(params, mLynxView);
  }

  private void endTestbench() {
    final String path = this.getDumpFilePath();
    Runnable runnable = new Runnable() {
      @Override
      public void run() {
        onReplayFinish(sEndForAll);
        LynxBaseInspectorOwner owner = mLynxView.getBaseInspectorOwner();
        if (owner != null) {
          owner.endTestbench(path);
        }
      }
    };
    new Handler(Looper.getMainLooper()).postDelayed(runnable, 1000);
  }

  private String getDumpFilePath() {
    SimpleDateFormat formatter = new SimpleDateFormat("yyyy-MM-dd-HHmmss", Locale.US);
    formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
    File dir = mContext.getExternalFilesDir(null);
    File file = new File(dir, "lynx-testbench-" + formatter.format(new Date()));
    return file.getPath();
  }

  public void load() {
    if (mLoadTemplateURL != null && mLoadTemplateData != null) {
      TemplateData old_data = mLoadTemplateData;
      mLoadTemplateData = mLoadTemplateData.deepClone();
      old_data.recycle();
      mLynxView.renderTemplateWithBaseUrl(templateSource, mLoadTemplateData, mLoadTemplateURL);
    }
  }

  // when page reload, execute these functions in order
  public void reloadAction() {
    long mLastDelayTime = 1000;
    for (int index = 0; index < mReloadList.size(); index++) {
      mReloadList.get(index).run();
      if (index == mReloadList.size() - 1) {
        // Exec EndTest Action at last action dealy time + 3000ms.
        mLastDelayTime = mReloadList.get(index).getDelayTime() + mDelayEndInterval;
      }
    }
    Message endMsg = Message.obtain();
    endMsg.what = END_TESTBENCH;
    // TODO(zhaosong.lmm): This is to prevent timeouts when retrieving layout files in e2e testing.
    // Just a temporary fix and we need to adopt a more reasonable approach.
    if (mLastDelayTime >= 60 * 1000) {
      mLastDelayTime = 5 * 1000;
    }
    mHandler.sendMessageDelayed(endMsg, mLastDelayTime);
  }

  public void destroy() {
    if (mLoadTemplateData != null) {
      mLoadTemplateData.recycle();
    }

    if (mGlobalPropCacheCloned != null) {
      mGlobalPropCacheCloned.recycle();
    }
    if (mGlobalProps != null) {
      mGlobalProps.recycle();
    }
    if (mLynxView != null) {
      mLynxView.destroy();
    }
    mHandler.removeCallbacksAndMessages(null);
  }
}
