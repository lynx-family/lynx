// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.View
import android.view.ViewGroup
import androidx.appcompat.widget.Toolbar
import com.lynx.explorer.BuildConfig
import com.lynx.explorer.ImageBehavior
import com.lynx.explorer.loading.ExplorerLoadingView
import com.lynx.explorer.modules.ExplorerModule
import com.lynx.explorer.provider.ExplorerTemplateProvider
import com.lynx.explorer.routing.LaunchDescriptor
import com.lynx.explorer.routing.RouteException
import com.lynx.explorer.routing.RouteResult
import com.lynx.explorer.routing.SparklingSchemeAdapter
import com.lynx.tasm.LynxEnv
import com.lynx.tasm.LynxViewClient
import com.lynx.xelement.XElementBehaviors
import com.tiktok.sparkling.R as SparklingR
import com.tiktok.sparkling.Sparkling
import com.tiktok.sparkling.SparklingActivity
import com.tiktok.sparkling.SparklingContext
import com.tiktok.sparkling.SparklingUIProvider
import com.tiktok.sparkling.hybridkit.HybridKit
import com.tiktok.sparkling.hybridkit.config.BaseInfoConfig
import com.tiktok.sparkling.hybridkit.config.SparklingHybridConfig
import com.tiktok.sparkling.hybridkit.config.SparklingLynxConfig
import com.tiktok.sparkling.hybridkit.lynx.SparklingLynxModuleWrapper
import com.tiktok.sparkling.hybridkit.lynx.SimpleLynxKitView
import com.tiktok.sparkling.hybridkit.utils.GlobalPropsUtils
import com.tiktok.sparkling.utils.SchemeParser
import java.lang.ref.WeakReference
import java.util.WeakHashMap

object RuntimeExtensionProvider : SparklingRuntimeExtension {
  override val supported = true

  override fun initialize(application: Application) {
    HybridKit.init(application)
    val sparklingBehaviors = ImageBehavior().create() + XElementBehaviors().create()
    // Explorer owns the first LynxEnv.init call. Sparkling's later init is intentionally
    // idempotent, so register its global behaviors through LynxEnv's post-init API as well.
    LynxEnv.inst().addBehaviors(sparklingBehaviors)
    val lynxBuilder = SparklingLynxConfig.Builder(application)
    lynxBuilder.setTemplateProvider(ExplorerTemplateProvider(application))
    lynxBuilder.addBehaviors(sparklingBehaviors)
    lynxBuilder.addLynxModules(mapOf("ExplorerModule" to SparklingLynxModuleWrapper(ExplorerModule::class.java, null)))
    val hybridBuilder = SparklingHybridConfig.Builder(BaseInfoConfig(BuildConfig.DEBUG))
    hybridBuilder.setLynxConfig(lynxBuilder.build())
    HybridKit.setHybridConfig(hybridBuilder.build(), application)
    HybridKit.initLynxKit()
    GlobalPropsUtils.instance.setStableProps(mapOf(
      "sparklingAvailable" to true,
      "sparklingNavigation" to true,
      "explorerSupportsExplicitRouteOwnership" to true,
      "explorerSupportsSparklingContainer" to true
    ))
    SparklingNavigationRegistrar.install(application)
  }

  override fun open(context: Context, descriptor: LaunchDescriptor): RouteResult {
    return try {
      val canonicalScheme = SparklingSchemeAdapter.canonicalScheme(descriptor)
      val officialParameters = SchemeParser.parseDefaultScheme(canonicalScheme)
      if (officialParameters?.bundle.isNullOrBlank()) {
        return RouteResult.failure("malformed_sparkling_scheme", "The Sparkling scheme has no resolvable bundle.")
      }
      val sparklingContext = SparklingContext().apply {
        scheme = canonicalScheme
        extra = descriptor.extras
          .filterKeys { it !in RESERVED_RUNTIME_PROPS }
          .plus("preferredTheme" to ExplorerAppearance.read(context))
        sparklingUIProvider = ExplorerUIProvider()
      }
      GlobalPropsUtils.instance.setUnstableProps(
        sparklingContext.containerId,
        mapOf(
          "preferredTheme" to ExplorerAppearance.read(context),
          "explorerPreferredContainer" to ExplorerRuntimePreferences.read(context),
          "explorerSparklingVersion" to BuildConfig.SPARKLING_VERSION
        )
      )
      if (Sparkling.build(context.applicationContext, sparklingContext).navigate()) RouteResult.accepted()
      else RouteResult.failure("sparkling_launch_failed", "Sparkling rejected the route.")
    } catch (error: RouteException) {
      RouteResult.failure(error.code, error.message)
    } catch (error: Exception) {
      RouteResult.failure("sparkling_launch_failed", error.message ?: "Sparkling launch failed.")
    }
  }

