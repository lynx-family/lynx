// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.http

import androidx.annotation.Keep
import com.lynx.jsbridge.network.HttpRequest
import com.lynx.jsbridge.network.HttpResponse
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

    override fun request(request: HttpRequest, callback: LynxHttpRequestCallback) {
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
                        it.httpBody = response.body?.bytes() ?: byteArrayOf()
                    })
                }
            }
        })
    }
}