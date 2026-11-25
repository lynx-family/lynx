// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.testkit.runner.BuildResult
import org.gradle.testkit.runner.GradleRunner
import org.gradle.testkit.runner.TaskOutcome
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder

import java.util.zip.ZipEntry
import java.util.zip.ZipFile
import java.util.zip.ZipOutputStream

import static org.junit.Assert.assertEquals
import static org.junit.Assert.assertNull
import static org.junit.Assert.assertTrue

class LynxLibraryPublishPluginTest {
    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder()

    // Injects lynx.lib.json into the AAR top-level META-INF/lynx/. The fake
    // android-library plugin stands in for AGP: it produces a bundleReleaseAar
    // task whose declared output is a stub AAR. The publish plugin finalizes into
    // an injection task that reads that task's own output (not a directory scan),
    // so the AAR ends up carrying the manifest at the top level.
    @Test
    void injectsManifestIntoAarTopLevel() {
        File projectDir = temporaryFolder.newFolder('lib')
        writeFakeAndroidLibraryPlugin(projectDir)
        new File(projectDir, 'settings.gradle').text = "rootProject.name = 'demo-lib'\n"
        new File(projectDir, 'lynx.lib.json').text =
            '{"platforms":{"android":{"packageName":"com.example.aar"}}}'
        new File(projectDir, 'build.gradle').text = '''
          plugins {
            id 'com.android.library'
            id 'org.lynxsdk.lynx.library-publish'
          }
        '''.stripIndent()

        BuildResult result = GradleRunner.create()
            .withProjectDir(projectDir)
            .withPluginClasspath()
            .withArguments('bundleReleaseAar', '--stacktrace')
            .build()

        assertEquals(TaskOutcome.SUCCESS,
            result.task(':injectLynxLibraryManifest').outcome)
        File aar = new File(projectDir, 'build/outputs/aar/demo-lib-release.aar')
        assertTrue('AAR should exist', aar.isFile())
        assertEquals('{"platforms":{"android":{"packageName":"com.example.aar"}}}',
            readEntry(aar, 'META-INF/lynx/lynx.lib.json'))
    }

    // A missing manifest is a hard error: an AAR published without it cannot be
    // discovered by the consumer's autolink plugin.
    @Test
    void failsWhenManifestMissing() {
        File projectDir = temporaryFolder.newFolder('lib')
        writeFakeAndroidLibraryPlugin(projectDir)
        new File(projectDir, 'settings.gradle').text = "rootProject.name = 'demo-lib'\n"
        new File(projectDir, 'build.gradle').text = '''
          plugins {
            id 'com.android.library'
            id 'org.lynxsdk.lynx.library-publish'
          }
        '''.stripIndent()

        BuildResult result = GradleRunner.create()
            .withProjectDir(projectDir)
            .withPluginClasspath()
            .withArguments('bundleReleaseAar', '--stacktrace')
            .buildAndFail()

        assertTrue(result.output.contains('lynx.lib.json not found'))
    }

    private static String readEntry(File aar, String entryName) {
        new ZipFile(aar).withCloseable { zip ->
            def entry = zip.getEntry(entryName)
            return entry == null ? null : zip.getInputStream(entry).getText('UTF-8')
        }
    }

    // Minimal stand-in for AGP: registers a bundleReleaseAar task that writes a
    // stub .aar (a real zip) into build/outputs/aar so the publish plugin's zip
    // post-process has something to inject into.
    private static void writeFakeAndroidLibraryPlugin(File projectDir) {
        File buildSrc = new File(projectDir, 'buildSrc')
        new File(buildSrc, 'src/main/groovy').mkdirs()
        new File(buildSrc, 'build.gradle').text = '''
          plugins {
            id 'groovy'
            id 'java-gradle-plugin'
          }
          dependencies {
            implementation gradleApi()
            implementation localGroovy()
          }
          gradlePlugin {
            plugins {
              fakeAndroidLibrary {
                id = 'com.android.library'
                implementationClass = 'FakeAndroidLibraryPlugin'
              }
            }
          }
        '''.stripIndent()
        new File(buildSrc, 'src/main/groovy/FakeAndroidLibraryPlugin.groovy').text = '''
          import org.gradle.api.Plugin
          import org.gradle.api.Project
          import java.util.zip.ZipEntry
          import java.util.zip.ZipOutputStream

          class FakeAndroidLibraryPlugin implements Plugin<Project> {
            @Override
            void apply(Project project) {
              project.tasks.register('bundleReleaseAar') { task ->
                File aarDir = new File(project.buildDir, 'outputs/aar')
                File aar = new File(aarDir, "${project.name}-release.aar")
                task.outputs.file(aar)
                task.doLast {
                  aarDir.mkdirs()
                  new ZipOutputStream(new FileOutputStream(aar)).withCloseable { zip ->
                    zip.putNextEntry(new ZipEntry('classes.jar'))
                    zip.write('stub'.getBytes('UTF-8'))
                    zip.closeEntry()
                  }
                }
              }
            }
          }
        '''.stripIndent()
    }
}
