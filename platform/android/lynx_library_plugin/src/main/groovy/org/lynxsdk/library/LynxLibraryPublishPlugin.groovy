// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import groovy.json.JsonSlurper
import org.gradle.api.GradleException
import org.gradle.api.Plugin
import org.gradle.api.Project

// Publisher-side plugin for Lynx native libraries shipped as an AAR.
//
// A published AAR is pre-compiled: the consumer's autolink plugin cannot run the
// annotation processor over it, so the provider must already be compiled into the
// AAR and a manifest must sit at the AAR top-level META-INF/lynx/. This plugin
// removes the boilerplate every AAR publisher would otherwise hand-write:
//   1. Inject the kapt argument `lynx.library.packageName` so lynx-processor emits
//      <packageName>.LynxLibraryProviderImpl (provider generation is opt-in).
//   2. Register a task that injects lynx.lib.json into the AAR top-level
//      META-INF/lynx/ and finalize bundle<Variant>Aar into it.
//
// The lynx-processor dependency itself is NOT added here; the publisher declares
// `kapt("...:lynx-processor:<sdkVersion>")` with its own SDK version so it stays
// aligned with the runtime. packageName has a single source of truth: it is read
// from lynx.lib.json and reused for the kapt argument, so it can never drift from
// the manifest the consumer reads.
class LynxLibraryPublishPlugin implements Plugin<Project> {
    private static final String EXTENSION_NAME = 'lynxLibraryPublish'
    private static final String MANIFEST_FILE_NAME = 'lynx.lib.json'
    private static final String MANIFEST_PREFIX = 'META-INF/lynx'
    private static final String KAPT_PACKAGE_NAME_ARG = 'lynx.library.packageName'

    @Override
    void apply(Project project) {
        LynxLibraryPublishExtension extension = project.extensions.create(
            EXTENSION_NAME, LynxLibraryPublishExtension)

        project.afterEvaluate {
            File manifest = resolveManifest(project, extension)
            String packageName = readPackageName(manifest)

            injectKaptPackageName(project, packageName)
            registerManifestInjection(project, manifest)
        }
    }

    // DSL-specified path wins; otherwise the project root's lynx.lib.json. Missing
    // in both is an error: an AAR published without a manifest cannot be autolinked.
    private static File resolveManifest(Project project, LynxLibraryPublishExtension extension) {
        File manifest = extension.manifest ?: project.file(MANIFEST_FILE_NAME)
        if (!manifest.isFile()) {
            throw new GradleException(
                "Lynx autolink publish: ${MANIFEST_FILE_NAME} not found at ${manifest}. " +
                "Place it at the project root or set ${EXTENSION_NAME}.manifest.")
        }
        manifest
    }

    private static String readPackageName(File manifest) {
        Object json
        try {
            json = new JsonSlurper().parse(manifest)
        } catch (Exception e) {
            throw new GradleException("Failed to parse ${manifest}: ${e.message}", e)
        }
        Object android = json?.platforms?.android
        if (!(android instanceof Map)) {
            throw new GradleException("Missing platforms.android object in ${manifest}")
        }
        String packageName = android.packageName
        if (packageName == null || packageName.trim().isEmpty()) {
            throw new GradleException("Missing platforms.android.packageName in ${manifest}")
        }
        packageName.trim()
    }

    // Provider generation is opt-in: lynx-processor only emits the provider when
    // this argument is set. Cover both javac (-A) and kapt argument channels so it
    // works regardless of how the publisher compiles.
    private static void injectKaptPackageName(Project project, String packageName) {
        project.tasks.withType(org.gradle.api.tasks.compile.JavaCompile).configureEach { task ->
            task.options.compilerArgs += "-A${KAPT_PACKAGE_NAME_ARG}=${packageName}".toString()
        }
        project.plugins.withId('kotlin-kapt') {
            Object kapt = project.extensions.findByName('kapt')
            if (kapt != null && kapt.metaClass.respondsTo(kapt, 'arguments', Closure)) {
                kapt.arguments {
                    arg(KAPT_PACKAGE_NAME_ARG, packageName)
                }
            }
        }
    }

    // Inject the manifest into the AAR top-level META-INF/lynx/ via a plain zip
    // post-process (no AGP-version-specific packaging API).
    //
    // AARs are read from the producing tasks' own declared outputs, not by scanning
    // build/outputs/aar. This pins the exact files this build produced, so we never
    // pick up stale or other-variant AARs and never hardcode the output directory.
    // Every bundle<Flavor>ReleaseAar task is finalized into a single injection task
    // that injects each one, so multi-flavor release AARs each get the manifest.
    // Only the release build type is handled (publishing is release-only).
    private static void registerManifestInjection(Project project, File manifest) {
        def injectTask = project.tasks.register('injectLynxLibraryManifest') { task ->
            task.inputs.file(manifest)
            task.doLast {
                List<File> aarList = project.tasks
                    .matching { it.name ==~ /bundle.*ReleaseAar/ }
                    .collectMany { it.outputs.files.files }
                    .findAll { it.name.endsWith('.aar') && it.isFile() }
                    .unique()
                if (aarList.isEmpty()) {
                    throw new GradleException(
                        "Lynx autolink publish: no release AAR output found to inject.")
                }
                aarList.each { aar ->
                    project.ant.zip(destfile: aar.absolutePath, update: true) {
                        zipfileset(file: manifest.absolutePath, prefix: MANIFEST_PREFIX)
                    }
                    project.logger.lifecycle(
                        "[LynxLibrary] injected ${MANIFEST_PREFIX}/${MANIFEST_FILE_NAME} into ${aar.name}")
                }
            }
        }
        project.tasks.matching { it.name ==~ /bundle.*ReleaseAar/ }.configureEach { task ->
            task.finalizedBy(injectTask)
        }
    }
}
