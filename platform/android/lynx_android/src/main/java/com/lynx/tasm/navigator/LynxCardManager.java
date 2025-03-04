// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.navigator;

import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewClient;
import com.lynx.tasm.behavior.herotransition.HeroTransitionManager;
import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;
import java.util.Stack;

public class LynxCardManager implements LynxRouteLruCache.LynxRouteCacheListener {
  private WeakReference<LynxHolder> holderRef;
  private Stack<LynxRoute> routeStack = new Stack<>();
  private WeakReference<LynxView> initLynxViewRef;
  private HashMap<String, Object> routeTable;

  private LynxRouteLruCache lruCache;

  LynxCardManager(LynxHolder holder, int capacity) {
    this.holderRef = new WeakReference<>(holder);
    this.lruCache = new LynxRouteLruCache(capacity, this);
  }

  public void registerRoute(ReadableMap routeTable) {
    this.routeTable = routeTable.asHashMap();
  }

  public void registerInitLynxView(LynxView lynxView) {
    this.initLynxViewRef = new WeakReference<>(lynxView);
  }

  public void push(final String name, Map<String, Object> param) {
    String templateUrl = getTemplateUrl(name);
    final LynxRoute route = new LynxRoute(templateUrl, param);
    buildLynxView(route, new LynxViewCreationListener() {
      @Override
      public void onReady(LynxView lynxView) {
        if (lynxView != null) {
          if (!routeStack.isEmpty()) {
            LynxRoute previousRoute = routeStack.peek();
            final LynxView previousLynxView = lruCache.get(previousRoute);
            invokeOnHide(previousLynxView);
          } else {
            invokeOnHide(getInitLynxView());
          }
          routeStack.push(route);
          LynxHolder holder = getCurrentHolder();
          if (holder != null) {
            holder.showLynxView(lynxView, name);
          }
        }
      }

      @Override
      public void onFailed() {}
    });
  }

  public void replace(final String name, Map<String, Object> param) {
    String templateUrl = getTemplateUrl(name);
    final LynxRoute route = new LynxRoute(templateUrl, param);
    buildLynxView(route, new LynxViewCreationListener() {
      @Override
      public void onReady(LynxView lynxView) {
        if (lynxView != null) {
          if (!routeStack.isEmpty()) {
            LynxRoute previousRoute = routeStack.pop();
            LynxView previousLynxView = lruCache.remove(previousRoute);
            hideLynxView(previousLynxView);
          } else {
            hideLynxView(getInitLynxView());
          }
          routeStack.push(route);
          LynxHolder holder = getCurrentHolder();
          if (holder != null) {
            holder.showLynxView(lynxView, name);
          }
        }
      }

      @Override
      public void onFailed() {}
    });
  }

  private String getTemplateUrl(String name) {
    if (routeTable != null) {
      Object route = routeTable.get(name);
      if (route instanceof String) {
        return ((String) route);
      }
    }
    return name;
  }

  public void pop() {
    if (!routeStack.isEmpty()) {
      LynxRoute route = routeStack.pop();
      LynxView lynxView = lruCache.remove(route);
      hideLynxView(lynxView);
    } else {
      LynxHolder holder = getCurrentHolder();
      if (holder != null) {
        holder.quit();
      }
    }
  }

  public boolean onBackPressed() {
    if (!routeStack.isEmpty()) {
      LynxRoute route = routeStack.pop();
      LynxView lynxView = lruCache.remove(route);
      hideLynxView(lynxView);
      return true;
    }
    return false;
  }

  private void buildLynxView(final LynxRoute route, final LynxViewCreationListener listener) {
    LynxHolder holder = getCurrentHolder();
    if (holder != null) {
      holder.createLynxView(route, new LynxViewCreationListener() {
        @Override
        public void onReady(LynxView lynxView) {
          if (lynxView != null) {
            lruCache.put(route, lynxView);
            listener.onReady(lynxView);
          }
        }

        @Override
        public void onFailed() {
          listener.onFailed();
        }
      });
    }
  }

  private void hideLynxView(final LynxView lynxView) {
    if (lynxView == null) {
      return;
    }
    HeroTransitionManager.inst().executeExitAnim(
        lynxView, new HeroTransitionManager.LynxViewExitFinishListener() {
          @Override
          public void onLynxViewExited() {
            LynxHolder holder = getCurrentHolder();
            if (holder != null) {
              holder.dismissLynxView(lynxView);
            }
            lynxView.destroy();
          }
        });
    invokeOnShow();
  }

  private void invokeOnHide(final LynxView lynxView) {
    if (lynxView == null) {
      return;
    }
    HeroTransitionManager.inst().executePauseAnim(lynxView);
    lynxView.onEnterBackground();
  }

  private void invokeOnShow() {
    if (!routeStack.isEmpty()) {
      LynxRoute route = routeStack.peek();
      final LynxView lynxView = lruCache.get(route);
      if (lynxView != null) {
        if (lynxView.getParent() == null) {
          LynxHolder holder = getCurrentHolder();
          if (holder != null) {
            holder.showLynxView(lynxView, route.getRouteName());
            lynxView.addLynxViewClient(new LynxViewClient() {
              @Override
              public void onLoadSuccess() {
                HeroTransitionManager.inst().executeResumeAnim(lynxView);
                lynxView.onEnterForeground();
              }
            });
          }
        } else {
          HeroTransitionManager.inst().executeResumeAnim(lynxView);
          lynxView.onEnterForeground();
        }
      }
    } else {
      LynxView initLynxView = getInitLynxView();
      if (initLynxView != null) {
        HeroTransitionManager.inst().executeResumeAnim(initLynxView);
        initLynxView.onEnterForeground();
      }
    }
  }

  public void onEnterForeground() {
    if (!routeStack.isEmpty()) {
      LynxRoute route = routeStack.peek();
      LynxView lynxView = lruCache.get(route);
      if (lynxView != null) {
        lynxView.onEnterForeground();
      }
    } else {
      LynxView initLynxView = getInitLynxView();
      if (initLynxView != null) {
        initLynxView.onEnterForeground();
      }
    }
  }

  public void onEnterBackground() {
    if (!routeStack.isEmpty()) {
      LynxRoute route = routeStack.peek();
      LynxView lynxView = lruCache.get(route);
      if (lynxView != null) {
        lynxView.onEnterBackground();
      }
    } else {
      LynxView initLynxView = getInitLynxView();
      if (initLynxView != null) {
        initLynxView.onEnterBackground();
      }
    }
  }

  @Override
  public void onLynxViewEvicted(LynxView view) {
    LynxHolder holder = getCurrentHolder();
    if (holder != null) {
      holder.dismissLynxView(view);
    }
    view.destroy();
  }

  private LynxHolder getCurrentHolder() {
    if (holderRef != null) {
      return holderRef.get();
    }
    return null;
  }

  private LynxView getInitLynxView() {
    if (initLynxViewRef != null) {
      return initLynxViewRef.get();
    }
    return null;
  }

  @Override
  public void onLynxViewRecreated(LynxRoute key, LynxViewCreationListener listener) {
    buildLynxView(key, listener);
  }

  public void onDestroy() {
    for (LynxRoute route : this.routeStack) {
      this.lruCache.remove(route);
    }
    this.routeStack.clear();
    if (routeTable != null) {
      this.routeTable.clear();
    }
  }
}
