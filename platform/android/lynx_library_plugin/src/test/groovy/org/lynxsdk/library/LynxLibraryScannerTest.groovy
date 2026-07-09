// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.GradleException
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

import static org.junit.Assert.assertEquals
import static org.junit.Assert.assertTrue

class LynxLibraryScannerTest {
    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder()

    @Test
    void scansScopedAndUnscopedPackages() {
        File root = temporaryFolder.newFolder('app')
        writeLibrary(root, 'plain-lib', 'com.example.plain', null)
        writeLibrary(root, '@scope/scoped-lib', 'com.example.scoped', 'native/android')

        List<LynxLibraryInfo> libraries = LynxLibraryScanner.scan(new File(root, 'android'))

        assertEquals(2, libraries.size())
        assertEquals(['@scope/scoped-lib', 'plain-lib'], libraries.collect { it.npmName })
        assertEquals(':lynx_library__scope_scoped_lib', libraries[0].projectPath)
        assertEquals('com.example.plain.LynxLibraryProviderImpl',
            libraries[1].providerClassName)
    }

    @Test
    void rejectsMalformedAndroidManifest() {
        File root = temporaryFolder.newFolder('app')
        File packageDir = new File(root, 'node_modules/bad-lib')
        packageDir.mkdirs()
        new File(packageDir, 'lynx.lib.json').text =
            '{"platforms":{"android":{"sourceDir":"android"}}}'
        new File(packageDir, 'android').mkdirs()
        new File(packageDir, 'android/build.gradle').text = ''

        try {
            LynxLibraryScanner.scan(root)
            assertTrue('Expected GradleException', false)
        } catch (GradleException e) {
            assertTrue(e.message.contains('platforms.android.packageName'))
        }
    }

    @Test
    void parsesNodeApiAddons() {
        File root = temporaryFolder.newFolder('app')
        File packageDir = writeLibrary(root, 'napi-lib', 'com.example.napi', null)
        File jniLibsDir = new File(packageDir, 'napi_addons')
        new File(jniLibsDir, 'arm64-v8a').mkdirs()
        String libraryName = 'demo_addon'
        new File(jniLibsDir, "arm64-v8a/lib${libraryName}.so").text = ''
        new File(packageDir, 'lynx.lib.json').text = '''{
          "platforms": {
            "android": {
              "packageName": "com.example.napi",
              "nodeApiAddons": [{
                "name": "demo_addon",
                "libraryName": "demo_addon",
                "jniLibsDir": "napi_addons",
                "required": false
              }]
            }
          }
        }'''

        List<LynxLibraryInfo> libraries = LynxLibraryScanner.scan(root)

        assertEquals(1, libraries.size())
        assertEquals(1, libraries.first().nodeApiAddons.size())
        LynxNodeApiAddonInfo addon = libraries.first().nodeApiAddons.first()
        assertEquals('demo_addon', addon.name)
        assertEquals('demo_addon', addon.libraryName)
        assertEquals("lib${libraryName}.so".toString(), addon.sharedLibraryName)
        assertEquals(LynxLibraryScanner.canonicalOrAbsolute(jniLibsDir), addon.jniLibsDir)
        assertTrue(!addon.required)
    }

    @Test
    void rejectsInvalidNodeApiAddonName() {
        File root = temporaryFolder.newFolder('app')
        File packageDir = writeLibrary(root, 'bad-napi-lib', 'com.example.invalid.name', null)
        new File(packageDir, 'lynx.lib.json').text = '''{
          "platforms": {
            "android": {
              "packageName": "com.example.invalid.name",
              "nodeApiAddons": [{"name": "@scope/bad"}]
            }
          }
        }'''

        try {
            LynxLibraryScanner.scan(root)
            assertTrue('Expected GradleException', false)
        } catch (GradleException e) {
            assertTrue(e.message.contains('nodeApiAddons[0].name'))
        }
    }

    @Test
    void rejectsNodeApiAddonPathOutsidePackage() {
        File root = temporaryFolder.newFolder('app')
        File packageDir = writeLibrary(root, 'bad-path-lib', 'com.example.invalid.path', null)
        new File(packageDir, 'lynx.lib.json').text = '''{
          "platforms": {
            "android": {
              "packageName": "com.example.invalid.path",
              "nodeApiAddons": [{"name": "demo", "jniLibsDir": "../shared"}]
            }
          }
        }'''

        try {
            LynxLibraryScanner.scan(root)
            assertTrue('Expected GradleException', false)
        } catch (GradleException e) {
            assertTrue(e.message.contains('must stay within package directory'))
        }
    }

    @Test
    void fallsBackToAbsoluteFileWhenCanonicalFileFails() {
        File file = new BrokenCanonicalFile(new File(temporaryFolder.root, 'node_modules').path)

        assertEquals(file.absoluteFile, LynxLibraryScanner.canonicalOrAbsolute(file))
    }

    private static File writeLibrary(
        File root, String npmName, String packageName, String sourceDir) {
        File packageDir = new File(root, "node_modules/${npmName}")
        packageDir.mkdirs()
        String androidSourceDir = sourceDir ?: 'android'
        new File(packageDir, androidSourceDir).mkdirs()
        new File(packageDir, "${androidSourceDir}/build.gradle").text = ''
        String sourceEntry = sourceDir == null ? '' : ",\"sourceDir\":\"${sourceDir}\""
        new File(packageDir, 'lynx.lib.json').text =
            "{\"platforms\":{\"android\":{\"packageName\":\"${packageName}\"${sourceEntry}}}}"
        packageDir
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
