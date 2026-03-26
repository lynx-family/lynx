// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.markdown.IMarkdownViewHandle;
import com.lynx.markdown.IResourceLoader;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.text.TypefaceCache;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.fontface.FontFaceManager;
import java.lang.ref.WeakReference;
import java.util.HashMap;

public class MarkdownResourceLoader implements IResourceLoader {
  private final MarkdownResourceContext mHost;
  private final HashMap<String, MarkdownImageResource> mImageCache = new HashMap<>();
  private final HashMap<String, MarkdownInlineViewHandle> mInlineViewHandles = new HashMap<>();
  private boolean mEnableImageDownSampling = false;
  private boolean mImageSyncLoad = false;

  public MarkdownResourceLoader(MarkdownResourceContext host) {
    mHost = host;
  }

  public void setEnableImageDownSampling(boolean enable) {
    mEnableImageDownSampling = enable;
  }

  public void setImageSyncLoad(boolean syncLoad) {
    mImageSyncLoad = syncLoad;
  }

  public void release() {
    for (MarkdownImageResource image : mImageCache.values()) {
      image.release();
    }
    mImageCache.clear();
    mInlineViewHandles.clear();
  }

  @Override
  public @Nullable Drawable loadImage(String source) {
    LynxContext context = mHost.getLynxContext();
    if (TextUtils.isEmpty(source) || context == null) {
      return null;
    }
    MarkdownImageResource image = mImageCache.get(source);
    if (image == null) {
      image = new MarkdownImageResource(source, mEnableImageDownSampling, mImageSyncLoad, mHost);
      mImageCache.put(source, image);
    }
    return image.getDrawable(mHost.getDrawableCallback());
  }

  @Override
  public @Nullable Typeface loadFont(String family, int weight, int style) {
    LynxContext context = mHost.getLynxContext();
    if (context == null) {
      return null;
    }
    int resolvedStyle = resolveTypefaceStyle(weight, style);
    Typeface typeface;
    if (TextUtils.isEmpty(family)) {
      typeface = Typeface.defaultFromStyle(resolvedStyle);
    } else {
      typeface = TypefaceCache.getTypeface(context, family, resolvedStyle);
    }

    if (typeface != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      typeface = Typeface.create(typeface, Math.max(1, weight), isItalic(resolvedStyle));
    }

    if (!TextUtils.isEmpty(family) && typeface == null) {
      FontFaceManager.getInstance().getTypeface(
          context, family, resolvedStyle, new WeakTypefaceListener(this));
    }
    return typeface;
  }

  @Override
  public @Nullable IMarkdownViewHandle loadInlineView(String id) {
    if (TextUtils.isEmpty(id)) {
      return null;
    }
    MarkdownInlineViewHandle cachedHandle = mInlineViewHandles.get(id);
    if (cachedHandle != null) {
      return cachedHandle;
    }
    NativeLayoutNodeRef layoutNodeRef = null;
    LynxBaseUI ui = null;
    for (int i = 0; i < mHost.getChildCount(); i++) {
      ShadowNode child = mHost.getChildAt(i);
      if (!(child instanceof NativeLayoutNodeRef)) {
        continue;
      }
      NativeLayoutNodeRef candidate = (NativeLayoutNodeRef) child;
      if (!id.equals(candidate.getIdSelector())) {
        continue;
      }
      layoutNodeRef = candidate;
      LynxContext context = mHost.getLynxContext();
      if (context != null && context.getLynxUIOwner() != null) {
        ui = context.getLynxUIOwner().findLynxUIBySign(child.getSignature());
      }
      break;
    }
    if (layoutNodeRef == null) {
      return null;
    }
    MarkdownInlineViewHandle handle = new MarkdownInlineViewHandle(layoutNodeRef, ui, mHost);
    mInlineViewHandles.put(id, handle);
    return handle;
  }

  private void notifyResourceLoaded() {
    if (!mHost.isHostDestroyed()) {
      mHost.onResourceLoaded();
    }
  }

  private static int resolveTypefaceStyle(int weight, int style) {
    boolean bold = weight > 400 || style == Typeface.BOLD || style == Typeface.BOLD_ITALIC;
    boolean italic = isItalic(style) || style == 1;
    if (bold && italic) {
      return Typeface.BOLD_ITALIC;
    }
    if (italic) {
      return Typeface.ITALIC;
    }
    if (bold) {
      return Typeface.BOLD;
    }
    return Typeface.NORMAL;
  }

  private static boolean isItalic(int style) {
    return style == Typeface.ITALIC || style == Typeface.BOLD_ITALIC;
  }

  private static class WeakTypefaceListener implements TypefaceCache.TypefaceListener {
    private final WeakReference<MarkdownResourceLoader> mLoaderRef;

    WeakTypefaceListener(MarkdownResourceLoader loader) {
      mLoaderRef = new WeakReference<>(loader);
    }

    @Override
    public void onTypefaceUpdate(Typeface typeface, int style) {
      MarkdownResourceLoader loader = mLoaderRef.get();
      if (loader == null) {
        return;
      }
      loader.notifyResourceLoaded();
    }
  }
}
