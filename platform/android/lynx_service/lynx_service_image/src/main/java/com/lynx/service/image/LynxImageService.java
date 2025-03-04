// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.image;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.view.View;
import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.facebook.cache.common.CacheKey;
import com.facebook.common.executors.UiThreadImmediateExecutorService;
import com.facebook.common.references.CloseableReference;
import com.facebook.datasource.BaseDataSubscriber;
import com.facebook.datasource.DataSource;
import com.facebook.drawable.base.DrawableWithCaches;
import com.facebook.drawee.backends.pipeline.DefaultDrawableFactory;
import com.facebook.drawee.backends.pipeline.Fresco;
import com.facebook.fresco.animation.drawable.AnimatedDrawable2;
import com.facebook.fresco.animation.drawable.BaseAnimationListener;
import com.facebook.imagepipeline.cache.MemoryCache;
import com.facebook.imagepipeline.common.ImageDecodeOptionsBuilder;
import com.facebook.imagepipeline.common.Priority;
import com.facebook.imagepipeline.image.CloseableImage;
import com.facebook.imagepipeline.request.ImageRequest;
import com.facebook.imagepipeline.request.ImageRequestBuilder;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.service.image.decoder.LoopCountModifyingBackend;
import com.lynx.service.image.utils.ImageUtils;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.background.BackgroundLayerDrawable;
import com.lynx.tasm.behavior.ui.image.BackgroundImageDrawable;
import com.lynx.tasm.behavior.ui.image.FlattenUIImage;
import com.lynx.tasm.behavior.ui.image.InlineImageShadowNode;
import com.lynx.tasm.behavior.ui.image.UIImage;
import com.lynx.tasm.image.AutoSizeImage;
import com.lynx.tasm.image.ImageErrorCodeUtils;
import com.lynx.tasm.image.model.AnimationListener;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.service.ILynxImageService;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;

@Keep
public class LynxImageService implements ILynxImageService {
  public static final String PRIORITY_KEY = "priority";
  public static final String PRIORITY_LOW = "low";
  public static final String PRIORITY_MEDIUM = "medium";
  public static final String PRIORITY_HIGH = "high";
  public static final String CACHE_TARGET_KEY = "cacheTarget";
  public static final String CACHE_DISK = "disk";
  public static final String CACHE_BITMAP = "bitmap";

  private volatile static LynxImageService sInstance = null;

  private LynxImageService() {
    super();
    List<Behavior> behaviorList = new ArrayList<>();
    behaviorList.add(new Behavior("image", true, true) {
      @Override
      public LynxUI createUI(LynxContext context) {
        return new UIImage(context);
      }

      @Override
      public LynxFlattenUI createFlattenUI(final LynxContext context) {
        return new FlattenUIImage(context);
      }
      @Override
      public ShadowNode createShadowNode() {
        return new AutoSizeImage();
      }
    });
    behaviorList.add(new Behavior("inline-image", false, true) {
      @Override
      public ShadowNode createShadowNode() {
        return new InlineImageShadowNode();
      }
    });
    LynxEnv.inst().addBehaviors(behaviorList);
  }

  public static LynxImageService getInstance() {
    if (sInstance == null) {
      synchronized (LynxImageService.class) {
        if (sInstance == null) {
          sInstance = new LynxImageService();
        }
      }
    }
    return sInstance;
  }

  private DefaultDrawableFactory mDefaultDrawableFactory;
  private MemoryCache<CacheKey, CloseableImage> mMemoryCache;

  ConcurrentHashMap<ImageRequestInfo, CloseableReference<CloseableImage>> mImageReferenceMap =
      new ConcurrentHashMap<>();

