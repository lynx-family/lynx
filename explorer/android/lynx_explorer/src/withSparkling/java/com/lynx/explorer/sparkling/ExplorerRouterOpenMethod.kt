// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.util.Log
import com.tiktok.sparkling.method.registry.core.BridgePlatformType
import com.tiktok.sparkling.method.registry.core.IDLBridgeMethod
import com.tiktok.sparkling.method.registry.core.annotation.IDLMethodName
import com.tiktok.sparkling.method.registry.core.annotation.IDLMethodParamField
import com.tiktok.sparkling.method.registry.core.annotation.IDLMethodParamModel
import com.tiktok.sparkling.method.registry.core.annotation.IDLMethodResultModel
import com.tiktok.sparkling.method.registry.core.base.AbsSparklingIDLMethod
import com.tiktok.sparkling.method.registry.core.model.idl.CompletionBlock
import com.tiktok.sparkling.method.registry.core.model.idl.IDLMethodBaseParamModel
import com.tiktok.sparkling.method.registry.core.model.idl.IDLMethodBaseResultModel
import com.tiktok.sparkling.method.registry.core.utils.createXModel
import com.tiktok.sparkling.method.router.open.ReplaceType
import com.tiktok.sparkling.method.router.utils.RouterProvider

class ExplorerRouterOpenMethod :
    AbsSparklingIDLMethod<
        ExplorerRouterOpenMethod.OpenParams,
        ExplorerRouterOpenMethod.OpenResult>() {
  @IDLMethodName(
      name = "router.open",
      params = [
        "scheme",
        "replace",
        "replaceType",
        "useSysBrowser",
        "animated",
        "interceptor",
        "extra",
      ])
  final override val name: String = "router.open"

  override fun handle(
      params: OpenParams,
      callback: CompletionBlock<OpenResult>,
      type: BridgePlatformType) {
    val scheme = params.scheme
    if (scheme.isBlank()) {
      callback.onFailure(
          IDLBridgeMethod.INVALID_PARAM,
          "Invalid params: scheme must be a non-empty string",
          null)
      return
    }

    val replaceType = if (!params.replaceType.isNullOrBlank()) {
      try {
        ReplaceType.valueOf(params.replaceType!!)
      } catch (error: IllegalArgumentException) {
        callback.onFailure(
            IDLBridgeMethod.INVALID_PARAM,
            "Invalid replaceType: ${params.replaceType}. " +
                "Valid values are: ${ReplaceType.values().joinToString()}",
            null)
        return
      }
    } else {
      ReplaceType.onlyCloseAfterOpenSucceed
    }

    val context = getSDKContext()?.context
    if (context == null) {
      callback.onFailure(IDLBridgeMethod.FAIL, "Context not provided in host", null)
      return
    }
    val routerDepend = RouterProvider.hostRouterDepend
    if (routerDepend == null) {
      callback.onFailure(IDLBridgeMethod.FAIL, "Router service not available", null)
      return
    }

    val extraParams = mutableMapOf<String, Any>(
        "useSysBrowser" to (params.useSysBrowser ?: false),
        "extra" to (params.extra ?: emptyMap<String, Any?>()),
    )
    val opened = try {
      when {
        params.replace != true ->
          routerDepend.openScheme(getSDKContext(), scheme, extraParams, type, context)
        replaceType == ReplaceType.alwaysCloseBeforeOpen -> {
          routerDepend.closeView(getSDKContext(), type)
          routerDepend.openScheme(getSDKContext(), scheme, extraParams, type, context)
        }
        replaceType == ReplaceType.alwaysCloseAfterOpen -> {
          val result =
              routerDepend.openScheme(getSDKContext(), scheme, extraParams, type, context)
          routerDepend.closeView(getSDKContext(), type)
          result
        }
        else -> {
          val result =
              routerDepend.openScheme(getSDKContext(), scheme, extraParams, type, context)
          if (result) routerDepend.closeView(getSDKContext(), type)
          result
        }
      }
    } catch (error: Exception) {
      Log.e(TAG, "Exception while opening scheme: ${error.message}")
      false
    }

    if (opened) {
      callback.onSuccess(
          OpenResult::class.java.createXModel(getSDKContext()?.containerID))
    } else {
      callback.onFailure(
          IDLBridgeMethod.FAIL, "Failed to open scheme: $scheme", null)
    }
  }

  @IDLMethodParamModel
  interface OpenParams : IDLMethodBaseParamModel {
    @get:IDLMethodParamField(required = true, isGetter = true, keyPath = "scheme")
    val scheme: String

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "replace")
    val replace: Boolean?

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "replaceType")
    val replaceType: String?

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "useSysBrowser")
    val useSysBrowser: Boolean?

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "animated")
    val animated: Boolean?

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "interceptor")
    val interceptor: String?

    @get:IDLMethodParamField(required = false, isGetter = true, keyPath = "extra")
    val extra: Map<String, Any?>?
  }

  @IDLMethodResultModel
  interface OpenResult : IDLMethodBaseResultModel

  companion object {
    private const val TAG = "ExplorerRouterOpen"
  }
}
