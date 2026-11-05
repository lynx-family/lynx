// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.Plugin
import org.gradle.api.GradleException
import org.gradle.api.Project
import org.gradle.api.attributes.Attribute
import org.gradle.api.tasks.PathSensitivity
import org.gradle.api.tasks.compile.JavaCompile

import java.util.zip.ZipFile

class LynxLibraryBuildPlugin implements Plugin<Project> {
    private static final String AAR_MANIFEST_ENTRY = 'META-INF/lynx/lynx.lib.json'

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
        boolean scanAar = sources.contains(LynxLibraryScanner.SOURCE_AAR)
        // Source-flow libraries are known at configuration time; aar libraries are
        // only known after dependency resolution, so they are appended per-variant.
        List<LynxLibraryInfo> sourceLibraries = deduplicateByProvider(libraries)
        root.extensions.extraProperties.set('lynxAutolinkLibraries', sourceLibraries)

        root.allprojects { Project subproject ->
            configureLibraryProject(subproject, sourceLibraries)
            subproject.plugins.withId('com.android.application') {
                configureAndroidConsumer(subproject, sourceLibraries, scanAar)
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
        Project project, List<LynxLibraryInfo> sourceLibraries, boolean scanAar) {
        sourceLibraries.each { LynxLibraryInfo library ->
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
            configureAndroidVariant(project, variant, sourceLibraries, scanAar)
        }
    }

    private static void configureAndroidVariant(
        Project project, Object variant, List<LynxLibraryInfo> sourceLibraries, boolean scanAar) {
        String variantName = variant.name
        String taskName = "generate${variantName.capitalize()}LynxLibraryRegistry"
        File generatedDir = new File(
            project.buildDir, "generated/source/lynxLibraryRegistry/${variantName}")
        def taskProvider = project.tasks.register(taskName) { task ->
            // Declare source-library manifests as task inputs so a manifest change
            // (e.g. provider package rename) re-runs generation without a clean
            // build. aar libraries are resolved at execution time and tracked via
            // the runtime classpath, so they are not declared here.
            task.inputs.files(sourceLibraries.collect { it.manifestFile })
                .withPropertyName('lynxLibraryManifests')
                .withPathSensitivity(PathSensitivity.RELATIVE)
            task.outputs.dir(generatedDir)
            task.doLast {
                // aar libraries are only resolvable at execution time; merge them
                // with the source libraries, then dedup by provider class.
                List<LynxLibraryInfo> aarLibraries = scanAar ?
                    scanAarLibraries(project, variantName) : []
                List<LynxLibraryInfo> libraries =
                    deduplicateByProvider(sourceLibraries + aarLibraries)
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
        configureNodeApiAddons(project, variantName, sourceLibraries)
    }

    // Enumerate the AARs resolved for this variant's runtime classpath and read
    // each one's top-level META-INF/lynx/lynx.lib.json. Runs at execution time so
    // it never forces resolution during configuration or mutates the graph.
    // Falls back to skip+warn (empty list) on AGP versions without the artifact
    // view API.
    private static List<LynxLibraryInfo> scanAarLibraries(Project project, String variantName) {
        String configName = "${variantName}RuntimeClasspath"
        def configuration = project.configurations.findByName(configName)
        if (configuration == null) {
            project.logger.warn(
                "[LynxLibrary] aar scan skipped: configuration '${configName}' not found")
            return []
        }
        def artifacts
        try {
            Attribute<String> artifactType = Attribute.of('artifactType', String)
            artifacts = configuration.incoming.artifactView { viewConfig ->
                viewConfig.attributes { it.attribute(artifactType, 'aar') }
                viewConfig.lenient(true)
            }.artifacts.artifacts
        } catch (Throwable t) {
            project.logger.warn(
                "[LynxLibrary] aar scan skipped: artifact view unavailable (${t.message})")
            return []
        }

        Map<String, LynxLibraryInfo> byManifest = [:]
        artifacts.each { artifact ->
            File file = artifact.file
            if (file == null || !file.name.endsWith('.aar')) {
                return
            }
            String identifier = artifact.id?.componentIdentifier?.displayName ?: file.name
            String jsonText = readAarManifest(file)
            if (jsonText == null) {
                return
            }
            LynxLibraryInfo info = LynxLibraryScanner.parseAarManifest(jsonText, identifier)
            if (info != null) {
                byManifest[info.providerClassName] = info
            }
        }
        byManifest.values().toList().sort { it.npmName }
    }

    // Read the AAR's top-level META-INF/lynx/lynx.lib.json entry, or null if the
    // file is absent/unreadable. Package-private for unit testing; the enclosing
    // artifact enumeration is covered by end-to-end tests instead.
    static String readAarManifest(File aar) {
        try {
            ZipFile zip = new ZipFile(aar)
            try {
                def entry = zip.getEntry(AAR_MANIFEST_ENTRY)
                return entry == null ? null : zip.getInputStream(entry).getText('UTF-8')
            } finally {
                zip.close()
            }
        } catch (Exception ignored) {
            return null
        }
    }

    private static void configureNodeApiAddons(
        Project project, String variantName, List<LynxLibraryInfo> libraries) {
        // Only source-flow addons carry a jniLibsDir whose .so must be copied into
        // the app. AAR addons ship their .so inside the AAR, so they contribute
        // only System.loadLibrary via the generated registry, no copy here.
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
