// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.video

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.view.View
import androidx.media3.common.AudioAttributes
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.exoplayer.DefaultRenderersFactory
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.ui.AspectRatioFrameLayout
import androidx.media3.ui.PlayerView

class LynxVideoView(context: Context) : LynxVideoPlayable {

    private val playerView: PlayerView = PlayerView(context)
    private val mainHandler = Handler(Looper.getMainLooper())
    private val player: ExoPlayer
    @Volatile
    private var callback: LynxVideoPlayable.Callback? = null
    private var loop: Boolean = false
    private var volume: Float = 1.0f
    private var muted: Boolean = false
    private var speed: Float = 1.0f
    private var objectFit: String? = "contain"
    private var currentSrc: String? = null
    private var hasFiredFirstFrame: Boolean = false
    private var isExplicitlyStopped: Boolean = false
    private var isBuffering: Boolean = false
    private var suppressNextPlayingForLoop: Boolean = false
    private var suppressNextPaused: Boolean = false
    @Volatile
    private var isReleased: Boolean = false
    @Volatile
    private var cachedDurationMs: Long = 0
    @Volatile
    private var cachedCurrentPositionMs: Long = 0
    @Volatile
    private var cachedIsPlaying: Boolean = false
    private val playerListener: Player.Listener by lazy { createPlayerListener() }

    private fun isOnMainThread(): Boolean = Looper.myLooper() == mainHandler.looper

    private fun runOnMainThread(allowAfterRelease: Boolean = false, action: () -> Unit) {
        if (!allowAfterRelease && isReleased) {
            return
        }
        if (isOnMainThread()) {
            if (allowAfterRelease || !isReleased) {
                action()
            }
            return
        }
        mainHandler.post {
            if (allowAfterRelease || !isReleased) {
                action()
            }
        }
    }

    private fun notifyCallback(action: LynxVideoPlayable.Callback.() -> Unit) {
        runOnMainThread {
            callback?.action()
        }
    }

    private fun refreshPlaybackSnapshot() {
        cachedDurationMs = player.duration.coerceAtLeast(0)
        cachedCurrentPositionMs = player.currentPosition.coerceAtLeast(0)
        cachedIsPlaying = player.isPlaying && player.playbackState != Player.STATE_ENDED
    }

    private fun <T> readPlayerState(cachedValue: T, read: () -> T): T =
        if (!isReleased && isOnMainThread()) {
            read()
        } else {
            cachedValue
        }

    private fun createPlayerListener(): Player.Listener = object : Player.Listener {
        override fun onPlaybackStateChanged(playbackState: Int) {
            if (isReleased) {
                return
            }
            refreshPlaybackSnapshot()
            when (playbackState) {
                Player.STATE_READY -> {
                    if (isBuffering) {
                        isBuffering = false
                    }
                }
                Player.STATE_ENDED -> {
                    if (loop) {
                        notifyCallback { onLooped() }
                        suppressNextPlayingForLoop = true
                        suppressNextPaused = true
                        player.seekTo(0)
                        player.play()
                        refreshPlaybackSnapshot()
                    } else {
                        notifyCallback { onEnded() }
                    }
                }
                Player.STATE_BUFFERING -> {
                    isBuffering = true
                    val bufferedMs = if (player.bufferedPosition == C.TIME_UNSET) {
                        player.currentPosition
                    } else {
                        player.bufferedPosition
                    }
                    notifyCallback { onBuffering(bufferedMs.coerceAtLeast(0)) }
                }
            }
        }

        override fun onIsPlayingChanged(isPlaying: Boolean) {
            if (isReleased) {
                return
            }
            refreshPlaybackSnapshot()
            if (isPlaying) {
                if (suppressNextPlayingForLoop) {
                    suppressNextPlayingForLoop = false
                    return
                }
                notifyCallback { onPlaying() }
            } else {
                if (suppressNextPaused) {
                    suppressNextPaused = false
                } else if (!isExplicitlyStopped && player.playbackState != Player.STATE_ENDED) {
                    notifyCallback { onPaused() }
                }
            }
        }

        override fun onPlayerError(error: PlaybackException) {
            if (isReleased) {
                return
            }
            val errorMsg = error.message ?: "Unknown playback error"
            notifyCallback { onError(error.errorCode, errorMsg) }
        }

        override fun onPositionDiscontinuity(
            oldPosition: Player.PositionInfo,
            newPosition: Player.PositionInfo,
            reason: Int
        ) {
            if (isReleased) {
                return
            }
            if (reason == Player.DISCONTINUITY_REASON_SEEK) {
                // Seek completed
            }
        }

        override fun onRenderedFirstFrame() {
            if (isReleased) {
                return
            }
            refreshPlaybackSnapshot()
            if (!hasFiredFirstFrame) {
                hasFiredFirstFrame = true
                val durationMs = cachedDurationMs
                notifyCallback { onFirstFrame(durationMs) }
            }
        }
    }

