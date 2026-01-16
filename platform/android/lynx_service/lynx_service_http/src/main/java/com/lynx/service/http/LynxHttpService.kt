// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.http

import androidx.annotation.Keep
import com.lynx.jsbridge.network.HttpRequest
import com.lynx.jsbridge.network.HttpResponse
import com.lynx.jsbridge.network.HttpStreamingDelegate
import com.lynx.react.bridge.JavaOnlyMap
import com.lynx.tasm.service.ILynxHttpService
import com.lynx.tasm.service.LynxHttpRequestCallback
import okhttp3.Call
import okhttp3.Callback
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.Headers.Companion.toHeaders
import java.io.IOException

@Keep
object LynxHttpService : ILynxHttpService {
    private const val TAG = "LynxHttpService"
    private const val CODE_FAILED_INTERNALLY = 499
    private val client = OkHttpClient()
    private const val deprecatedStreamingFlag = "useStreaming";

    private fun requestInner(request: HttpRequest, callback: LynxHttpRequestCallback, delegate: HttpStreamingDelegate?) {
        val okBody = if ("GET".equals(request.httpMethod, true)) null else request.httpBody.toRequestBody()

        val okRequest = Request.Builder()
            .url(request.url)
            .method(request.httpMethod, okBody)
            .headers(request.httpHeaders.asHashMap().mapValues{it.value.toString()}.toHeaders())
            .build()

        val httpResponse = HttpResponse().also {
            it.url = request.url
            it.statusCode = CODE_FAILED_INTERNALLY
        }

        client.newCall(okRequest).enqueue(object : Callback {
            override fun onFailure(call: Call, e: IOException) {
                callback.invoke(httpResponse.also {
                    it.statusText = e.toString()
                })
            }

            override fun onResponse(call: Call, response: Response) {
                response.use {
                    val httpHeaders = JavaOnlyMap()
                    response.headers.toMultimap().map { httpHeaders.put(it.key, it.value.joinToString(separator = ", ")) }
                    callback.invoke(httpResponse.also {
                        it.statusCode = response.code
                        it.statusText = response.message
                        it.httpHeaders = httpHeaders
                        if (delegate == null) {
                            it.httpBody = response.body?.bytes() ?: byteArrayOf()
                        } 
                    })

                    if (delegate != null) {
                        response.body?.let { body ->
                            body.byteStream().use { inputStream ->
                              val useDeprecatedStreamingConfig = request.customConfig.getBoolean(deprecatedStreamingFlag, false);  
                              if (useDeprecatedStreamingConfig) {
                                  delegate.deprecatedChunkedStreamingBody(inputStream);
                              } else {
                                  delegate.streamingBody(inputStream);
                              }
                                delegate.onEnd()
                            }
                        } ?: run {
                            delegate.onEnd()
                        }
                    }
                }
            }
        })
    }

    override fun request(request: HttpRequest, callback: LynxHttpRequestCallback) {
        requestInner(request, callback, null);
    }

    override fun requestStreaming(request: HttpRequest, callback: LynxHttpRequestCallback, delegate: HttpStreamingDelegate) {
        requestInner(request, callback, delegate);
    }
}
