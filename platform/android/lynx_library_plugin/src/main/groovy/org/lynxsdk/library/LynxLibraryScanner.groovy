// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import groovy.json.JsonSlurper
import org.gradle.api.GradleException

class LynxLibraryScanner {
    private static final String ADDON_NAME_PATTERN = '[A-Za-z0-9_.-]+'

    static List<LynxLibraryInfo> scan(File startDir) {
        Set<File> nodeModulesDirs = findNodeModulesDirs(startDir)
        List<LynxLibraryInfo> result = []
        nodeModulesDirs.each { File nodeModules ->
            findManifestFiles(nodeModules).each { File manifest ->
                LynxLibraryInfo info = parseManifest(manifest)
                if (info != null) {
                    result << info
                }
            }
        }
        result.sort { it.npmName }
    }

    private static Set<File> findNodeModulesDirs(File startDir) {
        Set<File> dirs = new LinkedHashSet<>()
        File current = startDir
        int depth = 0
        while (current != null && depth < 6) {
            File nodeModules = new File(current, 'node_modules')
            if (nodeModules.isDirectory()) {
                dirs << canonicalOrAbsolute(nodeModules)
            }
            current = current.parentFile
            depth++
        }
        dirs
    }

    static File canonicalOrAbsolute(File file) {
        try {
            return file.canonicalFile
        } catch (IOException ignored) {
            return file.absoluteFile
        }
    }

    private static List<File> findManifestFiles(File nodeModules) {
        List<File> manifests = []
        File[] packages = nodeModules.listFiles()
        if (packages == null) {
            return manifests
        }
        packages.each { File pkg ->
            if (!pkg.isDirectory()) {
                return
            }
            if (pkg.name.startsWith('.')) {
                return
            }
            if (pkg.name.startsWith('@')) {
                File[] scopedPackages = pkg.listFiles()
                if (scopedPackages != null) {
                    scopedPackages.each { File scopedPkg ->
                        addManifest(manifests, scopedPkg)
                    }
                }
            } else {
                addManifest(manifests, pkg)
            }
        }
        manifests
    }

    private static void addManifest(List<File> manifests, File packageDir) {
        if (!packageDir.isDirectory()) {
            return
        }
        File manifest = new File(packageDir, 'lynx.lib.json')
        if (manifest.isFile()) {
            manifests << manifest
        }
    }

    private static LynxLibraryInfo parseManifest(File manifest) {
        Object json
        try {
            json = new JsonSlurper().parse(manifest)
        } catch (Exception e) {
            throw new GradleException("Failed to parse ${manifest}: ${e.message}", e)
        }

        Object android = json?.platforms?.android
        if (android == null) {
            return null
        }
        if (!(android instanceof Map)) {
            throw new GradleException("Invalid android platform entry in ${manifest}")
        }

        String packageName = android.packageName
        if (packageName == null || packageName.trim().isEmpty()) {
            throw new GradleException("Missing platforms.android.packageName in ${manifest}")
        }
        String sourceDir = android.sourceDir ?: 'android'
        File packageDir = manifest.parentFile
        File androidDir = new File(packageDir, sourceDir)
        if (!androidDir.isDirectory()) {
            throw new GradleException(
                "Android sourceDir '${sourceDir}' does not exist for ${manifest}")
        }
        if (!new File(androidDir, 'build.gradle').isFile()
            && !new File(androidDir, 'build.gradle.kts').isFile()) {
            throw new GradleException(
                "Android sourceDir '${sourceDir}' for ${manifest} must contain a build.gradle file")
        }

        String npmName = packageDir.name
        if (packageDir.parentFile?.name?.startsWith('@')) {
            npmName = "${packageDir.parentFile.name}/${packageDir.name}"
        }
        List<LynxNodeApiAddonInfo> nodeApiAddons = parseNodeApiAddons(
            android.nodeApiAddons, packageDir, manifest)
        String providerClassName = parseProviderClassName(
            android, packageName.trim(), manifest)
        new LynxLibraryInfo(npmName, packageDir, manifest, packageName.trim(), sourceDir,
            androidDir, projectPathFor(npmName), providerClassName, nodeApiAddons)
    }

