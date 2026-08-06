// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.net.Uri
import java.net.URLDecoder
import java.nio.charset.StandardCharsets

class LaunchDescriptorParser @JvmOverloads constructor(
  private val recorderPrefix: String = "sslocal://arkview",
  private val maximumWrapperDepth: Int = 2
) {
  fun parse(input: String, requestedRuntime: RequestedRuntime, source: RouteSource): LaunchDescriptor =
    parseInternal(input, input, requestedRuntime, source, 0)

  private fun parseInternal(input: String, original: String, requested: RequestedRuntime, source: RouteSource, depth: Int): LaunchDescriptor {
    val route = input.trim()
    if (route.isEmpty()) fail("empty_input", "Enter a Lynx bundle URL or a Sparkling hybrid scheme.")
    validatePercentEncoding(route)
    if ('%' in route) decode(route) // Validate percent-encoded bytes as UTF-8 before Uri normalizes them.
    val uri = try { Uri.parse(route) } catch (_: Exception) { fail("malformed_url", "Invalid absolute URL: $route") }
    val scheme = uri.scheme?.lowercase() ?: fail("malformed_url", "Invalid absolute URL: $route")
    if (scheme == "lynx") {
      if (!uri.host.equals("open", true)) fail("unsupported_scheme", "Unsupported Lynx route.")
      if (depth >= maximumWrapperDepth) fail("wrapper_depth_exceeded", "Too many nested lynx://open wrappers.")
      val target = wrapperTarget(uri)
      return parseInternal(target, original, requested, source, depth + 1)
    }
    if (recorderPrefix.isNotEmpty() && route.startsWith(recorderPrefix)) {
      if (requested == RequestedRuntime.SPARKLING) fail("recorder_unsupported_in_sparkling", "Recorder routes can only be opened with Lynx.")
      return descriptor(original, LaunchResource.Recorder(route), requested, ResolvedRuntime.LYNX, source, null, queryItems(uri))
    }
    return when (scheme) {
      "http", "https" -> {
        if (uri.host.isNullOrEmpty()) fail("malformed_url", "Remote bundle has no host.")
        descriptor(original, LaunchResource.RemoteBundle(route), requested, requested.resolveRaw(), source, null, queryItems(uri))
      }
      "assets" -> {
        val path = (uri.host.orEmpty() + uri.path.orEmpty()).trimStart('/')
        if (path.isEmpty()) fail("missing_target", "The route is missing its bundle target.")
        descriptor(original, LaunchResource.LocalBundle(path), requested, requested.resolveRaw(), source, null, queryItems(uri))
      }
      "file" -> parseFile(route, uri, original, requested, source)
      "hybrid" -> parseCanonical(route, uri, original, requested, source)
      else -> fail("unsupported_scheme", "The URL scheme '$scheme' is not supported by Lynx Explorer.")
    }
  }

  private fun parseFile(route: String, uri: Uri, original: String, requested: RequestedRuntime, source: RouteSource): LaunchDescriptor {
    if (!uri.host.equals("lynx", true)) fail("malformed_url", "Invalid local Lynx URL: $route")
    val nested = uri.encodedQuery ?: fail("missing_target", "The route is missing its local target.")
    validatePercentEncoding(nested)
    val local = Uri.parse(normalizeLegacyLocalQuery(nested))
    if (!local.scheme.equals("local", true)) fail("malformed_url", "Invalid local Lynx URL: $route")
    val path = (local.host.orEmpty() + local.path.orEmpty()).trimStart('/')
    if (path.isEmpty()) fail("missing_target", "The route is missing its local target.")
    return descriptor(original, LaunchResource.LocalBundle(path), requested, requested.resolveRaw(), source, null, queryItems(local))
  }

  private fun normalizeLegacyLocalQuery(value: String): String {
    if ('?' in value) return value
    val separator = value.indexOf('&')
    return if (separator < 0) value else value.substring(0, separator) + "?" + value.substring(separator + 1)
  }

  private fun parseCanonical(route: String, uri: Uri, original: String, requested: RequestedRuntime, source: RouteSource): LaunchDescriptor {
    if (!uri.host.equals("lynxview_page", true)) fail("unsupported_hybrid_host", "Unsupported Sparkling hybrid host '${uri.host.orEmpty()}'.")
    val items = queryItems(uri)
    val bundles = items.filter { it.name == "bundle" && !it.value.isNullOrEmpty() }
    val urls = items.filter { it.name == "url" && !it.value.isNullOrEmpty() }
    if (bundles.size + urls.size == 0) fail("missing_target", "The Sparkling route is missing bundle or url.")
    if (bundles.size + urls.size != 1) fail("ambiguous_target", "The Sparkling route must contain exactly one resource target.")
    val resource = if (bundles.isNotEmpty()) LaunchResource.LocalBundle(bundles.single().value!!) else {
      val value = urls.single().value!!
      val target = Uri.parse(value)
      if (target.scheme !in listOf("http", "https")) fail("malformed_sparkling_scheme", "Unsupported Sparkling URL target: $value")
      LaunchResource.RemoteBundle(value)
    }
    return descriptor(original, resource, requested, ResolvedRuntime.SPARKLING, source, route, items)
  }

  private fun descriptor(original: String, resource: LaunchResource, requested: RequestedRuntime, resolved: ResolvedRuntime, source: RouteSource, canonical: String?, items: List<LaunchQueryItem>): LaunchDescriptor {
    fun last(name: String) = items.lastOrNull { it.name == name && it.value != null }?.value
    fun bool(name: String) = last(name)?.let(::legacyBoolean)
    val fullscreen = bool("fullscreen") ?: false
    val hidden = fullscreen || (bool("hide_nav_bar") ?: bool("hidden_nav") ?: false)
    val width = legacyInt(last("width")); val height = legacyInt(last("height"))
    val viewport = if (width != null && height != null && width > 0 && height > 0) ViewportOptions(width, height) else null
    val extras = linkedMapOf<String, String>(); val props = linkedMapOf<String, Any>()
    items.forEach { it.value?.let { value -> extras[it.name] = value; props[camelCase(it.name)] = value } }
    return LaunchDescriptor(
      original, resource, last("initial_page"), viewport,
      AppearanceOptions(last("container_bg_color"), fullscreen || bool("trans_status_bar") == true, fullscreen || bool("hide_status_bar") == true, last("force_theme_style")),
      NavigationOptions(hidden, fullscreen, last("title"), last("title_color"), last("nav_bar_color") ?: last("bar_color"), last("back_button_style"), last("orientation")?.takeIf { it == "portrait" || it == "landscape" }),
      bool("animated") ?: true, last("enable_napi_addon")?.lowercase() in setOf("1", "true", "yes"), items.toList(), extras.toMap(), props.toMap(), requested, resolved, source, canonical)
  }

  private fun wrapperTarget(uri: Uri): String {
    val raw = uri.encodedQuery.orEmpty()
    val marker = "url="
    val starts = raw.split('&').count { it.startsWith(marker) }
    val index = raw.indexOf(marker)
    if (index < 0) fail("missing_target", "The route is missing its url target.")
    if (starts > 1) fail("ambiguous_target", "The route contains more than one url target.")
    val tail = raw.substring(index + marker.length)
    if (tail.substringBefore('&').contains("://") && tail.contains('&')) return decode(tail)
    val target = uri.getQueryParameter("url")
    if (target.isNullOrEmpty()) fail("missing_target", "The route is missing its url target.")
    return target
  }

  private fun queryItems(uri: Uri): List<LaunchQueryItem> {
    val query = uri.encodedQuery ?: return emptyList()
    return query.split('&').map { component ->
      val separator = component.indexOf('=')
      if (separator < 0) LaunchQueryItem(decode(component), null)
      else LaunchQueryItem(decode(component.substring(0, separator)), decode(component.substring(separator + 1)))
    }
  }

  private fun RequestedRuntime.resolveRaw() = if (this == RequestedRuntime.SPARKLING) ResolvedRuntime.SPARKLING else ResolvedRuntime.LYNX
  private fun decode(value: String): String {
    val decoded = try {
      URLDecoder.decode(value.replace("+", "%2B"), StandardCharsets.UTF_8.name())
    } catch (_: IllegalArgumentException) {
      fail("invalid_percent_encoding", "Invalid percent escape in route.")
    }
    if ('\uFFFD' in decoded) fail("invalid_percent_encoding", "The route contains invalid UTF-8 percent encoding.")
    return decoded
  }
  private fun legacyBoolean(value: String): Boolean = value.toDoubleOrNull()?.let { it != 0.0 } ?: value.equals("true", true) || value.equals("yes", true)
  private fun legacyInt(value: String?): Int? {
    if (value == null) return null
    return Regex("^[+-]?\\d+").find(value)?.value?.toLongOrNull()
      ?.takeIf { it in Int.MIN_VALUE..Int.MAX_VALUE }?.toInt()
  }
  private fun camelCase(name: String): String = name.trimStart('_').split('_').filter { it.isNotEmpty() }.let { parts -> parts.firstOrNull().orEmpty() + parts.drop(1).joinToString("") { it.replaceFirstChar(Char::uppercase) } }
  private fun validatePercentEncoding(value: String) {
    var i = 0
    while (i < value.length) { if (value[i] == '%') { if (i + 2 >= value.length || !value[i + 1].isHex() || !value[i + 2].isHex()) fail("invalid_percent_encoding", "Invalid percent escape in route."); i += 3 } else i++ }
  }
  private fun Char.isHex() = this in '0'..'9' || this in 'a'..'f' || this in 'A'..'F'
  private fun fail(code: String, message: String): Nothing = throw RouteException(code, message)
}
