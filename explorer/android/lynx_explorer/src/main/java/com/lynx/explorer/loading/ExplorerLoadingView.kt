// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.loading

import android.content.Context
import android.graphics.Color
import android.view.Gravity
import android.view.View
import android.widget.Button
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView

/** Shared loading/error surface used by both Explorer runtime launchers. */
class ExplorerLoadingView(context: Context) : LinearLayout(context) {
  enum class State { DOWNLOADING, RENDERING, ERROR }
  var state: State = State.DOWNLOADING
    private set
  private val progress = ProgressBar(context)
  private val title = TextView(context)
  private val detail = TextView(context)
  private val retry = Button(context).apply { text = "Retry"; visibility = View.GONE }

  init {
    orientation = VERTICAL
    gravity = Gravity.CENTER
    setPadding(48, 48, 48, 48)
    addView(progress)
    addView(title)
    addView(detail)
    addView(retry)
    applyAppearance()
  }

  fun showDownloading(runtime: String) { state = State.DOWNLOADING; progress.visibility = View.VISIBLE; retry.visibility = View.GONE; title.text = "Downloading bundle"; detail.text = runtime }
  fun showRendering(runtime: String) { state = State.RENDERING; progress.visibility = View.VISIBLE; retry.visibility = View.GONE; title.text = "Rendering"; detail.text = runtime }
  fun showError(message: String, action: (() -> Unit)?) { state = State.ERROR; progress.visibility = View.GONE; title.text = "Unable to open page"; detail.text = message; retry.visibility = if (action == null) View.GONE else View.VISIBLE; retry.setOnClickListener { action?.invoke() } }

  private fun applyAppearance() {
    val dark = (resources.configuration.uiMode and 0x30) == 0x20
    setBackgroundColor(if (dark) Color.rgb(18, 18, 18) else Color.WHITE)
    val foreground = if (dark) Color.WHITE else Color.rgb(24, 24, 24)
    title.setTextColor(foreground); detail.setTextColor(foreground)
  }
}
