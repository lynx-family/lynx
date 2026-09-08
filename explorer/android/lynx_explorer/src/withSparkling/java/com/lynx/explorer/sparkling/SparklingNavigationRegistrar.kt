// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.app.Activity
import android.app.Application
import android.content.Context
import com.tiktok.sparkling.Sparkling
import com.tiktok.sparkling.SparklingContext
import com.tiktok.sparkling.hybridkit.utils.GlobalPropsUtils
import com.tiktok.sparkling.method.registry.core.BridgePlatformType
import com.tiktok.sparkling.method.registry.core.IBridgeContext
import com.tiktok.sparkling.method.registry.core.SparklingBridgeManager
import com.tiktok.sparkling.method.registry.core.model.context.ContextProviderFactory
import com.tiktok.sparkling.method.router.close.RouterCloseMethod
import com.tiktok.sparkling.method.router.open.RouterOpenMethod
import com.tiktok.sparkling.method.router.utils.AbsRouteOpenHandler
import com.tiktok.sparkling.method.router.utils.IHostRouterDepend
import com.tiktok.sparkling.method.router.utils.RouterProvider

object SparklingNavigationRegistrar {
  @JvmStatic
  fun install(application: Application) {
    SparklingBridgeManager.registerIDLMethod(RouterOpenMethod::class.java, BridgePlatformType.LYNX)
    SparklingBridgeManager.registerIDLMethod(RouterCloseMethod::class.java, BridgePlatformType.LYNX)
    RouterProvider.hostRouterDepend = ExplorerRouterDepend(application)
    GlobalPropsUtils.Companion.instance.setStableProps(
        mapOf(
            "sparklingAvailable" to true,
            "sparklingNavigation" to true,
        ))
  }

  @JvmStatic
  fun isRouterOpenRegistered(): Boolean {
    return SparklingBridgeManager.findIDLMethodClass(
        BridgePlatformType.LYNX, "router.open") != null
  }

  private class ExplorerRouterDepend(private val application: Application) : IHostRouterDepend {
    override fun openScheme(
        bridgeContext: IBridgeContext?,
        scheme: String,
        extraParams: Map<String, Any>,
        platformType: BridgePlatformType,
        context: Context?): Boolean {
      if (scheme.isBlank()) {
        return false
      }
      val sparklingContext = SparklingContext()
      sparklingContext.scheme = scheme
      val launchContext = context ?: bridgeContext?.context ?: application
      return Sparkling.build(launchContext.applicationContext, sparklingContext).navigate()
    }

    override fun closeView(
        bridgeContext: IBridgeContext?,
        type: BridgePlatformType,
        containerID: String?,
        animated: Boolean?): Boolean {
      val activity = bridgeContext?.ownerActivity ?: return false
      activity.runOnUiThread {
        if (!activity.isFinishing) {
          activity.finish()
        }
      }
      return true
    }

    override fun provideRouteOpenHandlerList(
        contextProviderFactory: ContextProviderFactory?): List<AbsRouteOpenHandler> {
      return emptyList()
    }

    override fun provideRouteOpenExceptionHandler(
        contextProviderFactory: ContextProviderFactory?): AbsRouteOpenHandler? {
      return null
    }
  }
}