    private static String parseProviderClassName(
        Map android, String packageName, File manifest) {
        if (!android.containsKey('providerClassName')) {
            return "${packageName}.LynxLibraryProviderImpl"
        }
        Object providerClassName = android.providerClassName
        if (providerClassName == null) {
            return null
        }
        if (!(providerClassName instanceof String)
            || providerClassName.trim().isEmpty()
            || !(providerClassName.trim() ==~ '[A-Za-z_$][A-Za-z0-9_$]*(\\.[A-Za-z_$][A-Za-z0-9_$]*)+')) {
            throw new GradleException(
                "Invalid platforms.android.providerClassName in ${manifest}; expected a fully qualified Java class name or null")
        }
        providerClassName.trim()
    }

    private static List<LynxNodeApiAddonInfo> parseNodeApiAddons(
        Object addons, File packageDir, File manifest) {
        if (addons == null) {
            return []
        }
        if (!(addons instanceof List)) {
            throw new GradleException("platforms.android.nodeApiAddons in ${manifest} must be an array")
        }
        List<LynxNodeApiAddonInfo> result = []
        addons.eachWithIndex { Object addon, int index ->
            if (!(addon instanceof Map)) {
                throw new GradleException(
                    "platforms.android.nodeApiAddons[${index}] in ${manifest} must be an object")
            }
            String name = addon.name?.trim()
            validateAddonName(name, "platforms.android.nodeApiAddons[${index}].name", manifest)
            String libraryName = (addon.libraryName ?: name)?.trim()
            validateAddonName(libraryName,
                "platforms.android.nodeApiAddons[${index}].libraryName", manifest)

            File jniLibsDir = null
            if (addon.containsKey('jniLibsDir')) {
                if (!(addon.jniLibsDir instanceof String)
                    || addon.jniLibsDir.trim().isEmpty()) {
                    throw new GradleException(
                        "platforms.android.nodeApiAddons[${index}].jniLibsDir in ${manifest} must be a non-empty string")
                }
                String jniLibsDirName = addon.jniLibsDir.trim()
                jniLibsDir = resolvePackagePath(
                    packageDir, jniLibsDirName, manifest,
                    "platforms.android.nodeApiAddons[${index}].jniLibsDir")
            }
            if (addon.containsKey('required') && !(addon.required instanceof Boolean)) {
                throw new GradleException(
                    "platforms.android.nodeApiAddons[${index}].required in ${manifest} must be a boolean")
            }
            boolean required = addon.containsKey('required') ? addon.required : true
            result << new LynxNodeApiAddonInfo(name, libraryName, jniLibsDir, required)
        }
        result
    }

    private static void validateAddonName(String name, String fieldName, File manifest) {
        if (name == null || name.trim().isEmpty()) {
            throw new GradleException("Missing ${fieldName} in ${manifest}")
        }
        String normalized = name.trim()
        if (normalized.size() > 128 || normalized.contains('..') ||
            !(normalized ==~ ADDON_NAME_PATTERN)) {
            throw new GradleException(
                "Invalid ${fieldName} '${name}' in ${manifest}; expected [A-Za-z0-9_.-] without '..'")
        }
    }

    private static File resolvePackagePath(
        File packageDir, String configuredPath, File manifest, String fieldName) {
        File path = new File(configuredPath)
        if (!path.isAbsolute()) {
            path = new File(packageDir, configuredPath)
        }
        File packageRoot = canonicalOrAbsolute(packageDir)
        File resolved = canonicalOrAbsolute(path)
        String packagePath = packageRoot.path
        String resolvedPath = resolved.path
        if (resolvedPath == packagePath || resolvedPath.startsWith("${packagePath}${File.separator}")) {
            return resolved
        }
        throw new GradleException(
            "Android ${fieldName} '${configuredPath}' must stay within package directory for ${manifest}")
    }

    static String projectPathFor(String npmName) {
        String safeName = npmName.replaceAll('[^A-Za-z0-9_]+', '_')
        ":lynx_library_${safeName}"
    }
}
