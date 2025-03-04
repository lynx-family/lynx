// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import android.content.res.AssetManager;
import android.graphics.Typeface;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.utils.FontFaceParser;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

/**
 * Cache typeface with font family name
 */
public class TypefaceCache {
  private static final String TAG = "TypefaceCache";
  // Typeface.NORMAL = 0
  // Typeface.BOLD = 1
  // Typeface.ITALIC = 2
  // Typeface.BOLD_ITALIC = 3
  private static final int sMaxStyles = 4;
  // Relation for font family name and typefaces of full style
  private static final Map<String, Typeface[]> sFontFamilyCache = new HashMap<>();
  // Relation for typeface and typefaces of full style
  private static final Map<Typeface, Typeface[]> sTypefaceCache = new HashMap<>();
  // Relate for style and file extension
  private static final String[] sFileExtensions = {"", "_bold", "_italic", "_bold_italic"};
  private static final String[] sFileSuffixes = {".ttf", ".otf"};

  private static class AssetsFontCache {
    // mTypeface may be null ,indicates no related Font in Assets
    public Typeface mTypeface;
    AssetsFontCache(Typeface typeface) {
      mTypeface = typeface;
    }
  }
  private static ConcurrentHashMap<String, AssetsFontCache> mAssetFontCache =
      new ConcurrentHashMap<>();

  /**
   * The typeface provided by LazyProvider will be cached into TypefaceCache
   */
  public interface LazyProvider {
    Typeface getTypeface(String fontFamilyName, int style);
  }

  private static final List<LazyProvider> sLazyProviders = new ArrayList<>();

  public synchronized static void addLazyProvider(LazyProvider provider) {
    if (provider == null) {
      return;
    }
    sLazyProviders.add(provider);
  }

  public synchronized static void removeLazyProvider(LazyProvider provider) {
    sLazyProviders.remove(provider);
  }

  public synchronized static boolean containsTypeface(String fontFamilyName) {
    return sFontFamilyCache.containsKey(fontFamilyName);
  }

  public synchronized static boolean containsTypeface(String fontFamilyName, int style) {
    Typeface[] typefaces = sFontFamilyCache.get(fontFamilyName);
    return typefaces != null && typefaces[style] != null;
  }

  /**
   * This method will force cache update, so check font family name exists or not before cache
   * type face throw {@link #containsTypeface(String, int)})})}
   * @param fontFamilyName
   * @param style
   * @param typeface
   */
  public synchronized static void cacheTypeface(
      String fontFamilyName, int style, @NonNull Typeface typeface) {
    Typeface[] typefaces = sFontFamilyCache.get(fontFamilyName);
    if (typefaces == null) {
      typefaces = new Typeface[sMaxStyles];
      sFontFamilyCache.put(fontFamilyName, typefaces);
    }
    typefaces[style] = typeface;
    sTypefaceCache.put(typeface, typefaces);
  }

  public static void cacheFullStyleTypefacesFromAssets(
      AssetManager assetManager, String fontFamilyName, String path) {
    for (int style = 0; style < sMaxStyles; ++style) {
      cacheTypefaceFromAssets(assetManager, fontFamilyName, style, path);
    }
  }

  public static void cacheTypefaceFromAssets(@NonNull AssetManager assetManager,
      @NonNull String fontFamilyName, int style, @NonNull String path) {
    Typeface typeface = getTypefaceFromAssets(assetManager, fontFamilyName, style, path);
    if (typeface != null) {
      cacheTypeface(fontFamilyName, style, typeface);
    }
  }

  public static Typeface getTypefaceFromAssets(@NonNull AssetManager assetManager,
      @NonNull String fontFamilyName, int style, @NonNull String path) {
    String key = fontFamilyName + path + style;
    if (mAssetFontCache.containsKey(key)) {
      return mAssetFontCache.get(key).mTypeface;
    }
    String extension = sFileExtensions[style];
    StringBuilder fileNameBuffer = new StringBuilder(32)
                                       .append(path)
                                       .append(path.endsWith("/") || path.equals("") ? "" : "/")
                                       .append(fontFamilyName)
                                       .append(extension);
    int length = fileNameBuffer.length();
    for (String fileSuffix : sFileSuffixes) {
      String fileName = fileNameBuffer.toString();
      if (!fileName.endsWith(fileSuffix))
        fileName = fileNameBuffer.append(fileSuffix).toString();
      try {
        Typeface ret = Typeface.createFromAsset(assetManager, fileName);
        AssetsFontCache cache = new AssetsFontCache(ret);
        mAssetFontCache.put(key, cache);
        return ret;
      } catch (RuntimeException e) {
        LLog.w(TAG, "Exception happens whe cacheTypefaceFromAssets with message " + e.getMessage());
        fileNameBuffer.setLength(length);
      }
    }
    AssetsFontCache cache = new AssetsFontCache(null);
    mAssetFontCache.put(key, cache);
    return null;
  }

