// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.junit.Test

import static org.junit.Assert.assertFalse
import static org.junit.Assert.assertTrue

class LynxLibraryRegistryGeneratorTest {
    @Test
    void generatesReflectionBasedRegistry() {
        LynxLibraryInfo library = new LynxLibraryInfo('demo', new File('/tmp/demo'),
            new File('/tmp/demo/lynx.lib.json'), 'com.demo.library', 'android',
            new File('/tmp/demo/android'), ':lynx_library_demo')

        String source = LynxLibraryRegistryGenerator.generate([library])

        assertTrue(source.contains('package com.lynx.tasm.library;'))
        assertTrue(source.contains('public final class LynxAutolinkGenerated'))
        assertTrue(source.contains('private LynxAutolinkGenerated()'))
        assertTrue(source.contains('"com.demo.library.LynxLibraryProviderImpl"'))
        assertTrue(source.contains('setupGlobal(Context context)'))
        assertTrue(source.contains('setup(LynxViewBuilder builder)'))
    }

    @Test
    void generatesNoopRegistryWhenNoLibrariesExist() {
        String source = LynxLibraryRegistryGenerator.generate([])

        assertTrue(source.contains('private static final String[] PROVIDERS'))
        assertTrue(source.contains('new String[] {'))
        assertTrue(source.contains('LynxLibraryRegistry.setupGlobal(context, PROVIDERS);'))
    }

    @Test
    void keepsGeneratedEntryInFixedPackage() {
        String source = LynxLibraryRegistryGenerator.generate([])

        assertTrue(source.contains('package com.lynx.tasm.library;'))
        assertFalse(source.contains('generated.libraries'))
    }
}