  @Override
  public void fetchImage(@NonNull ImageRequestInfo imageRequestInfo,
      @NonNull ImageLoadListener loadListener, @Nullable AnimationListener animationListener,
      @NonNull Context context) {
    if (mDefaultDrawableFactory == null) {
      mDefaultDrawableFactory = new DefaultDrawableFactory(context.getResources(),
          Fresco.getImagePipelineFactory().getAnimatedDrawableFactory(context));
    }
    if (mMemoryCache == null) {
      mMemoryCache = Fresco.getImagePipelineFactory().getBitmapMemoryCache();
    }
    ImageRequest imageRequest = ImageUtils.getFrescoImageRequest(imageRequestInfo);
    CloseableReference<CloseableImage> closeableReference = ImageUtils.getCachedImage(
        mMemoryCache, ImageUtils.getCacheKey(imageRequest, imageRequestInfo.getCallerContext()));
    if (closeableReference != null && closeableReference.get() != null) {
      CloseableImage image = closeableReference.get();
      Drawable drawable = mDefaultDrawableFactory.createDrawable(image);
      if (drawable != null) {
        loadListener.onSuccess(
            drawable, imageRequestInfo, new ImageInfo(image.getWidth(), image.getHeight(), false));
        return;
      }
    }
    Fresco.getImagePipeline()
        .fetchDecodedImage(imageRequest, imageRequestInfo.getCallerContext())
        .subscribe(new BaseDataSubscriber<CloseableReference<CloseableImage>>() {
          @Override
          protected void onNewResultImpl(
              DataSource<CloseableReference<CloseableImage>> dataSource) {
            handleImageLoadSuccess(dataSource, imageRequestInfo, loadListener, animationListener);
          }

          @Override
          protected void onFailureImpl(DataSource<CloseableReference<CloseableImage>> dataSource) {
            final int errorCode =
                ImageErrorCodeUtils.checkImageException(dataSource.getFailureCause());
            loadListener.onFailure(errorCode, dataSource.getFailureCause());
          }
        }, UiThreadImmediateExecutorService.getInstance());
  }

  private void handleImageLoadSuccess(DataSource<CloseableReference<CloseableImage>> dataSource,
      ImageRequestInfo imageRequestInfo, ImageLoadListener loadListener,
      AnimationListener animationListener) {
    CloseableReference<CloseableImage> reference = dataSource.getResult();
    try {
      Drawable drawable = mDefaultDrawableFactory.createDrawable(reference.get());
      CloseableImage image = reference.get();
      mImageReferenceMap.put(imageRequestInfo, reference.clone());
      boolean isAnim = handleImageAnimListener(imageRequestInfo, drawable, animationListener);
      loadListener.onSuccess(
          drawable, imageRequestInfo, new ImageInfo(image.getWidth(), image.getHeight(), isAnim));
    } catch (Exception exception) {
      CloseableReference.closeSafely(reference);
      loadListener.onFailure(LynxSubErrorCode.E_RESOURCE_IMAGE_PIC_SOURCE, exception);
    } finally {
      CloseableReference.closeSafely(reference);
    }
  }

  private boolean handleImageAnimListener(
      ImageRequestInfo imageRequestInfo, Drawable drawable, AnimationListener animationListener) {
    boolean isAnim = false;
    if (drawable instanceof AnimatedDrawable2) {
      isAnim = true;
      AnimatedDrawable2 animatedDrawable = (AnimatedDrawable2) drawable;
      if (animationListener != null) {
        animatedDrawable.setAnimationListener(new BaseAnimationListener() {
          @Override
          public void onAnimationStart(AnimatedDrawable2 drawable) {
            animationListener.onAnimationStart(drawable);
          }

          @Override
          public void onAnimationStop(AnimatedDrawable2 drawable) {
            if (drawable.isRunning()) {
              animationListener.onAnimationCurrentLoop(drawable);
              animationListener.onAnimationFinalLoop(drawable);
            }
          }

          @Override
          public void onAnimationRepeat(AnimatedDrawable2 drawable) {
            if (drawable.isRunning()) {
              animationListener.onAnimationCurrentLoop(drawable);
            }
          }
        });
      }
      animatedDrawable.setAnimationBackend(new LoopCountModifyingBackend(
          animatedDrawable.getAnimationBackend(), imageRequestInfo.getLoopCount()));
      if (imageRequestInfo.isAutoPlay()) {
        animatedDrawable.start();
      }
    }
    return isAnim;
  }

