// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.tasks.compile.JavaCompile

class LynxLibraryBuildPlugin implements Plugin<Project> {
    @Override
    void apply(Project project) {
        Project root = project.rootProject
        List<LynxLibraryInfo> libraries = LynxLibraryScanner.scan(root.projectDir)
        root.extensions.extraProperties.set('lynxAutolinkLibraries', libraries)

        root.allprojects { Project subproject ->
            configureLibraryProject(subproject, libraries)
            subproject.plugins.withId('com.android.application') {
                configureAndroidConsumer(subproject, libraries)
            }
        }
    }

    private static void configureLibraryProject(
        Project project, List<LynxLibraryInfo> libraries) {
        LynxLibraryInfo library = libraries.find { it.projectPath == project.path }
        if (library == null) {
            return
        }

        project.tasks.withType(JavaCompile).configureEach { JavaCompile task ->
            task.options.compilerArgs +=
                "-Alynx.library.packageName=${library.androidPackageName}"
        }

        project.plugins.withId('kotlin-kapt') {
            Object kapt = project.extensions.findByName('kapt')
            if (kapt != null && kapt.metaClass.respondsTo(kapt, 'arguments', Closure)) {
                kapt.arguments {
                    arg('lynx.library.packageName', library.androidPackageName)
                }
            }
        }
    }

    private static void configureAndroidConsumer(
        Project project, List<LynxLibraryInfo> libraries) {
        libraries.each { LynxLibraryInfo library ->
            Project libraryProject = project.rootProject.findProject(library.projectPath)
            if (libraryProject != null && libraryProject != project) {
                project.dependencies.add('implementation',
                    project.dependencies.project(path: library.projectPath))
            }
        }

        Object android = project.extensions.findByName('android')
        if (android == null) {
            return
        }

        android.applicationVariants.all { variant ->
            configureAndroidVariant(project, variant, libraries)
        }
    }

    private static void configureAndroidVariant(
        Project project, Object variant, List<LynxLibraryInfo> libraries) {
        String variantName = variant.name
        String taskName = "generate${variantName.capitalize()}LynxLibraryRegistry"
        File generatedDir = new File(
            project.buildDir, "generated/source/lynxLibraryRegistry/${variantName}")
        def taskProvider = project.tasks.register(taskName) { task ->
            task.outputs.dir(generatedDir)
            task.doLast {
                File packageDir = new File(generatedDir,
                    LynxLibraryRegistryGenerator.REGISTRY_PACKAGE_NAME.replace('.', '/'))
                packageDir.mkdirs()
                File output = new File(packageDir,
                    "${LynxLibraryRegistryGenerator.REGISTRY_CLASS_NAME}.java")
                output.text = LynxLibraryRegistryGenerator.generate(libraries)
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
