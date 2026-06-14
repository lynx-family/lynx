// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.video

interface LynxVideoPlayable {
    fun setSrc(src: String?)
    fun setLoop(loop: Boolean)
    fun setVolume(volume: Float)
    fun setMuted(muted: Boolean)
    fun setSpeed(speed: Float)
    fun setObjectFit(objectFit: String?)

    fun play()
    fun pause()
    fun stop()
    fun seek(positionMs: Long)

    fun getDuration(): Long
    fun getCurrentPosition(): Long
    fun isPlaying(): Boolean

    fun setCallback(callback: Callback?)
    fun release()

    interface Callback {
        fun onFirstFrame(durationMs: Long)
        fun onPlaying()
        fun onPaused()
        fun onStopped()
        fun onTimeUpdate(currentMs: Long, durationMs: Long)
        fun onEnded()
        fun onLooped()
        fun onError(errorCode: Int, errorMsg: String)
        fun onBuffering(bufferedMs: Long)
    }
}
