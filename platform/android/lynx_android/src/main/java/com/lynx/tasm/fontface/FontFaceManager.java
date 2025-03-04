// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import android.graphics.Typeface;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.TextUtils;
import android.util.Base64;
import android.util.Pair;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TypefaceCache;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.provider.ILynxResourceResponseDataInfo;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceFetcher;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import com.lynx.tasm.resourceprovider.generic.LynxGenericResourceFetcher;
import com.lynx.tasm.service.LynxResourceServiceRequestParams;
import com.lynx.tasm.utils.CallStackUtil;
import com.lynx.tasm.utils.TypefaceUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class FontFaceManager {
  private static final String TAG = "FontFaceManager";
  // TODO: For resource loading methods such as base64, it can be used as a general utils method.
  private static final String LOCAL_SRC_PREFIX = "file://";
  private static final String LOCAL_ASSET_PREFIX = "asset:///";
  private static final String URL_SRC_PREFIX = "https";
  private static final String URL_HTTP_SRC_PREFIX = "http";
  private static final String BASE64_SRC_PREFIX = "data:";
  private static final String BASE64_SRC_CONTAIN = "base64,";
  private static final String
      TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE_WITH_GENERIC_LYNX_RESOURCE_FETCHER =
          "FontFaceManager.loadTypefaceWithGenericLynxResourceFetcher";
  private static final String TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE =
      "FontFaceManager.loadTypeface";

  private static class Holder {
    static final FontFaceManager INSTANCE = new FontFaceManager();
  }

  public static FontFaceManager getInstance() {
    return Holder.INSTANCE;
  }

  /**
   * key: font face  url/local src
   */
  private Map<String, StyledTypeface> mCacheTypeface = new HashMap<>();
  private List<FontFaceGroup> mLoadingFontFace = new ArrayList<>();

  public Typeface getTypeface(final LynxContext context, final String fontFamily, final int style,
      final TypefaceCache.TypefaceListener listener) {
    final FontFace fontFace = context.getFontFace(fontFamily);
    if (fontFace == null) {
      // can not get fontFace, did you declare @font-face { } in .ttss file ?
      return null;
    }

    synchronized (this) {
      // try to check if the StyledTypeface has been cached, key is : type+url
      final StyledTypeface cached_typeface = getCacheTypeface(fontFace);
      if (cached_typeface != null && cached_typeface.checkTypefaceHasCreated(style)) {
        // before Android 8.1, we need to create Typeface in Main thread, just fallback to do
        // findOrLoadFontFace process if Typeface not created
        return cached_typeface.getStyledTypeFace(style);
      }
    }

    final StyledTypeface typeface = fontFace.getTypeface();
    final Handler handler = new Handler(Looper.myLooper());
    if (typeface != null) {
      // first: get typeface from FontFace
      if (listener != null) {
        handler.post(new Runnable() {
          @Override
          public void run() {
            LLog.i("Lynx", "load font success " + fontFamily + style);
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
              listener.onTypefaceUpdate(typeface.getStyledTypeFace(style), style);
            } else {
              invokeTypefaceListenerOnUIThread(handler, listener, typeface, style);
            }
          }
        });
      }
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
        return typeface.getStyledTypeFace(style);
      } else {
        return typeface.getStyledTypeFace(0);
      }
    }

    LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
      @Override
      public void run() {
        findOrLoadFontFace(context, fontFace, style, listener, handler);
      }
    });
    return null;
  }

  private void findOrLoadFontFace(LynxContext lynxContext, final FontFace fontFace, final int style,
      final TypefaceCache.TypefaceListener listener, final Handler handler) {
    FontFaceGroup group;
    Iterator<Pair<FontFace.TYPE, String>> iterator;
    Iterator<Pair<FontFace.TYPE, String>> iteratorForRetry;
    synchronized (this) {
      // second: get typeface from cached url or local
      final StyledTypeface typeface = getCacheTypeface(fontFace);
      if (typeface != null) {
        // find cache, set to fontFace, we can get typeface directly next time
        fontFace.setStyledTypeface(typeface);
        // cache url or local
        cacheSrc(fontFace, typeface);
        // get styled typeface in this thread
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
          final Typeface font = typeface.getStyledTypeFace(style);
          if (listener == null) {
            return;
          }
          handler.post(new Runnable() {
            @Override
            public void run() {
              LLog.i("Lynx", "load font success");
              listener.onTypefaceUpdate(font, style);
            }
          });
        } else {
          if (listener == null) {
            return;
          }
          invokeTypefaceListenerOnUIThread(handler, listener, typeface, style);
        }
        return;
      }

      // third: load typeface
      // did not find cache, we should load typeface
      for (FontFaceGroup faceGroup : mLoadingFontFace) {
        if (faceGroup.isSameFontFace(fontFace)) {
          // the same typeface is loading
          faceGroup.addFontFace(fontFace);
          faceGroup.addListener(new Pair<>(listener, style));
          return;
        }
      }
      // never load
      group = new FontFaceGroup();
      group.addListener(new Pair<>(listener, style));
      group.addFontFace(fontFace);
      mLoadingFontFace.add(group);
      iterator = fontFace.getSrc().iterator();
      // TODO: Existing for new load link testing, no need to use the old link for backing after the
      // new link is stabilized.
      iteratorForRetry = fontFace.getSrc().iterator();
    }

    if (lynxContext.getGenericResourceFetcher() != null) {
      TraceEvent.beginSection(
          TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE_WITH_GENERIC_LYNX_RESOURCE_FETCHER);
      LLog.i(TAG, "Try to loadTypeface with GenericLynxResourceFetcher.");
      loadTypefaceWithGenericLynxResourceFetcher(
          lynxContext, group, iterator, iteratorForRetry, handler);
      TraceEvent.endSection(
          TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE_WITH_GENERIC_LYNX_RESOURCE_FETCHER);
    } else {
      TraceEvent.beginSection(TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE);
      loadTypeface(lynxContext, group, iterator, handler);
      TraceEvent.endSection(TRACE_FONT_FACE_MANAGER_LOAD_TYPEFACE);
    }
  }

  private synchronized void cacheSrc(FontFace fontFace, StyledTypeface typeface) {
    for (Pair<FontFace.TYPE, String> pair : fontFace.getSrc()) {
      String key = pair.first.name() + pair.second;
      mCacheTypeface.put(key, typeface);
    }
  }

  private String getPathFromFontResourceProvider(@NonNull LynxResourceProvider fontResourceProvider,
      final LynxContext context, final FontFace.TYPE type, final String src) {
    final String[] result = new String[1];

    Bundle requestParams = new Bundle();
    requestParams.putString("type", type.toString());
    LynxResourceRequest<Bundle> resourceRequest = new LynxResourceRequest(src, requestParams);
    fontResourceProvider.request(resourceRequest, new LynxResourceCallback<String>() {
      @Override
      public void onResponse(@NonNull LynxResourceResponse<String> response) {
        super.onResponse(response);
        String path = response.getData();
        if (response.success()) {
          result[0] = path;
        } else {
          reportError(LynxSubErrorCode.E_RESOURCE_FONT_RESOURCE_LOAD_ERROR,
              response.getError().getMessage(), src, null, context);
        }
      }
    });
    return result[0];
  }

  private void loadTypeface(LynxContext context, final FontFaceGroup fontFaceGroup,
      final Iterator<Pair<FontFace.TYPE, String>> iterator, final Handler handler) {
    if (!iterator.hasNext()) {
      // after all src fails to load, the code will run here.
      return;
    }
    Pair<FontFace.TYPE, String> next = iterator.next();
    Typeface typeface = null;

    LynxResourceProvider fontResourceProvider = context.getProviderRegistry().getProviderByKey(
        LynxProviderRegistry.LYNX_PROVIDER_TYPE_FONT);

    // No need to request base64 resource.
    if (fontResourceProvider != null && !next.second.startsWith(BASE64_SRC_PREFIX)) {
      String path =
          getPathFromFontResourceProvider(fontResourceProvider, context, next.first, next.second);
      if (path != null) {
        if (path.startsWith(URL_SRC_PREFIX)) {
          typeface =
              LynxFontFaceLoader.getLoader(context).loadFontFace(context, FontFace.TYPE.URL, path);
        } else if (path.startsWith(LOCAL_SRC_PREFIX)) {
          typeface = createTypefaceFromFile(path.substring(LOCAL_SRC_PREFIX.length()), context);
        } else if (path.startsWith(LOCAL_ASSET_PREFIX)) {
          try {
            typeface = Typeface.createFromAsset(LynxEnv.inst().getAppContext().getAssets(),
                path.substring(LOCAL_ASSET_PREFIX.length()));
          } catch (RuntimeException e) {
            reportError(LynxSubErrorCode.E_RESOURCE_FONT_REGISTER_FAILED,
                "Create typeface from local asset failed", path, e, context);
          }
        }
      }
    }

    if (typeface == null) {
      typeface =
          LynxFontFaceLoader.getLoader(context).loadFontFace(context, next.first, next.second);
    }

    if (typeface == null) {
      // after failing to load typeface through src, a loading attempt will be made to the next src.
      loadTypeface(context, fontFaceGroup, iterator, handler);
      return;
    }
    // typeface load success
    final StyledTypeface styledTypeface = new StyledTypeface(typeface);
    synchronized (this) {
      for (FontFace fontFace : fontFaceGroup.getFontFaces()) {
        fontFace.setStyledTypeface(styledTypeface);
        cacheSrc(fontFace, styledTypeface);
      }
      mLoadingFontFace.remove(fontFaceGroup);
    }
    // Android 8.1 and earlier need to call Typeface.create(mOriginTypeface, style) in the main
    // thread.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      Iterator<Pair<TypefaceCache.TypefaceListener, Integer>> itr =
          fontFaceGroup.getListeners().iterator();
      while (itr.hasNext()) {
        Pair<TypefaceCache.TypefaceListener, Integer> pair = itr.next();
        styledTypeface.getStyledTypeFace(pair.second);
      }
    }
    handler.post(new Runnable() {
      @Override
      public void run() {
        Iterator<Pair<TypefaceCache.TypefaceListener, Integer>> iterator =
            fontFaceGroup.getListeners().iterator();
        while (iterator.hasNext()) {
          final Pair<TypefaceCache.TypefaceListener, Integer> listener = iterator.next();
          iterator.remove();
          if (listener.first == null) {
            continue;
          }
          if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            LLog.i("Lynx", "load font success");
            listener.first.onTypefaceUpdate(
                styledTypeface.getStyledTypeFace(listener.second), listener.second);
          } else {
            invokeTypefaceListenerOnUIThread(
                handler, listener.first, styledTypeface, listener.second);
          }
        }
      }
    });
  }
  // This method is executed asynchronously, and the typeface loading logic in it is completed
  // synchronously.
  private void loadTypefaceWithGenericLynxResourceFetcher(@NonNull LynxContext context,
      @NonNull final FontFaceGroup fontFaceGroup,
      @NonNull final Iterator<Pair<FontFace.TYPE, String>> iterator,
      @NonNull final Iterator<Pair<FontFace.TYPE, String>> iteratorForRetry,
      @NonNull final Handler handler) {
    if (!iterator.hasNext()) {
      // after all src fails to load, the code will run here.
      // load Typeface with GenericLynxResourceFetcher failed, try loadTypeface.
      LLog.w(TAG, "load typeface with GenericLynxResourceFetcher failed, try loadTypeface.");
      loadTypeface(context, fontFaceGroup, iteratorForRetry, handler);
      return;
    }
    Pair<FontFace.TYPE, String> next = iterator.next();
    String fontFaceSRC = next.second;

    Typeface typeface = null;

    if (!TextUtils.isEmpty(fontFaceSRC)) {
      if (FontFace.TYPE.LOCAL == next.first) {
        if (fontFaceSRC.startsWith(LOCAL_SRC_PREFIX)) {
          typeface =
              createTypefaceFromFile(fontFaceSRC.substring(LOCAL_SRC_PREFIX.length()), context);
        }
      } else {
        if (fontFaceSRC.startsWith(BASE64_SRC_PREFIX) && fontFaceSRC.contains(BASE64_SRC_CONTAIN)) {
          typeface = loadFromBase64(context, next.first, fontFaceSRC);
        } else if (fontFaceSRC.startsWith(URL_HTTP_SRC_PREFIX)) {
          typeface = loadTypeFaceFromHttpSRCByGenericResourceFetcher(context, fontFaceSRC);
        } else {
          reportError(LynxSubErrorCode.E_RESOURCE_FONT_SRC_FORMAT_ERROR, "Src format is incorrect",
              fontFaceSRC, null, context);
        }
      }
    }
    // after failing to load typeface through src, a loading attempt will be made to the next src.
    if (typeface == null) {
      loadTypefaceWithGenericLynxResourceFetcher(
          context, fontFaceGroup, iterator, iteratorForRetry, handler);
      return;
    }

    // typeface load success
    LLog.i(TAG, "Lynx load typeface with GenericLynxResourceFetcher success.");
    final StyledTypeface styledTypeface = new StyledTypeface(typeface);
    synchronized (this) {
      for (FontFace fontFace : fontFaceGroup.getFontFaces()) {
        fontFace.setStyledTypeface(styledTypeface);
        cacheSrc(fontFace, styledTypeface);
      }
      mLoadingFontFace.remove(fontFaceGroup);
    }
    // Android 8.1 and earlier need to call Typeface.create(mOriginTypeface, style) in the main
    // thread.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      Iterator<Pair<TypefaceCache.TypefaceListener, Integer>> itr =
          fontFaceGroup.getListeners().iterator();
      while (itr.hasNext()) {
        Pair<TypefaceCache.TypefaceListener, Integer> pair = itr.next();
        styledTypeface.getStyledTypeFace(pair.second);
      }
    }
    handler.post(new Runnable() {
      @Override
      public void run() {
        Iterator<Pair<TypefaceCache.TypefaceListener, Integer>> iterator =
            fontFaceGroup.getListeners().iterator();
        while (iterator.hasNext()) {
          final Pair<TypefaceCache.TypefaceListener, Integer> listener = iterator.next();
          iterator.remove();
          if (listener.first == null) {
            continue;
          }
          typefaceHandlerPost(handler, listener.first, styledTypeface, listener.second);
        }
      }
    });
  }

  @Nullable
  private Typeface loadTypeFaceFromHttpSRCByGenericResourceFetcher(
      @NonNull LynxContext context, @NonNull String fontFaceSRC) {
    // The process of this function is performed synchronously, but since the overall process of
    // getFontFace is asynchronous, this process is also performed asynchronously in the entire
    // process.
    com.lynx.tasm.resourceprovider.LynxResourceRequest
        request = new com.lynx.tasm.resourceprovider.LynxResourceRequest(fontFaceSRC,
        com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType.LynxResourceTypeFont);
    request.setAsyncMode(com.lynx.tasm.resourceprovider.LynxResourceRequest.AsyncMode.EXACTLY_SYNC);
    LynxGenericResourceFetcher genericResourceFetcher = context.getGenericResourceFetcher();
    Typeface typeface[] = new Typeface[1];
    if (genericResourceFetcher != null) {
      genericResourceFetcher.fetchResource(
          request, new com.lynx.tasm.resourceprovider.LynxResourceCallback<byte[]>() {
            @Override
            public void onResponse(
                com.lynx.tasm.resourceprovider.LynxResourceResponse<byte[]> response) {
              if (response.getState()
                      == com.lynx.tasm.resourceprovider.LynxResourceResponse.ResponseState.SUCCESS
                  && null != response.getData()) {
                try {
                  typeface[0] = TypefaceUtils.createFromBytes(context, response.getData());
                } catch (Exception e) {
                  reportError(LynxSubErrorCode.E_RESOURCE_FONT_REGISTER_FAILED,
                      "Create typeface from bytes failed", fontFaceSRC, e, context);
                }
              } else {
                reportError(LynxSubErrorCode.E_RESOURCE_FONT_RESOURCE_LOAD_ERROR,
                    "Load font with genericResourceFetcher failed:"
                        + response.getError().getMessage(),
                    fontFaceSRC, null, context);
              }
            };
          });
    };
    return typeface[0];
  }

  /**
   * get StyledTypeface for fontFace
   *
   * @param fontFace
   * @return
   */
  private synchronized StyledTypeface getCacheTypeface(FontFace fontFace) {
    for (Pair<FontFace.TYPE, String> pair : fontFace.getSrc()) {
      String key = pair.first.name() + pair.second;
      return mCacheTypeface.get(key);
    }
    return null;
  }

  // This method is executed synchronously.
  @Nullable
  private Typeface loadFromBase64(
      @NonNull final LynxContext context, @NonNull FontFace.TYPE type, @NonNull final String src) {
    if (TextUtils.isEmpty(src) || type == FontFace.TYPE.LOCAL) {
      return null;
    }
    final int index = src.indexOf(BASE64_SRC_CONTAIN);
    if (!src.startsWith("data:") || index == -1) {
      return null;
    }
    final String encoded = src.substring(index + BASE64_SRC_CONTAIN.length());
    try {
      byte[] bytes = Base64.decode(encoded, Base64.DEFAULT);
      return TypefaceUtils.createFromBytes(context, bytes);
    } catch (Exception e) {
      reportError(LynxSubErrorCode.E_RESOURCE_FONT_BASE64_PARSING_ERROR,
          "Error when parsing base64 resource", src, e, context);
      return null;
    }
  }

  private void typefaceHandlerPost(@NonNull final Handler handler,
      @NonNull final TypefaceCache.TypefaceListener listener,
      @NonNull final StyledTypeface styledTypeface, @Nullable Integer style) {
    int intStyle = style == null ? 0 : style;
    // Android 8.1 and earlier need to call Typeface.create(mOriginTypeface, style) in the main
    // thread.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      LLog.i(TAG, "Lynx load font success.");
      listener.onTypefaceUpdate(styledTypeface.getStyledTypeFace(intStyle), intStyle);
    } else {
      invokeTypefaceListenerOnUIThread(handler, listener, styledTypeface, intStyle);
    }
  }

  /**
   * Invoke typefaceListener on ui thread.
   *
   * @param listener
   * @param styledTypeface
   * @param style
   */
  private void invokeTypefaceListenerOnUIThread(@NonNull Handler layoutThreadHandler,
      @NonNull final TypefaceCache.TypefaceListener listener,
      @NonNull final StyledTypeface styledTypeface, int style) {
    UIThreadUtils.runOnUiThreadImmediately(new Runnable() {
      @Override
      public void run() {
        // Android 8.1 and earlier need to call Typeface.create(mOriginTypeface, style) in the
        // main thread.
        final Typeface typeface = styledTypeface.getStyledTypeFace(style);
        layoutThreadHandler.post(new Runnable() {
          @Override
          public void run() {
            LLog.i(TAG, "Lynx load font success.");
            listener.onTypefaceUpdate(typeface, style);
          }
        });
      }
    });
  }

  private void reportError(int subErrorCode, @NonNull String errorMsg, @NonNull String src,
      Exception e, LynxContext context) {
    LynxError error = new LynxError(subErrorCode, errorMsg);
    if (e != null) {
      error.setCallStack(CallStackUtil.getStackTraceStringTrimmed(e));
      LLog.e(TAG, e.getMessage());
    } else {
      LLog.e(TAG, errorMsg + ",src:" + src);
    }

    context.reportResourceError(src, "font", error);
  }

  private Typeface createTypefaceFromFile(String filePath, LynxContext context) {
    Typeface typeface = null;
    try {
      typeface = Typeface.createFromFile(filePath);
    } catch (RuntimeException e) {
      reportError(LynxSubErrorCode.E_RESOURCE_FONT_REGISTER_FAILED,
          "Create typeface from local path failed", filePath, e, context);
    }

    return typeface;
  }
}
