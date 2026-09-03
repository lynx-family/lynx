// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.Plugin
import org.gradle.api.GradleException
import org.gradle.api.Project
import org.gradle.api.tasks.compile.JavaCompile

class LynxLibraryBuildPlugin implements Plugin<Project> {
    @Override
    void apply(Project project) {
        Project root = project.rootProject
        List<String> sources = LynxLibraryScanner.enabledSources(
            root.findProperty(LynxLibraryScanner.SOURCES_PROPERTY))

        List<LynxLibraryInfo> libraries = []
        if (sources.contains(LynxLibraryScanner.SOURCE_NODE_MODULES)) {
            libraries.addAll(LynxLibraryScanner.scan(root.projectDir))
        }
        if (sources.contains(LynxLibraryScanner.SOURCE_LOCAL_PROJECT)) {
            libraries.addAll(scanLocalProjects(root, libraries))
        }
        libraries = deduplicateByProvider(libraries)
        root.extensions.extraProperties.set('lynxAutolinkLibraries', libraries)

        root.allprojects { Project subproject ->
            configureLibraryProject(subproject, libraries)
            subproject.plugins.withId('com.android.application') {
                configureAndroidConsumer(subproject, libraries)
            }
        }
    }

    // Discover host-included local projects that ship a lynx.lib.json. Skip any
    // project already found via node_modules (same projectDir) to avoid double
    // discovery.
    private static List<LynxLibraryInfo> scanLocalProjects(
        Project root, List<LynxLibraryInfo> alreadyFound) {
        Set<File> knownDirs = alreadyFound.collect {
            LynxLibraryScanner.canonicalOrAbsolute(it.androidDir)
        }.toSet()
        List<LynxLibraryInfo> result = []
        root.allprojects.each { Project subproject ->
            if (subproject == root) {
                return
            }
            File dir = LynxLibraryScanner.canonicalOrAbsolute(subproject.projectDir)
            if (knownDirs.contains(dir)) {
                return
            }
            LynxLibraryInfo info = LynxLibraryScanner.scanLocalProject(
                subproject.projectDir, subproject.path)
            if (info != null) {
                result << info
            }
        }
        result.sort { it.npmName }
    }

    // Merge sources and drop duplicate providers. The same package resolved from
    // more than one source is expected; keep the first and stay silent.
    private static List<LynxLibraryInfo> deduplicateByProvider(List<LynxLibraryInfo> libraries) {
        libraries.unique { it.providerClassName }
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
            // Only node_modules libraries are included by the settings plugin and
            // need the build plugin to wire `implementation project(...)`.
            // local_project libraries are depended on by the host itself.
            if (library.source != LynxLibraryScanner.SOURCE_NODE_MODULES) {
                return
            }
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
        configureNodeApiAddons(project, variantName, libraries)
    }

    private static void configureNodeApiAddons(
        Project project, String variantName, List<LynxLibraryInfo> libraries) {
        List<LynxLibraryInfo> librariesWithAddons = libraries.findAll { LynxLibraryInfo library ->
            library.nodeApiAddons.any { LynxNodeApiAddonInfo addon ->
                addon.hasPrebuiltLibrary()
            }
        }
        if (librariesWithAddons.isEmpty()) {
            return
        }

        Object android = project.extensions.findByName('android')
        if (android == null) {
            return
        }

        File generatedJniLibsDir = new File(
            project.buildDir, "generated/lynxNodeApiAddons/${variantName}/jniLibs")
        String taskName = "copy${variantName.capitalize()}LynxNodeApiAddons"
        def copyTask = project.tasks.register(taskName) { task ->
            task.outputs.dir(generatedJniLibsDir)
            task.doLast {
                project.delete(generatedJniLibsDir)
                librariesWithAddons.each { LynxLibraryInfo library ->
                    library.nodeApiAddons.findAll { LynxNodeApiAddonInfo addon ->
                        addon.hasPrebuiltLibrary()
                    }.each { LynxNodeApiAddonInfo addon ->
                        copyNodeApiAddon(project, library, addon, generatedJniLibsDir)
                    }
                }
            }
        }

        android.sourceSets.maybeCreate(variantName).jniLibs.srcDir(generatedJniLibsDir)
        String mergeTaskName = "merge${variantName.capitalize()}JniLibFolders"
        project.tasks.matching { it.name == mergeTaskName }.configureEach { task ->
            task.dependsOn(copyTask)
        }
    }

    private static void copyNodeApiAddon(Project project, LynxLibraryInfo library,
        LynxNodeApiAddonInfo addon, File generatedJniLibsDir) {
        File[] abiDirs = addon.jniLibsDir.listFiles({ File file -> file.isDirectory() } as FileFilter)
        if (abiDirs == null || abiDirs.length == 0) {
            handleMissingAddon(library, addon,
                "Node-API addon '${addon.name}' declares '${addon.jniLibsDir}', but no ABI directories were found")
            return
        }

        int copied = 0
        abiDirs.each { File abiDir ->
            File source = new File(abiDir, addon.sharedLibraryName)
            if (!source.isFile()) {
                return
            }
            File targetDir = new File(generatedJniLibsDir, abiDir.name)
            project.copy {
                from source
                into targetDir
            }
            copied++
        }

        if (copied == 0) {
            handleMissingAddon(library, addon,
                "Node-API addon '${addon.name}' could not find '${addon.sharedLibraryName}' under ${addon.jniLibsDir}")
        }
    }

    private static void handleMissingAddon(
        LynxLibraryInfo library, LynxNodeApiAddonInfo addon, String message) {
        String fullMessage = "${message} for ${library.manifestFile}"
        if (addon.required) {
            throw new GradleException(fullMessage)
        }
        println("[LynxLibrary] ${fullMessage}")
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
