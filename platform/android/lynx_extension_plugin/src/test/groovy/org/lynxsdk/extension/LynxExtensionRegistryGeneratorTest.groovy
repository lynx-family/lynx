// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import org.junit.Test

import static org.junit.Assert.assertTrue

class LynxExtensionRegistryGeneratorTest {
    @Test
    void generatesReflectionBasedRegistry() {
        LynxExtensionInfo extension = new LynxExtensionInfo('demo', new File('/tmp/demo'),
            new File('/tmp/demo/lynx.ext.json'), 'com.demo.extension', 'android',
            new File('/tmp/demo/android'), ':lynx_extension_demo')

        String source = LynxExtensionRegistryGenerator.generate('com.demo.app', [extension])

        assertTrue(source.contains('package com.demo.app.generated.extensions;'))
        assertTrue(source.contains('"com.demo.extension.LynxExtensionProviderImpl"'))
        assertTrue(source.contains('setupGlobal(Context context)'))
        assertTrue(source.contains('setup(LynxViewBuilder builder)'))
    }

    @Test
    void generatesNoopRegistryWhenNoExtensionExists() {
        String source = LynxExtensionRegistryGenerator.generate('com.demo.app', [])

        assertTrue(source.contains('private static final String[] PROVIDERS'))
        assertTrue(source.contains('new String[] {'))
        assertTrue(source.contains('LynxExtensionRegistry.setupGlobal(context, PROVIDERS);'))
    }
}
