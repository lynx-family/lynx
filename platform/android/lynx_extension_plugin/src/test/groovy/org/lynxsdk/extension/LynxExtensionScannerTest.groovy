// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import org.gradle.api.GradleException
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

import static org.junit.Assert.assertEquals
import static org.junit.Assert.assertTrue

class LynxExtensionScannerTest {
    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder()

    @Test
    void scansScopedAndUnscopedPackages() {
        File root = temporaryFolder.newFolder('app')
        writeExtension(root, 'plain-ext', 'com.example.plain', null)
        writeExtension(root, '@scope/scoped-ext', 'com.example.scoped', 'native/android')

        List<LynxExtensionInfo> extensions = LynxExtensionScanner.scan(new File(root, 'android'))

        assertEquals(2, extensions.size())
        assertEquals(['@scope/scoped-ext', 'plain-ext'], extensions.collect { it.npmName })
        assertEquals(':lynx_extension__scope_scoped_ext', extensions[0].projectPath)
        assertEquals('com.example.plain.LynxExtensionProviderImpl',
            extensions[1].providerClassName)
    }

    @Test
    void rejectsMalformedAndroidManifest() {
        File root = temporaryFolder.newFolder('app')
        File packageDir = new File(root, 'node_modules/bad-ext')
        packageDir.mkdirs()
        new File(packageDir, 'lynx.ext.json').text =
            '{"platforms":{"android":{"sourceDir":"android"}}}'
        new File(packageDir, 'android').mkdirs()
        new File(packageDir, 'android/build.gradle').text = ''

        try {
            LynxExtensionScanner.scan(root)
            assertTrue('Expected GradleException', false)
        } catch (GradleException e) {
            assertTrue(e.message.contains('platforms.android.packageName'))
        }
    }

    @Test
    void fallsBackToAbsoluteFileWhenCanonicalFileFails() {
        File file = new BrokenCanonicalFile(new File(temporaryFolder.root, 'node_modules').path)

        assertEquals(file.absoluteFile, LynxExtensionScanner.canonicalOrAbsolute(file))
    }

    private static void writeExtension(
        File root, String npmName, String packageName, String sourceDir) {
        File packageDir = new File(root, "node_modules/${npmName}")
        packageDir.mkdirs()
        String androidSourceDir = sourceDir ?: 'android'
        new File(packageDir, androidSourceDir).mkdirs()
        new File(packageDir, "${androidSourceDir}/build.gradle").text = ''
        String sourceEntry = sourceDir == null ? '' : ",\"sourceDir\":\"${sourceDir}\""
        new File(packageDir, 'lynx.ext.json').text =
            "{\"platforms\":{\"android\":{\"packageName\":\"${packageName}\"${sourceEntry}}}}"
    }

    private static class BrokenCanonicalFile extends File {
        BrokenCanonicalFile(String pathname) {
            super(pathname)
        }

        @Override
        File getCanonicalFile() throws IOException {
            throw new IOException('canonical unavailable')
        }
    }
}
