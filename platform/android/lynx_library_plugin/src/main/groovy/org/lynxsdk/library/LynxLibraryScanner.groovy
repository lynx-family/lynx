// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import groovy.json.JsonSlurper
import org.gradle.api.GradleException

class LynxLibraryScanner {
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
        new LynxLibraryInfo(npmName, packageDir, manifest, packageName.trim(), sourceDir,
            androidDir, projectPathFor(npmName))
    }

    static String projectPathFor(String npmName) {
        String safeName = npmName.replaceAll('[^A-Za-z0-9_]+', '_')
        ":lynx_library_${safeName}"
    }
}
