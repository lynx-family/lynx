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

import static org.junit.Assert.assertEquals
import static org.junit.Assert.assertFalse
import static org.junit.Assert.assertTrue

class LynxLibraryBuildPluginTest {
    @Rule
    public TemporaryFolder temporaryFolder = new TemporaryFolder()

    @Test
    void regeneratesRegistryWhenManifestChanges() {
        File projectDir = temporaryFolder.newFolder('app')
        writeFakeAndroidPlugin(projectDir)
        new File(projectDir, 'settings.gradle').text = "rootProject.name = 'test-app'\n"
        new File(projectDir, 'build.gradle').text = '''
          plugins {
            id 'com.android.application'
            id 'org.lynxsdk.lynx.library-build'
          }
        '''.stripIndent()

        File packageDir = new File(projectDir, 'node_modules/demo-library')
        new File(packageDir, 'android').mkdirs()
        new File(packageDir, 'android/build.gradle').text = ''
        File manifest = new File(packageDir, 'lynx.lib.json')
        manifest.text = manifestFor('com.example.initial')

        BuildResult firstBuild = runRegistryTask(projectDir)

        assertEquals(TaskOutcome.SUCCESS,
            firstBuild.task(':generateDebugLynxLibraryRegistry').outcome)
        File generatedRegistry = new File(projectDir,
            'build/generated/source/lynxLibraryRegistry/debug/' +
                'com/lynx/tasm/library/LynxAutolinkGenerated.java')
        assertTrue(generatedRegistry.text.contains(
            'com.example.initial.LynxLibraryProviderImpl'))

        manifest.text = manifestFor('com.example.renamed')
        BuildResult secondBuild = runRegistryTask(projectDir)

        assertEquals(TaskOutcome.SUCCESS,
            secondBuild.task(':generateDebugLynxLibraryRegistry').outcome)
        assertFalse(generatedRegistry.text.contains(
            'com.example.initial.LynxLibraryProviderImpl'))
        assertTrue(generatedRegistry.text.contains(
            'com.example.renamed.LynxLibraryProviderImpl'))
    }

    private static BuildResult runRegistryTask(File projectDir) {
        GradleRunner.create()
            .withProjectDir(projectDir)
            .withPluginClasspath()
            .withArguments('generateDebugLynxLibraryRegistry', '--stacktrace')
            .build()
    }

    private static String manifestFor(String packageName) {
        """{
          "platforms": {
            "android": {
              "packageName": "${packageName}",
              "sourceDir": "android"
            }
          }
        }"""
    }

    private static void writeFakeAndroidPlugin(File projectDir) {
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
              fakeAndroidApplication {
                id = 'com.android.application'
                implementationClass = 'FakeAndroidApplicationPlugin'
              }
            }
          }
        '''.stripIndent()
        new File(buildSrc,
            'src/main/groovy/FakeAndroidApplicationPlugin.groovy').text = '''
          import org.gradle.api.Plugin
          import org.gradle.api.Project
          import org.gradle.api.Task

          class FakeAndroidApplicationPlugin implements Plugin<Project> {
            @Override
            void apply(Project project) {
              project.extensions.add('android', new FakeAndroidExtension())
            }
          }

          class FakeAndroidExtension {
            final FakeApplicationVariants applicationVariants =
                new FakeApplicationVariants()
          }

          class FakeApplicationVariants {
            private final List<FakeAndroidVariant> variants = [
              new FakeAndroidVariant(name: 'debug')
            ]

            void all(Closure action) {
              variants.each(action)
            }
          }

          class FakeAndroidVariant {
            String name

            void registerJavaGeneratingTask(Task task, File generatedDir) {}
          }
        '''.stripIndent()
    }
}
