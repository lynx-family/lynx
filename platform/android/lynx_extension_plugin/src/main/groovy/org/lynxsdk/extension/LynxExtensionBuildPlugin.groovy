// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.extension

import org.gradle.api.Plugin
import org.gradle.api.Project
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

        android.applicationVariants.all { variant ->
            configureAndroidVariant(project, variant, extensions)
        }
    }

    private static void configureAndroidVariant(
        Project project, Object variant, List<LynxExtensionInfo> extensions) {
        String variantName = variant.name
        String taskName = "generate${variantName.capitalize()}LynxExtensionRegistry"
        File generatedDir = new File(
            project.buildDir, "generated/source/lynxExtensionRegistry/${variantName}")
        def taskProvider = project.tasks.register(taskName) { task ->
            task.outputs.dir(generatedDir)
            task.doLast {
                File packageDir = new File(generatedDir,
                    LynxExtensionRegistryGenerator.REGISTRY_PACKAGE_NAME.replace('.', '/'))
                packageDir.mkdirs()
                File output = new File(packageDir,
                    "${LynxExtensionRegistryGenerator.REGISTRY_CLASS_NAME}.java")
                output.text = LynxExtensionRegistryGenerator.generate(extensions)
            }
        }
        variant.registerJavaGeneratingTask(taskProvider.get(), generatedDir)
        wireGeneratedRegistrySource(project, variantName, taskProvider, generatedDir)
    }

    private static void wireGeneratedRegistrySource(
        Project project, String variantName, Object taskProvider, File generatedDir) {
        String compileTaskName = "compile${variantName.capitalize()}JavaWithJavac"
        project.tasks.withType(JavaCompile).matching { JavaCompile task ->
            task.name == compileTaskName
        }.configureEach { JavaCompile task ->
            task.dependsOn(taskProvider)
            task.source(generatedDir)
        }
    }

}