    init {
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(C.USAGE_MEDIA)
            .setContentType(C.AUDIO_CONTENT_TYPE_MOVIE)
            .build()

        val renderersFactory = DefaultRenderersFactory(context)
            .setExtensionRendererMode(DefaultRenderersFactory.EXTENSION_RENDERER_MODE_OFF)

        player = ExoPlayer.Builder(context, renderersFactory)
            .setLooper(mainHandler.looper)
            .setAudioAttributes(audioAttributes, true)
            .build()

        runOnMainThread {
            playerView.player = player
            playerView.useController = false
            applyObjectFit()
            applyVolume()
            applySpeed()
            player.addListener(playerListener)
            refreshPlaybackSnapshot()
        }
    }

    fun getPlayerView(): View = playerView

    override fun setSrc(src: String?) {
        runOnMainThread {
            currentSrc = src
            hasFiredFirstFrame = false
            isExplicitlyStopped = false
            suppressNextPlayingForLoop = false
            suppressNextPaused = suppressNextPaused || player.isPlaying
            player.pause()
            player.stop()
            player.clearMediaItems()
            cachedCurrentPositionMs = 0
            if (src.isNullOrEmpty()) {
                refreshPlaybackSnapshot()
                return@runOnMainThread
            }
            val mediaItem = MediaItem.fromUri(src)
            player.setMediaItem(mediaItem)
            player.prepare()
            refreshPlaybackSnapshot()
        }
    }

    override fun setLoop(loop: Boolean) {
        runOnMainThread {
            this.loop = loop
            // Always use REPEAT_MODE_OFF and handle looping manually in onPlaybackStateChanged
            // so that STATE_ENDED fires and we can dispatch onEnded / onLooped events.
            player.repeatMode = Player.REPEAT_MODE_OFF
        }
    }

    override fun setVolume(volume: Float) {
        runOnMainThread {
            this.volume = volume.coerceIn(0f, 1f)
            applyVolume()
        }
    }

    override fun setMuted(muted: Boolean) {
        runOnMainThread {
            this.muted = muted
            applyVolume()
        }
    }

    private fun applyVolume() {
        val effectiveVolume = if (muted) 0f else volume
        player.volume = effectiveVolume
    }

    override fun setSpeed(speed: Float) {
        runOnMainThread {
            this.speed = speed.coerceIn(0.1f, 2.0f)
            applySpeed()
        }
    }

    private fun applySpeed() {
        player.setPlaybackSpeed(speed)
    }

    override fun setObjectFit(objectFit: String?) {
        runOnMainThread {
            this.objectFit = objectFit
            applyObjectFit()
        }
    }

    private fun applyObjectFit() {
        playerView.resizeMode = when (objectFit) {
            "cover" -> AspectRatioFrameLayout.RESIZE_MODE_ZOOM
            "fill" -> AspectRatioFrameLayout.RESIZE_MODE_FILL
            else -> AspectRatioFrameLayout.RESIZE_MODE_FIT
        }
    }

    override fun play() {
        runOnMainThread {
            isExplicitlyStopped = false
            val src = currentSrc
            if (player.mediaItemCount == 0 && !src.isNullOrEmpty()) {
                val mediaItem = MediaItem.fromUri(src)
                player.setMediaItem(mediaItem)
                player.prepare()
            } else if (player.playbackState == Player.STATE_ENDED) {
                player.seekTo(0)
            }
            player.play()
            refreshPlaybackSnapshot()
        }
    }

    override fun pause() {
        runOnMainThread {
            player.pause()
            refreshPlaybackSnapshot()
        }
    }

    override fun stop() {
        runOnMainThread {
            isExplicitlyStopped = true
            suppressNextPaused = suppressNextPaused || player.isPlaying
            player.stop()
            player.clearMediaItems()
            cachedCurrentPositionMs = 0
            refreshPlaybackSnapshot()
            notifyCallback { onStopped() }
        }
    }

    override fun seek(positionMs: Long) {
        runOnMainThread {
            val targetPositionMs = positionMs.coerceAtLeast(0)
            player.seekTo(targetPositionMs)
            cachedCurrentPositionMs = targetPositionMs
        }
    }

    override fun getDuration(): Long =
        readPlayerState(cachedDurationMs) {
            player.duration.coerceAtLeast(0).also { cachedDurationMs = it }
        }

    override fun getCurrentPosition(): Long =
        readPlayerState(cachedCurrentPositionMs) {
            player.currentPosition.coerceAtLeast(0).also { cachedCurrentPositionMs = it }
        }

    override fun isPlaying(): Boolean =
        readPlayerState(cachedIsPlaying) {
            (player.isPlaying && player.playbackState != Player.STATE_ENDED).also {
                cachedIsPlaying = it
            }
        }

    override fun setCallback(callback: LynxVideoPlayable.Callback?) {
        if (callback == null) {
            this.callback = null
        }
        runOnMainThread {
            this.callback = callback
        }
    }

    @Synchronized
    override fun release() {
        if (isReleased) {
            return
        }
        isReleased = true
        callback = null
        runOnMainThread(allowAfterRelease = true) {
            player.removeListener(playerListener)
            playerView.player = null
            player.clearVideoSurface()
            player.stop()
            player.clearMediaItems()
            player.release()
        }
    }
}