  private class ExplorerUIProvider : SparklingUIProvider {
    override fun getLoadingView(context: Context): View {
      val activity = context as? SparklingActivity
        ?: throw IllegalArgumentException("Sparkling loading UI requires SparklingActivity.")
      SparklingLoadSuccessBridge.record(activity)
      return ExplorerLoadingView(context).apply { showDownloading("Sparkling") }
    }
    override fun getErrorView(context: Context): View = ExplorerLoadingView(context).apply { showError("Unable to load Sparkling page", null) }
    override fun getToolBar(context: Context): Toolbar {
      val activity = context as? Activity
        ?: throw IllegalArgumentException("Sparkling toolbar requires an Activity context.")
      if (activity !is SparklingActivity) {
        throw IllegalArgumentException(
          "Sparkling toolbar requires SparklingActivity, received ${activity::class.java.name}."
        )
      }
      SparklingLoadSuccessBridge.record(activity)
      return activity.findViewById<Toolbar>(SparklingR.id.toolbar)
        ?: throw IllegalStateException(
          "SparklingActivity layout is missing the public Sparkling toolbar view."
        )
    }
  }

  /**
   * Sparkling 2.1.0-rc.12 completes its loading UI through
   * SimpleLynxKitView.onLoadSuccess(). Newer Lynx revisions report success only to
   * LynxViewClient, so bridge that callback inside the enabled flavor until the SDK
   * moves to the client API itself.
   */
  private object SparklingLoadSuccessBridge {
    private var activity = WeakReference<SparklingActivity>(null)
    private val installedViews = WeakHashMap<SimpleLynxKitView, Boolean>()
    private val mainHandler = Handler(Looper.getMainLooper())

    fun record(current: SparklingActivity) {
      activity = WeakReference(current)
      scheduleInstall(BRIDGE_ATTACH_ATTEMPTS)
    }

    fun install(): Boolean {
      val decorView = activity.get()?.window?.decorView ?: return false
      val kitView = findKitView(decorView) ?: return false
      if (!kitView.isAttachedToWindow || !kitView.isLaidOut || !kitView.hasWindowFocus() ||
        !kitView.isShown || kitView.width == 0 || kitView.height == 0) {
        return false
      }
      synchronized(installedViews) {
        if (installedViews.put(kitView, true) == null) {
          kitView.addLynxViewClient(object : LynxViewClient() {
            override fun onLoadSuccess() {
              kitView.onLoadSuccess()
            }
          })
        }
      }
      return true
    }

    private fun scheduleInstall(attemptsLeft: Int) {
      mainHandler.postDelayed({
        if (!install() && attemptsLeft > 0) scheduleInstall(attemptsLeft - 1)
      }, BRIDGE_ATTACH_RETRY_DELAY_MS)
    }

    private fun findKitView(view: View): SimpleLynxKitView? {
      if (view is SimpleLynxKitView) return view
      if (view !is ViewGroup) return null
      for (index in 0 until view.childCount) {
        findKitView(view.getChildAt(index))?.let { return it }
      }
      return null
    }
  }

  private val RESERVED_RUNTIME_PROPS = setOf(
    "sparklingAvailable", "sparklingNavigation", "explorerSupportsExplicitRouteOwnership",
    "explorerSupportsSparklingContainer", "explorerPreferredContainer",
    "explorerSparklingVersion", "containerID", "spkPipe")

  private const val BRIDGE_ATTACH_ATTEMPTS = 60
  private const val BRIDGE_ATTACH_RETRY_DELAY_MS = 16L
}
