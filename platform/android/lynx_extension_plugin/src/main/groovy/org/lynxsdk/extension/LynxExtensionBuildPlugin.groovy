// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.GradleException
import org.gradle.api.tasks.compile.JavaCompile

class LynxExtensionBuildPlugin implements Plugin<Project> {
    @Override
    void apply(Project project) {
        Project root = project.rootProject
        List<LynxExtensionInfo> extensions = LynxExtensionScanner.scan(root.projectDir)
        root.extensions.extraProperties.set('lynxAutolinkExtensions', extensions)

        root.allprojects { Project subproject ->
            configureExtensionProject(subproject, extensions)
            subproject.plugins.withId('com.android.application') {
                configureAndroidConsumer(subproject, extensions)
            }
        }
    }

    private static void configureExtensionProject(
        Project project, List<LynxExtensionInfo> extensions) {
        LynxExtensionInfo extension = extensions.find { it.projectPath == project.path }
        if (extension == null) {
            return
        }

        project.tasks.withType(JavaCompile).configureEach { JavaCompile task ->
            task.options.compilerArgs +=
                "-Alynx.extension.packageName=${extension.androidPackageName}"
        }

        project.plugins.withId('kotlin-kapt') {
            Object kapt = project.extensions.findByName('kapt')
            if (kapt != null && kapt.metaClass.respondsTo(kapt, 'arguments', Closure)) {
                kapt.arguments {
                    arg('lynx.extension.packageName', extension.androidPackageName)
                }
            }
        }
    }

    private static void configureAndroidConsumer(
        Project project, List<LynxExtensionInfo> extensions) {
        extensions.each { LynxExtensionInfo extension ->
            Project extensionProject = project.rootProject.findProject(extension.projectPath)
            if (extensionProject != null && extensionProject != project) {
                project.dependencies.add('implementation',
                    project.dependencies.project(path: extension.projectPath))
            }
        }

        Object android = project.extensions.findByName('android')
        if (android == null) {
            return
        }

        File generatedDir = new File(project.buildDir, 'generated/source/lynxExtensionRegistry')
        project.tasks.register('generateLynxExtensionRegistry') { task ->
            task.outputs.dir(generatedDir)
            task.doLast {
                String packageName = resolvePackageName(project, android)
                File packageDir = new File(generatedDir,
                    "${packageName.replace('.', '/')}/generated/extensions")
                packageDir.mkdirs()
                File output = new File(packageDir, 'ExtensionRegistry.java')
                output.text = LynxExtensionRegistryGenerator.generate(packageName, extensions)
            }
        }

        android.sourceSets.main.java.srcDir(generatedDir)
        project.tasks.matching { it.name == 'preBuild' }.configureEach {
            it.dependsOn(project.tasks.named('generateLynxExtensionRegistry'))
        }
    }

    private static String resolvePackageName(Project project, Object android) {
        if (android.hasProperty('namespace') && android.namespace) {
            return android.namespace
        }
        if (android.defaultConfig?.applicationId) {
            return android.defaultConfig.applicationId
        }
        File manifest = new File(project.projectDir, 'src/main/AndroidManifest.xml')
        if (manifest.isFile()) {
            def matcher = manifest.text =~ /package\s*=\s*"([^"]+)"/
            if (matcher.find()) {
                return matcher.group(1)
            }
        }
        throw new GradleException(
            "Unable to resolve Android package name for Lynx ExtensionRegistry in ${project.path}")
    }
}
