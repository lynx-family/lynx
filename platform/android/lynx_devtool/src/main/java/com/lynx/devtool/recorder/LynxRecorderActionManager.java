// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtool.recorder;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.Color;
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
import com.lynx.recorder.LynxDebugInfoRecorder;
import com.lynx.tasm.EmbeddedMode;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxGroup;
import com.lynx.tasm.LynxGroup.LynxGroupBuilder;
import com.lynx.tasm.LynxLoadMeta;
import com.lynx.tasm.LynxLoadMode;
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
import com.lynx.tasm.utils.LynxViewBuilderProperty;
import com.lynx.tasm.utils.UIThreadUtils;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
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

public class LynxRecorderActionManager {
  private static final String TAG = "LynxRecorderActionManager";
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
  private static final int SWITCH_ENGINE_FROM_UI_THREAD = 15;
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
    mCanMockFuncMap.put("switchEngineFromUIThread", SWITCH_ENGINE_FROM_UI_THREAD);
  }

  private class LynxRecorderReplayDataProviderInternal implements LynxRecorderReplayDataProvider {
    // "Invoked Method Data" field from record json file
    public JSONArray functionCall;
    // "Callback" field from record json file
    public JSONObject callbackData;
    // replay additional info
    public JSONArray jsbIgnoredInfo;
    // "jsbSettings" field from record json file
    public JSONObject jsbSettings;

    public JSONObject sharedData;

    public JSONArray getFunctionCall() {
      return functionCall;
    }
    public JSONObject getCallbackData() {
      return callbackData;
    }
    public JSONArray getJsbIgnoredInfo() {
      return jsbIgnoredInfo;
    }
    public JSONObject getJsbSettings() {
      return jsbSettings;
    }

    public JSONObject getSharedData() {
      return sharedData;
    }
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

  private class LynxDebugInfoRecorderDelegate implements LynxDebugInfoRecorder {
    private final Map<String, String> debugInfoDict;

    public LynxDebugInfoRecorderDelegate() {
      debugInfoDict = new HashMap<>();
    }

    @Override
    public void setDebugInfo(String url, String debugInfo) {
      debugInfoDict.put(url, debugInfo);
    }

    @Override
    public String getDebugInfo(String url) {
      return debugInfoDict.get(url);
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
  private final ArrayList<LynxRecorderActionCallback> mCallbacks;
  private final Handler mHandler;
  private final Context mContext;
  private String mUrl;
  private String mRawUrl;
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
  private LynxRecorderFetcher mDynamicFetcher;
  private TestBenchLynxViewClient mViewClient;
  private boolean mReplayGesture;
  private boolean mPreDecode;
  private boolean mDisableOptPushStyleToBundle;
  private boolean mEnableUnifyFixedBehavior;
  private boolean mEnableBatchLayoutTaskWithSyncLayout;
  private boolean mEnableNativeScheduleCreateViewAsync;
  private int mDelayEndInterval;
  private LynxRecorderReplayStateView mStateView;
  private int mScreenWidth;
  private float mDensity;
  private int mScreenHeight;
  private LynxView mLynxView;
  private boolean mSSRLoaded;
  private boolean mDisableUpdateViewport;
  private boolean mCreateWhenReload;
  private int mBackgroundColor;
  private boolean mLandScape;
  private boolean mEnableAirStrictMode;
  private boolean mEnableSizeOptimization;
  private boolean mForbidTimeFreeze;
  private List<String> mPreloadScripts;
  private LynxGroup mLynxGroup;
  private float mRawFontScale;
  private String mSourceURL;
  private JSONObject mTemplateBundleParams;
  private TemplateBundle mTemplateBundle;
  private TemplateBundleOption mTemplateBundleOptions;
  private LynxRecorderReplayDataProviderInternal mDataProvider;
  private LynxDebugInfoRecorderDelegate mLynxDebugInfoRecorderDelegate;
  private int mEmbeddedMode = EmbeddedMode.UNSET;

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
        mStateView.setReplayState(LynxRecorderReplayStateView.PARSING_JSON_FILE);
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
          mStateView.setReplayState(LynxRecorderReplayStateView.INVALID_JSON_FILE);
          return;
        }

        JSONObject json = new JSONObject(map);
        if (json.has("Config") && !(json.get("Config").toString().equals("null"))) {
          mConfig = json.getJSONObject("Config");
          if (mConfig.has("jsbIgnoredInfo")) {
            mDataProvider.jsbIgnoredInfo = mConfig.getJSONArray("jsbIgnoredInfo");
          }

          if (mConfig.has("jsbSettings")) {
            mDataProvider.jsbSettings = mConfig.getJSONObject("jsbSettings");
          }
        }
        if (json.has("Debug Info")) {
          JSONArray debugInfo = json.getJSONArray("Debug Info");
          Log.i("LynxRecorderActionManager", "debugInfo: " + debugInfo.toString());
          for (int i = 0; i < debugInfo.length(); ++i) {
            JSONObject info = debugInfo.getJSONObject(i);
            String url = info.getString("url");
            String content = info.getString("content");
            mLynxDebugInfoRecorderDelegate.setDebugInfo(url, content);
          }
        }
        if (json.has("SharedData")) {
          mDataProvider.sharedData = json.getJSONObject("SharedData");
        }
        if (json.has("Invoked Method Data")) {
          mDataProvider.functionCall = json.getJSONArray("Invoked Method Data");
        }
        if (json.has("Callback")) {
          mDataProvider.callbackData = json.getJSONObject("Callback");
        }
        if (json.has("Component List")) {
          mockComponent(json.getJSONArray("Component List"));
        }
        if (json.has("Action List")) {
          if (checkFile(json.getJSONArray("Action List"))) {
            mDynamicFetcher.parse(json.getJSONArray("Action List"));
            mStateView.setReplayState(LynxRecorderReplayStateView.HANDLE_ACTION_LIST);
            handleActionList(json.getJSONArray("Action List"));
          } else {
            mStateView.setReplayState(LynxRecorderReplayStateView.RECORD_ERROR_MISS_TEMPLATEJS);
            return;
          }
        }
      } catch (JSONException e) {
        mStateView.setReplayState(LynxRecorderReplayStateView.INVALID_JSON_FILE);
        e.printStackTrace();
      }
    }

    @Override
    public void onFailed(String msg) {
      mainThreadChecker("onFailed");
      Log.e("LynxRecorderActionManager", "Read recorded file failed!");
      mStateView.setReplayState(LynxRecorderReplayStateView.ERROR_DOWNLOAD_FAILED);
    }

    private void mainThreadChecker(String methodName) {
      if (!Thread.currentThread().equals(Looper.getMainLooper().getThread())) {
        throw new IllegalThreadStateException(
            "Callback " + methodName + "must be fired on main thread.");
      }
    }
  }

  public LynxRecorderActionManager(@NonNull Intent intent, @NonNull Context context,
      @NonNull ViewGroup viewGroup, @Nullable LynxGroup lynxGroup) {
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
    mDataProvider = new LynxRecorderReplayDataProviderInternal();
    mTemplateBundleParams = null;
    mReplayGesture = false;
    mPreDecode = false;
    mThreadMode = -1;
    mStateView = new LynxRecorderReplayStateView(context);
    mDelayEndInterval = 3500;
    mRawFontScale = -1;
    mDynamicFetcher = new LynxRecorderFetcher();
    mLynxDebugInfoRecorderDelegate = new LynxDebugInfoRecorderDelegate();
    mLynxGroup = lynxGroup;
    Resources resources = mContext.getResources();
    DisplayMetrics dm = resources.getDisplayMetrics();
    mScreenWidth = dm.widthPixels;
    mScreenHeight = dm.heightPixels;
    mDensity = dm.density;
    mEnableSizeOptimization = false;
    mForbidTimeFreeze = false;
    mPreloadScripts = new ArrayList<>();
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
          case SWITCH_ENGINE_FROM_UI_THREAD:
            switchEngineFromUIThread((JSONObject) msg.obj);
            break;
          default:
            break;
        }
      }
    };
  }

  public void registerCallback(LynxRecorderActionCallback callback) {
    mCallbacks.add(callback);
  }

  public void onLynxViewWillBuild(LynxRecorderActionManager manager, LynxViewBuilder builder) {
    for (LynxRecorderActionCallback callback : mCallbacks) {
      callback.onLynxViewWillBuild(manager, builder);
    }
  }

  public void onReplayFinish(int endType) {
    for (LynxRecorderActionCallback callback : mCallbacks) {
      callback.onReplayFinish(endType);
    }
  }

  public void onTestBenchComplete() {
    for (LynxRecorderActionCallback callback : mCallbacks) {
      callback.onTestBenchComplete();
    }
  }

  private void onLynxViewDidBuild(@NonNull LynxView kitView, @NonNull Intent intent,
      @NonNull Context context, @NonNull ViewGroup view) {
    for (LynxRecorderActionCallback callback : mCallbacks) {
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

  public String getRawUrl() {
    return mRawUrl;
  }

  public JSONArray getComponentList() {
    return mComponentList;
  }

  public String[] getTestBenchPreloadScripts() {
    return mPreloadScripts.toArray(new String[0]);
  }

  public void startWithUrl(@NonNull String url) {
    if (!url.startsWith(LynxRecorderEnv.getInstance().lynxRecorderUrlPrefix)) {
      mStateView.setReplayState(LynxRecorderReplayStateView.ERROR_MISS_LYNXRECORDER_HEADER);
      mViewGroup.addView(mStateView);
      return;
    }
    mRawUrl = url;
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    mUrl = queryMap.getString("url");
    mSourceURL = queryMap.getString("source");
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

    mCreateWhenReload = queryMap.getBoolean("createWhenReload", false);

    mDisableOptPushStyleToBundle = queryMap.getInt("disable_opt_push_style_to_bundle", 0) == 1;

    mEnableUnifyFixedBehavior = queryMap.getInt("enable_unify_fixed_behavior", 0) == 1;

    mEnableBatchLayoutTaskWithSyncLayout =
        queryMap.getInt("enable_batch_layout_task_with_sync_layout", 0) == 1;

    mEnableNativeScheduleCreateViewAsync =
        queryMap.getInt("enable_native_schedule_create_view_async", 0) == 1;

    mEmbeddedMode = queryMap.getInt("embedded_mode", 0);

    int[] colorValues = {255, 255, 255, 255};

    String rgba = queryMap.getString("backgroundColor");

    if (rgba != null) {
      try {
        String[] values = rgba.split("_");
        for (int i = 0; i < values.length && i < 4; i++) {
          colorValues[i] = Integer.parseInt(values[i]);
        }
      } catch (IllegalArgumentException e) {
        Log.e(TAG, "set background color failed:" + e.toString());
      }
    }

    mBackgroundColor = Color.argb(colorValues[3], colorValues[0], colorValues[1], colorValues[2]);
    mViewGroup.setBackgroundColor(mBackgroundColor);
    mEnableSizeOptimization = queryMap.getBoolean("enableSizeOptimization", false);
    mForbidTimeFreeze = queryMap.getBoolean("forbidTimeFreeze", false);

    create();
  }

  private void create() {
    destroy();
    if (mLynxView != null) {
      mViewGroup.removeView(mLynxView);
      mLynxView = null;
    }

    mStateView.setReplayState(LynxRecorderReplayStateView.DOWNLOAD_JSON_FILE);
    mViewGroup.addView(mStateView);

    if (null != mSourceURL) {
      LynxEnv.inst().getTemplateProvider().loadTemplate(
          mSourceURL, new AbsTemplateProvider.Callback() {
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

  private String replayTimeEnvJScript() {
    InputStream in = null;
    try {
      in = this.mContext.getAssets().open("lynx_recorder.js");
      int length = in.available();
      byte[] buffer = new byte[length];
      in.read(buffer);

      String script = new String(buffer);

      script = script.replace("###LYNX_RECORDER_REPLAY_TIME###", String.valueOf(mStartTime));

      File file = new File(this.mContext.getFilesDir(), "lynx_recorder.js");
      if (file.exists()) {
        if (!file.delete()) {
          Log.e("TestBench", "Call replayTimeEnvJScript failed: file can't be deleted.");
          return "";
        }
      }
      if (file.createNewFile()) {
        String filePath = "";
        FileOutputStream outStream = null;
        try {
          outStream = new FileOutputStream(file);
          outStream.write(script.getBytes());
          filePath = "file://" + file.getPath();
        } catch (IOException e) {
          filePath = "";
          e.printStackTrace();
        } finally {
          if (outStream != null) {
            try {
              outStream.close();
            } catch (IOException e) {
              e.printStackTrace();
            }
          }
          return filePath;
        }
      } else {
        Log.e("TestBench", "Call replayTimeEnvJScript failed: file can't created.");
        return "";
      }
    } catch (IOException e) {
      e.printStackTrace();
      return "";
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
      (new LynxRecorderTemplateProvider()).loadTemplate(mUrl, new InnerCallback());
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
            public LynxUI createUIWithParams(LynxContext context, Object params) {
              return new UIView(context, params);
            }
          });
        }

      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
  }

  private void preprocessGlobalPropsDictData(JSONObject obj) throws JSONException {
    for (Iterator<String> it = obj.keys(); it.hasNext();) {
      String key = it.next();
      Object value = obj.get(key);
      if (value instanceof JSONObject) {
        preprocessGlobalPropsDictData((JSONObject) value);
      } else if (value instanceof JSONArray) {
        preprocessGlobalPropsArrayData((JSONArray) value);
      } else {
        if (key.equals("screenHeight")) {
          int dpHeight = (int) Math.ceil(mScreenHeight / mDensity);
          obj.put(key, dpHeight);
        } else if (key.equals("screenWidth")) {
          int dpWidth = (int) Math.ceil(mScreenWidth / mDensity);
          obj.put(key, dpWidth);
        }
      }
    }
  }

  private void preprocessGlobalPropsArrayData(JSONArray arr) throws JSONException {
    for (int i = 0; i < arr.length(); i++) {
      Object value = arr.get(i);
      if (value instanceof JSONObject) {
        preprocessGlobalPropsDictData((JSONObject) value);
      } else if (value instanceof JSONArray) {
        preprocessGlobalPropsArrayData((JSONArray) value);
      }
    }
  }

  private void setGlobalProps(JSONObject params) {
    try {
      if (mLynxView != null) {
        mGlobalPropsCache = null;
        JSONObject obj = params.getJSONObject("global_props");
        if (mEnableSizeOptimization) {
          preprocessGlobalPropsDictData(obj);
        }
        mGlobalProps = TemplateData.fromString(obj.toString());
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

  private int getLynxViewLayoutParams(int mode, int value) {
    switch (mode) {
      case View.MeasureSpec.EXACTLY:
        return value;
      default:
        return ViewGroup.LayoutParams.WRAP_CONTENT;
    }
  }

  private void updateViewLayoutParams(int widthLayout, int heightLayout) {
    ViewGroup.LayoutParams layoutParams = mLynxView.getLayoutParams();
    layoutParams.width = widthLayout;
    layoutParams.height = heightLayout;
    mLynxView.setLayoutParams(layoutParams);
  }

  private boolean enableJSRuntime() {
    if (mThreadStrategyData == null) {
      return true;
    }
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
      if (hasScreenSizeInfo()) {
        int recordScreenWidth = mConfig.getInt("screenWidth");
        int recordScreenHeight = mConfig.getInt("screenHeight");
        if (recordScreenWidth != 0 && recordScreenHeight != 0) {
          preferredLayoutHeight = (int) Math.ceil(
              ((double) preferredLayoutHeight / recordScreenHeight) * mScreenHeight);
          preferredLayoutWidth =
              (int) Math.ceil(((double) preferredLayoutWidth / recordScreenWidth) * mScreenWidth);
        }
      }
      int heightMeasureSpec =
          View.MeasureSpec.makeMeasureSpec(preferredLayoutHeight, getMeasureMode(heightMode));
      int widthMeasureSpec =
          View.MeasureSpec.makeMeasureSpec(preferredLayoutWidth, getMeasureMode(widthMode));

      int viewWidthLayoutParams =
          getLynxViewLayoutParams(getMeasureMode(widthMode), preferredLayoutWidth);
      int viewHeightLayoutParams =
          getLynxViewLayoutParams(getMeasureMode(heightMode), preferredLayoutHeight);
      if (mLandScape) {
        return new int[] {
            heightMeasureSpec, widthMeasureSpec, viewHeightLayoutParams, viewWidthLayoutParams};
      }
      return new int[] {
          widthMeasureSpec, heightMeasureSpec, viewWidthLayoutParams, viewHeightLayoutParams};
    } catch (Exception e) {
      e.printStackTrace();
      return null;
    }
  }

  private boolean hasScreenSizeInfo() {
    return mConfig != null && mConfig.has("screenWidth") && mConfig.has("screenHeight");
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
        builder.registerModule(
            "LynxRecorderReplayDataModule", LynxRecorderReplayDataModule.class, mDataProvider);
        builder.registerModule("LynxRecorderOpenUrlModule", LynxRecorderOpenUrlModule.class);
        if (BuildConfig.enable_frozen_mode) {
          builder.setThreadStrategyForRendering(ThreadStrategyForRendering.ALL_ON_UI);
        } else {
          builder.setThreadStrategyForRendering(
              getThreadStrategy(mThreadMode, mThreadStrategyData.getInt("threadStrategy")));
        }

        builder.setEmbeddedMode(mEmbeddedMode);

        LynxRecorderSourceProvider provider = new LynxRecorderSourceProvider();
        if (mConfig != null && mConfig.has("urlRedirect")) {
          provider.setUrlRedirect(mConfig.getJSONObject("urlRedirect"));
        }
        builder.setResourceProvider(LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS, provider);
        builder.setDynamicComponentFetcher(mDynamicFetcher);
        if (mRawFontScale != -1) {
          builder.setFontScale(mRawFontScale);
        }
        if (!mForbidTimeFreeze) {
          mPreloadScripts.add(replayTimeEnvJScript());
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
      if (hasScreenSizeInfo() && mEnableSizeOptimization) {
        updateViewLayoutParams(measureSpec[2], measureSpec[3]);
      }
      if (mLynxView.getBaseInspectorOwner() != null) {
        mLynxView.getBaseInspectorOwner().setDebugInfoInterceptor(mLynxDebugInfoRecorderDelegate);
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

  private LynxView buildFakeLynxView() {
    if (mLynxView == null) {
      try {
        LynxViewBuilder builder = new LynxViewBuilder();
        builder.setEnableJSRuntime(enableJSRuntime());
        builder.setEnableAirStrictMode(enableAir());
        builder.setPresetMeasuredSpec(0, 0);
        addExtraComponent(builder);
        builder.registerModule(
            "LynxRecorderReplayDataModule", LynxRecorderReplayDataModule.class, mDataProvider);
        builder.registerModule("LynxRecorderOpenUrlModule", LynxRecorderOpenUrlModule.class);
        if (BuildConfig.enable_frozen_mode) {
          builder.setThreadStrategyForRendering(ThreadStrategyForRendering.ALL_ON_UI);
        } else {
          builder.setThreadStrategyForRendering(getThreadStrategy(mThreadMode, 0));
        }

        LynxRecorderSourceProvider provider = new LynxRecorderSourceProvider();
        if (mConfig != null && mConfig.has("urlRedirect")) {
          provider.setUrlRedirect(mConfig.getJSONObject("urlRedirect"));
        }
        builder.setResourceProvider(LynxProviderRegistry.LYNX_PROVIDER_TYPE_EXTERNAL_JS, provider);
        builder.setDynamicComponentFetcher(mDynamicFetcher);
        if (mRawFontScale != -1) {
          builder.setFontScale(mRawFontScale);
        }
        if (!mForbidTimeFreeze) {
          mPreloadScripts.add(replayTimeEnvJScript());
        }
        onLynxViewWillBuild(this, builder);
        mLynxView = builder.build(mContext);
        mLynxView.getLynxContext().setLynxView(mLynxView);
        mViewClient = new TestBenchLynxViewClient();
        mLynxView.addLynxViewClient(mViewClient);
        mThreadStrategyData = null;
        onLynxViewDidBuild(mLynxView, mIntent, mContext, mViewGroup);
        if (mLynxView.getBaseInspectorOwner() != null) {
          mLynxView.getBaseInspectorOwner().setDebugInfoInterceptor(mLynxDebugInfoRecorderDelegate);
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    }
    return mLynxView;
  }

  private void avoidLynxViewBeingNull() {
    if (mLynxView == null) {
      mLynxView = buildFakeLynxView();
    }
  }

  private void loadTemplate(JSONObject params) {
    avoidLynxViewBeingNull();
    try {
      String url = LynxRecorderEnv.getInstance().lynxRecorderUrlPrefix;
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
          LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
          builder.setBinaryData(templateSource);
          builder.setUrl(url);
          builder.setInitialData(mLoadTemplateData);
          JSONObject nativeConfig = new JSONObject();
          if (mDisableOptPushStyleToBundle) {
            nativeConfig.put("enableOptPushStyleToBundle", false);
          }
          if (mEnableNativeScheduleCreateViewAsync) {
            nativeConfig.put("enableNativeScheduleCreateViewAsync", true);
          }
          if (mEnableUnifyFixedBehavior) {
            nativeConfig.put("enableUnifyFixedBehavior", true);
          }
          if (mEnableBatchLayoutTaskWithSyncLayout) {
            nativeConfig.put("enableBatchLayoutTaskWithSyncLayout", true);
          }
          Map lynxConfig = new HashMap<>();
          lynxConfig.put(LynxViewBuilderProperty.PLATFORM_CONFIG.getKey(), nativeConfig.toString());
          builder.setLynxViewConfig(lynxConfig);

          mLynxView.loadTemplate(builder.build());
        } else {
          LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
          builder.setLoadMode(LynxLoadMode.HYDRATE_SSR);
          builder.setUrl(url);
          builder.setBinaryData(templateSource);
          builder.setInitialData(mLoadTemplateData);
          mLynxView.loadTemplate(builder.build());
        }
      } else {
        Map<String, Object> dataMap = (Map) (templateInitData.toMap());
        LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
        builder.setLoadMode(LynxLoadMode.RENDER_SSR);
        builder.setUrl(url);
        builder.setBinaryData(templateSource);
        builder.setInitialData(TemplateData.fromMap(dataMap));
        mLynxView.loadTemplate(builder.build());
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

  private void switchEngineFromUIThread(JSONObject params) {
    try {
      if (mLynxView == null) {
        return;
      }
      boolean attach = params.getBoolean("attach");
      if (attach) {
        mLynxView.attachEngineToUIThread();
      } else {
        mLynxView.detachEngineFromUIThread();
      }
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
    avoidLynxViewBeingNull();
    try {
      String url = LynxRecorderEnv.getInstance().lynxRecorderUrlPrefix;
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
        LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
        builder.setTemplateBundle(
            TemplateBundle.fromTemplate(templateSource, mTemplateBundleOptions));
        builder.setUrl(url);
        builder.setInitialData(mLoadTemplateData);
        JSONObject nativeConfig = new JSONObject();
        if (mDisableOptPushStyleToBundle) {
          nativeConfig.put("enableOptPushStyleToBundle", false);
        }
        if (mEnableNativeScheduleCreateViewAsync) {
          nativeConfig.put("enableNativeScheduleCreateViewAsync", true);
        }
        if (mEnableUnifyFixedBehavior) {
          nativeConfig.put("enableUnifyFixedBehavior", true);
        }
        if (mEnableBatchLayoutTaskWithSyncLayout) {
          nativeConfig.put("enableBatchLayoutTaskWithSyncLayout", true);
        }
        Map lynxConfig = new HashMap<>();
        lynxConfig.put(LynxViewBuilderProperty.PLATFORM_CONFIG.getKey(), nativeConfig.toString());
        builder.setLynxViewConfig(lynxConfig);

        mLynxView.loadTemplate(builder.build());
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
          LynxRecorderEventSend.sendEventAndroid(params, mLynxView, xScaling, yScaling);
        }
      } catch (JSONException e) {
        e.printStackTrace();
      }
    } else {
      LynxRecorderEventSend.sendEventAndroid(params, mLynxView, 1, 1);
    }
  }

  private void sendGlobalEvent(JSONObject params) {
    LynxRecorderEventSend.sendGlobalEvent(params, mLynxView);
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
    if (mCreateWhenReload) {
      create();
    } else {
      if (mLoadTemplateURL != null && mLoadTemplateData != null) {
        TemplateData old_data = mLoadTemplateData;
        mLoadTemplateData = mLoadTemplateData.deepClone();
        old_data.recycle();
        if (null != mSourceURL) {
          LynxEnv.inst().getTemplateProvider().loadTemplate(
              mSourceURL, new AbsTemplateProvider.Callback() {
                @Override
                public void onSuccess(byte[] template) {
                  mLynxView.renderTemplateWithBaseUrl(
                      template, mLoadTemplateData, mLoadTemplateURL);
                }

                @Override
                public void onFailed(String msg) {
                  Log.e(TAG, "Load source template js fail!");
                }
              });
        } else {
          mLynxView.renderTemplateWithBaseUrl(templateSource, mLoadTemplateData, mLoadTemplateURL);
        }
      }
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
