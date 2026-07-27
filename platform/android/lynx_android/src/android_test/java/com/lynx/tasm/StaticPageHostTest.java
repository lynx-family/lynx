// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Test;

public class StaticPageHostTest {
  private static final int INSTANCE_ID = 42;

  @Test
  public void renderKeepsFirstDataByReference() throws Exception {
    List<Runnable> tasks = new ArrayList<>();
    StaticPageHost host = new StaticPageHost(tasks::add);
    HashMap<String, Object> initialData = new HashMap<>();
    initialData.put("title", "initial");
    RecordingPageInstance instance = new RecordingPageInstance();

    host.register(INSTANCE_ID, staticPageData(initialData), null, null, null);
    assertTrue(StaticPageHost.attach(INSTANCE_ID, instance));
    assertTrue(renderPageForNative(INSTANCE_ID));

    assertSame(initialData, instance.renderData);
    assertNull(instance.renderGlobalProps);
    host.clear();
    assertEquals(1, tasks.size());
    tasks.remove(0).run();
    assertTrue(instance.destroyed);
    assertFalse(StaticPageHost.attach(INSTANCE_ID, new RecordingPageInstance()));
  }

  @Test
  public void metadataMergesRootAndDispatchesOnOwnerExecutor() throws Exception {
    List<Runnable> tasks = new ArrayList<>();
    StaticPageHost host = new StaticPageHost(tasks::add);
    HashMap<String, Object> beforeLoad = new HashMap<>();
    beforeLoad.put("before", 1);
    beforeLoad.put("shared", "updated");
    HashMap<String, Object> initialData = new HashMap<>();
    initialData.put("initial", 2);
    initialData.put("shared", "initial");
    HashMap<String, Object> initialGlobalProps = new HashMap<>();
    initialGlobalProps.put("theme", "light");
    HashMap<String, Object> updatedGlobalPropsBeforeLoad = new HashMap<>();
    updatedGlobalPropsBeforeLoad.put("theme", "dark");
    HashMap<String, Object> mergedGlobalPropsBeforeLoad =
        new HashMap<>(updatedGlobalPropsBeforeLoad);
    mergedGlobalPropsBeforeLoad.put("locale", "zh");
    RecordingPageInstance instance = new RecordingPageInstance();

    host.updateMetaData(staticPageData(beforeLoad), staticPageData(updatedGlobalPropsBeforeLoad));
    host.updateMetaData(null, staticPageData(mergedGlobalPropsBeforeLoad));
    host.register(
        INSTANCE_ID, staticPageData(initialData), staticPageData(initialGlobalProps), null, null);
    assertTrue(StaticPageHost.attach(INSTANCE_ID, instance));
    assertTrue(renderPageForNative(INSTANCE_ID));
    assertEquals(1, instance.renderData.get("before"));
    assertEquals(2, instance.renderData.get("initial"));
    assertEquals("updated", instance.renderData.get("shared"));
    assertEquals("dark", instance.renderGlobalProps.get("theme"));
    assertEquals("zh", instance.renderGlobalProps.get("locale"));

    HashMap<String, Object> update = new HashMap<>();
    update.put("after", 3);
    HashMap<String, Object> globalPropsUpdate = new HashMap<>();
    globalPropsUpdate.put("locale", "en");
    Map<String, Object> mergedGlobalProps =
        host.updateMetaData(staticPageData(update), staticPageData(globalPropsUpdate));

    assertEquals(1, tasks.size());
    assertNull(instance.updatedData);
    tasks.remove(0).run();
    assertEquals(1, instance.updatedData.get("before"));
    assertEquals(2, instance.updatedData.get("initial"));
    assertEquals(3, instance.updatedData.get("after"));
    assertEquals("dark", mergedGlobalProps.get("theme"));
    assertEquals("en", mergedGlobalProps.get("locale"));
    assertSame(mergedGlobalProps, instance.updatedGlobalProps);
    host.clear();
  }

  @Test
  public void registerPreservesGlobalPropsPrecedence() {
    StaticPageHost host = new StaticPageHost(Runnable::run);
    HashMap<String, Object> currentGlobalProps = new HashMap<>();
    currentGlobalProps.put("loadOrder", "current");
    HashMap<String, Object> groupGlobalProps = new HashMap<>();
    groupGlobalProps.put("loadOrder", "group");
    HashMap<String, Object> loadGlobalProps = new HashMap<>();
    loadGlobalProps.put("loadOrder", "load");
    HashMap<String, Object> pendingGlobalProps = new HashMap<>();
    pendingGlobalProps.put("pending", "metadata");
    pendingGlobalProps.put("pendingWins", true);
    loadGlobalProps.put("pendingWins", false);

    host.updateMetaData(null, staticPageData(pendingGlobalProps));
    Map<String, Object> registeredGlobalProps =
        host.register(INSTANCE_ID, null, staticPageData(currentGlobalProps),
            staticPageData(groupGlobalProps), staticPageData(loadGlobalProps));

    assertEquals("load", registeredGlobalProps.get("loadOrder"));
    assertEquals("metadata", registeredGlobalProps.get("pending"));
    assertEquals(true, registeredGlobalProps.get("pendingWins"));
    host.clear();
  }

  @Test
  public void pendingUpdateIsDroppedWhenHostIsCleared() throws Exception {
    List<Runnable> tasks = new ArrayList<>();
    StaticPageHost host = new StaticPageHost(tasks::add);
    RecordingPageInstance instance = new RecordingPageInstance();
    host.register(INSTANCE_ID, staticPageData(new HashMap<>()), null, null, null);
    assertTrue(StaticPageHost.attach(INSTANCE_ID, instance));
    assertTrue(renderPageForNative(INSTANCE_ID));

    HashMap<String, Object> update = new HashMap<>();
    update.put("stale", true);
    host.updateMetaData(staticPageData(update), null);
    host.clear();

    assertEquals(2, tasks.size());
    tasks.remove(0).run();
    assertNull(instance.updatedData);
    tasks.remove(0).run();
    assertTrue(instance.destroyed);
  }

  private static boolean renderPageForNative(int instanceId) throws Exception {
    Method method = StaticPageHost.class.getDeclaredMethod("renderPageForNative", int.class);
    method.setAccessible(true);
    return (boolean) method.invoke(null, instanceId);
  }

  private static TemplateData staticPageData(Map<String, Object> data) {
    return TemplateData.createForStaticPage(data);
  }

  private static class RecordingPageInstance implements StaticPageInstance {
    Map<String, Object> renderData;
    Map<String, Object> renderGlobalProps;
    Map<String, Object> updatedData;
    Map<String, Object> updatedGlobalProps;
    boolean destroyed;

    @Override
    public void renderPage(Map<String, Object> data, Map<String, Object> globalProps) {
      renderData = data;
      renderGlobalProps = globalProps;
    }

    @Override
    public void updateMetaData(Map<String, Object> data, Map<String, Object> globalProps) {
      updatedData = data;
      updatedGlobalProps = globalProps;
    }

    @Override
    public void destroy() {
      destroyed = true;
    }
  }
}
