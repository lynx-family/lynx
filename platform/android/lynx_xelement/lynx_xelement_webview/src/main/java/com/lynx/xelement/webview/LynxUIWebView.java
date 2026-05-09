// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.webview;

import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.view.ViewGroup;
import android.webkit.WebResourceRequest;
import android.webkit.WebView;
import androidx.annotation.Keep;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.view.UISimpleView;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.xelement.webview.service.ILynxWebViewCallback;
import com.lynx.xelement.webview.service.ILynxWebViewService;
import com.lynx.xelement.webview.service.ILynxWebViewServiceProvider;
import java.util.HashMap;

@Keep
@LynxGeneratorName(packageName = "com.lynx.xelement.webview")
@LynxBehavior(tagName = {"webview"}, isCreateAsync = false)
public class LynxUIWebView<T extends LynxWebViewContainer> extends UISimpleView<T> {
  public static final String DEFAULT = "default";

  private WebView webView;
  private ILynxWebViewService webViewService;
  private boolean urlChanged;
  private boolean htmlChanged;
  private HashMap<String, Object> webViewParams;
  private ILynxWebViewServiceProvider provider;
  private String url;
  private String html;
  private String type = DEFAULT;

  public LynxUIWebView(LynxContext context) {
    this(context, null);
  }

  public LynxUIWebView(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected T createView(Context context) {
    if (context == null) {
      return null;
    }
    return createWebViewContainer(context);
  }

  protected T createWebViewContainer(Context context) {
    return (T) new LynxWebViewContainer(context);
  }

  public ILynxWebViewServiceProvider getProvider() {
    return provider;
  }

  public void setProvider(ILynxWebViewServiceProvider provider) {
    this.provider = provider;
  }

  public String getUrl() {
    return url;
  }

  public void setUrl(String url) {
    this.url = url;
  }

  public String getHtml() {
    return html;
  }

  public void setHtml(String html) {
    this.html = html;
  }

  public String getType() {
    return type;
  }

  public void setType(String type) {
    this.type = type;
  }

  @LynxProp(name = "src")
  public void setSrc(String src) {
    url = src;
    urlChanged = true;
  }

  @LynxProp(name = "html")
  public void setHtmlString(String html) {
    this.html = html;
    htmlChanged = true;
  }

  @LynxProp(name = "params")
  public void setParams(ReadableMap params) {
    webViewParams = params == null ? null : params.asHashMap();
  }

  @LynxProp(name = "webview-type")
  public void setWebViewType(String type) {
    this.type = type;
  }

  @LynxProp(name = "enable-debug")
  public void setDebug(boolean debug) {
    if (!LynxEnv.inst().isDevtoolEnabled()) {
      if (mContext != null) {
        mContext.handleLynxError(new LynxError(LynxSubErrorCode.E_COMPONENT_CUSTOM,
            "Not allow to set enable-debug if devtool is not enabled, please enable Lynx Devtool.",
            "", LynxError.LEVEL_WARN));
      }
      return;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      WebView.setWebContentsDebuggingEnabled(debug);
    }
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    if (webViewService == null) {
      webViewService = provider == null ? null : provider.getLynxWebViewService(type, mContext);
      if (webViewService != null) {
        webViewService.setParams(webViewParams);
        webViewService.initWebView();
        webViewService.setCallback(new ILynxWebViewCallback() {
          @Override
          public void onPageFinished(WebView view, String url) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
              webViewService.evaluateJavascript("(function() {"
                      + "   window.addEventListener('message', function(event) {"
                      + "       window.LynxWebViewBridge.onMessageReceived(event.data);"
                      + "   });"
                      + "})();",
                  null);
              getLynxContext().getEventEmitter().sendCustomEvent(
                  new LynxDetailEvent(getSign(), "load"));
            }
          }

          @Override
          public void onReceivedError(
              WebView view, WebResourceRequest request, int errorCode, String errorMessage) {
            LynxDetailEvent event = new LynxDetailEvent(getSign(), "error");
            event.addDetail("errorCode", errorCode);
            event.addDetail("errorMsg", errorMessage);
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }

          @Override
          public void onMessageReceived(String message) {
            LynxDetailEvent event = new LynxDetailEvent(getSign(), "message");
            event.addDetail("msg", message);
            getLynxContext().getEventEmitter().sendCustomEvent(event);
          }
        });
        webView = webViewService.getWebView();
        if (webView != null) {
          webView.setBackgroundColor(Color.TRANSPARENT);
          mView.addView(webView,
              new ViewGroup.LayoutParams(
                  ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        }
      }
    }
    if (urlChanged || htmlChanged) {
      boolean hasUrl = url != null && !url.isEmpty();
      boolean hasHtml = html != null && !html.isEmpty();
      if (hasUrl && urlChanged) {
        mView.post(new Runnable() {
          @Override
          public void run() {
            if (webViewService != null) {
              webViewService.loadUrl(url);
            }
          }
        });
      } else if (hasHtml && !hasUrl) {
        mView.post(new Runnable() {
          @Override
          public void run() {
            if (webViewService != null) {
              webViewService.loadHtmlString(html);
            }
          }
        });
      }
      if (!hasUrl && !hasHtml) {
        LynxDetailEvent event = new LynxDetailEvent(getSign(), "error");
        event.addDetail("errorCode", -1);
        event.addDetail("errorMsg", "invalid input: src and html are empty");
        getLynxContext().getEventEmitter().sendCustomEvent(event);
      }
      urlChanged = false;
      htmlChanged = false;
    }
  }

  @LynxUIMethod
  public void eval(ReadableMap params, Callback callback) {
    String func = params == null ? null : params.getString("func");
    if (func != null) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT && webViewService != null) {
        webViewService.evaluateJavascript(func, null);
      }
      if (callback != null) {
        callback.invoke(LynxUIMethodConstants.SUCCESS);
      }
    } else if (callback != null) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "params error");
    }
  }

  @LynxUIMethod
  public void reload(ReadableMap params, Callback callback) {
    if (webViewService != null) {
      webViewService.reload();
    }
    if (callback != null) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
    }
  }

  @Override
  public void destroy() {
    super.destroy();
    if (webViewService != null) {
      webViewService.destroy();
      webViewService = null;
    }
  }
}
