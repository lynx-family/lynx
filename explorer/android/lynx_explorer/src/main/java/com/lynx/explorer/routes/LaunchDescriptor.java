// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routes;

import android.net.Uri;
import com.lynx.explorer.utils.QueryMapUtils;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public final class LaunchDescriptor {
  public enum ContainerType { LEGACY, SPARKLING }

  public final String originalUrl;
  public final String resourceUrl;
  public final String pageName;
  public final boolean fullscreen;
  public final int width;
  public final int height;
  public final float density;
  public final String orientation;
  public final Map<String, Object> initialData;
  public final Map<String, Object> globalProps;
  public final Map<String, String> queryParameters;
  public final ContainerType requestedContainerType;

  private LaunchDescriptor(Builder builder) {
    originalUrl = builder.originalUrl;
    resourceUrl = builder.resourceUrl;
    pageName = builder.pageName;
    fullscreen = builder.fullscreen;
    width = builder.width;
    height = builder.height;
    density = builder.density;
    orientation = builder.orientation;
    initialData = Collections.unmodifiableMap(new HashMap<>(builder.initialData));
    globalProps = Collections.unmodifiableMap(new HashMap<>(builder.globalProps));
    queryParameters = Collections.unmodifiableMap(new HashMap<>(builder.queryParameters));
    requestedContainerType = builder.requestedContainerType;
  }

  public String toLegacyUrl() { return originalUrl != null ? originalUrl : resourceUrl; }

  public static Builder fromLegacyUrl(String url, ContainerType requestedContainerType) {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse(url);
    Builder builder = new Builder(url, unwrapLegacyResourceUrl(url), requestedContainerType);
    builder.fullscreen = queryMap.getBoolean("fullscreen", false);
    builder.width = queryMap.getInt("width", -1);
    builder.height = queryMap.getInt("height", -1);
    builder.density = queryMap.getFloat("density", -1f);
    builder.orientation = queryMap.contains("orientation") ? queryMap.getString("orientation") : null;
    builder.pageName = queryMap.contains("page") ? queryMap.getString("page")
        : queryMap.contains("page_name") ? queryMap.getString("page_name") : null;
    queryMap.toMap().forEach((key, value) -> {
      builder.queryParameters.put(key, value);
      builder.globalProps.put(toCamelCase(key), value);
    });
    builder.initialData.put("mockData", "Hello Lynx Explorer");
    return builder;
  }

  public static Builder fromSparklingCanonicalUrl(String url) {
    Uri uri = Uri.parse(url);
    String resource = uri.getQueryParameter("url");
    if (resource == null) resource = uri.getQueryParameter("bundle");
    if (resource == null) resource = uri.getQueryParameter("resource");
    if (resource == null) resource = url;
    Builder builder = fromLegacyUrl(resource, ContainerType.SPARKLING);
    builder.originalUrl = url;
    builder.resourceUrl = resource;
    builder.pageName = uri.getQueryParameter("page") != null ? uri.getQueryParameter("page") : builder.pageName;
    for (String name : uri.getQueryParameterNames()) {
      String value = uri.getQueryParameter(name);
      builder.queryParameters.put(name, value);
      if (name.startsWith("prop_") && value != null) {
        builder.globalProps.put(toCamelCase(name.substring("prop_".length())), value);
      }
    }
    return builder;
  }

  public static boolean isSparklingCanonicalUrl(String url) {
    if (url == null) return false;
    Uri uri = Uri.parse(url);
    String scheme = uri.getScheme();
    return "sparkling".equals(scheme) || "sparkling-lynx".equals(scheme) || "lynx-sparkling".equals(scheme);
  }

  public static String unwrapLegacyResourceUrl(String url) {
    if (url == null) return null;
    if (url.startsWith("file://lynx?local://")) return url.substring("file://lynx?".length());
    return url;
  }

  private static String toCamelCase(String key) {
    int leading = 0;
    while (leading < key.length() && key.charAt(leading) == '_') leading++;
    String[] parts = key.substring(leading).split("_");
    if (parts.length == 0) return key;
    String result = parts[0];
    for (int i = 1; i < parts.length; i++) {
      if (!parts[i].isEmpty()) result += parts[i].substring(0, 1).toUpperCase() + parts[i].substring(1);
    }
    return result;
  }

  public static final class Builder {
    private String originalUrl;
    private String resourceUrl;
    private String pageName;
    private boolean fullscreen;
    private int width = -1;
    private int height = -1;
    private float density = -1f;
    private String orientation;
    private final Map<String, Object> initialData = new HashMap<>();
    private final Map<String, Object> globalProps = new HashMap<>();
    private final Map<String, String> queryParameters = new HashMap<>();
    private ContainerType requestedContainerType;

    private Builder(String originalUrl, String resourceUrl, ContainerType requestedContainerType) {
      this.originalUrl = originalUrl;
      this.resourceUrl = resourceUrl;
      this.requestedContainerType = requestedContainerType;
    }
    public LaunchDescriptor build() { return new LaunchDescriptor(this); }
  }
}