  @Override
  public boolean startAnimation(@NonNull Drawable animatable) {
    if (animatable instanceof AnimatedDrawable2) {
      ((AnimatedDrawable2) animatable).stop();
      ((AnimatedDrawable2) animatable).start();
      return true;
    }
    return false;
  }

  @Override
  public boolean resumeAnimation(@NonNull Drawable animatable) {
    if (animatable instanceof AnimatedDrawable2) {
      ((AnimatedDrawable2) animatable).start();
      return true;
    }
    return false;
  }

  @Override
  public boolean pauseAnimation(@NonNull Drawable animatable) {
    if (animatable instanceof AnimatedDrawable2) {
      ((AnimatedDrawable2) animatable).stop();
      return true;
    }
    return false;
  }

  @Override
  public boolean stopAnimation(@NonNull Drawable animatable) {
    if (animatable instanceof AnimatedDrawable2) {
      ((AnimatedDrawable2) animatable).stop();
      return true;
    }
    return false;
  }

  private void prefetchImageToDiskCache(
      ImageRequest request, Object callerContext, @Nullable String priorityString) {
    Priority priority;
    if (priorityString != null && priorityString.equals(PRIORITY_HIGH)) {
      priority = Priority.HIGH;
    } else if (priorityString != null && priorityString.equals(PRIORITY_MEDIUM)) {
      priority = Priority.MEDIUM;
    } else {
      priority = Priority.LOW;
    }
    Fresco.getImagePipeline().prefetchToDiskCache(request, callerContext, priority);
  }

  private void prefetchImageToBitmapCache(ImageRequest request, Object callerContext) {
    Fresco.getImagePipeline().prefetchToBitmapCache(request, callerContext);
  }

  public void prefetchImage(
      @NonNull String uri, Object callerContext, @Nullable ReadableMap params) {
    String priorityString = (params == null ? null : params.getString(PRIORITY_KEY, null));
    String cacheString = (params == null ? null : params.getString(CACHE_TARGET_KEY, null));
    Uri imageUri = Uri.parse(uri);
    if (imageUri.getScheme() == null) {
      return;
    }
    ImageRequestBuilder builder = ImageRequestBuilder.newBuilderWithSource(imageUri);
    ImageDecodeOptionsBuilder decodeOptionsBuilder =
        new ImageDecodeOptionsBuilder().setBitmapConfig(Bitmap.Config.ARGB_8888);
    builder.setImageDecodeOptions(decodeOptionsBuilder.build());
    if (cacheString != null && cacheString.equals(CACHE_BITMAP)) {
      prefetchImageToBitmapCache(builder.build(), callerContext);
    } else {
      prefetchImageToDiskCache(builder.build(), callerContext, priorityString);
    }
  }

  @Override
  public void releaseImage(@NonNull ImageRequestInfo imageRequestInfo) {
    if (imageRequestInfo != null) {
      CloseableReference<CloseableImage> closeableReference =
          mImageReferenceMap.remove(imageRequestInfo);
      if (closeableReference != null) {
        CloseableReference.closeSafely(closeableReference);
      }
    }
  }

  @Override
  public void releaseAnimDrawable(@NonNull Drawable drawable) {
    if (drawable instanceof DrawableWithCaches) {
      ((DrawableWithCaches) drawable).dropCaches();
    }
  }

  @Override
  public @Nullable BackgroundLayerDrawable createBackgroundImageDrawable(
      @NonNull Context context, @NonNull String url) {
    return new BackgroundImageDrawable(context, url);
  }

  @Deprecated
  @Override
  public void setCustomImageDecoder(@NonNull Object builder) {}

  @Deprecated
  @Override
  public @Nullable Object getImageSRPostProcessor() {
    return null;
  }

  @Deprecated
  @Override
  public void setImageSRSize(@NonNull Object request, @NonNull View view) {}

  @Deprecated
  @Override
  public void setImageCacheChoice(@NonNull String cacheChoice, @NonNull Object builder) {}

  @Deprecated
  @Override
  public void setImagePlaceHolderHash(@NonNull Object hierarchy, @NonNull Object request,
      @NonNull Object scaleType, @NonNull String hash, @Nullable String metaData, int width,
      int height, int radius, int iterations, boolean isPreView) {}
}