  public static void cacheFullStyleTypefacesFromFile(String fontFamily, String path) {
    for (int style = 0; style < sMaxStyles; ++style) {
      cacheTypefaceFromFile(fontFamily, style, path);
    }
  }

  public static void cacheTypefaceFromFile(
      @NonNull String fontFamilyName, int style, @NonNull String path) {
    TraceEvent.beginSection("text.TypefaceCache.cacheTypefaceFromFile");
    Typeface typeface = getTypefaceFromFile(fontFamilyName, style, path);
    if (typeface != null) {
      cacheTypeface(fontFamilyName, style, typeface);
    }
    TraceEvent.endSection("text.TypefaceCache.cacheTypefaceFromFile");
  }

  public static Typeface getTypefaceFromFile(
      @NonNull String fontFamilyName, int style, @NonNull String path) {
    String extension = sFileExtensions[style];
    StringBuilder fileNameBuffer = new StringBuilder(32)
                                       .append(path)
                                       .append(path.endsWith("/") || path.equals("") ? "" : "/")
                                       .append(fontFamilyName)
                                       .append(extension);
    int length = fileNameBuffer.length();
    for (String fileSuffix : sFileExtensions) {
      String fileName = fileNameBuffer.append(fileSuffix).toString();
      try {
        return Typeface.createFromFile(fileName);
      } catch (RuntimeException e) {
        LLog.w(TAG, "Exception happens whe cacheTypefaceFromFile with message " + e.getMessage());
        fileNameBuffer.setLength(length);
      }
    }
    return null;
  }

  /**
   * maybe async
   */
  public synchronized static Typeface getTypeface(
      LynxContext context, final String fontFamily, final int style) {
    String[] family_array = fontFamily.split(",");
    for (String family : family_array) {
      family = FontFaceParser.trim(family);
      if (!TextUtils.isEmpty(family)) {
        Typeface typeface = getCachedTypeface(family, style);
        if (typeface != null) {
          return typeface;
        }
        typeface = FontFaceManager.getInstance().getTypeface(context, family, style, null);
        if (typeface != null) {
          return typeface;
        }
      }
    }
    LLog.v(TAG, "Can't not find typeface for fontFamily: " + fontFamily + " in lynx.");
    return null;
  }

  public synchronized static Typeface getCachedTypeface(final String fontFamily, final int style) {
    Typeface[] cache = sFontFamilyCache.get(fontFamily);
    if (cache != null && cache[style] != null) {
      // return cached value.
      return cache[style];
    }
    // Get typeface from lazy provider and cache them
    for (LazyProvider provider : sLazyProviders) {
      if (provider == null)
        continue;
      Typeface typeface = provider.getTypeface(fontFamily, style);
      if (typeface != null) {
        cacheTypeface(fontFamily, style, typeface);
        return typeface;
      }
    }

    return null;
  }

  public synchronized static Typeface getTypeface(Typeface family, int style) {
    if (family == null) {
      return Typeface.defaultFromStyle(style);
    }

    Typeface[] typefaceCache = sTypefaceCache.get(family);
    if (typefaceCache == null) {
      typefaceCache = new Typeface[sMaxStyles];
      typefaceCache[family.getStyle()] = family;
    } else if (typefaceCache[style] != null) {
      return typefaceCache[style];
    }

    family = Typeface.create(family, style);
    typefaceCache[style] = family;
    sTypefaceCache.put(family, typefaceCache);
    return family;
  }

  public interface TypefaceListener {
    void onTypefaceUpdate(Typeface typeface, final int style);
  }
}
