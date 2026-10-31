// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package org.lynxsdk.library

import org.gradle.api.Plugin
import org.gradle.api.initialization.Settings

class LynxLibrarySettingsPlugin implements Plugin<Settings> {
    @Override
    void apply(Settings settings) {
        // Only node_modules libraries are included at settings stage. local_project
        // libraries are included by the host itself; aar libraries need no include.
        List<String> sources = LynxLibraryScanner.enabledSources(readSourcesProperty(settings))
        if (!sources.contains(LynxLibraryScanner.SOURCE_NODE_MODULES)) {
            return
        }
        List<LynxLibraryInfo> libraries = LynxLibraryScanner.scan(settings.settingsDir)
        settings.extensions.extraProperties.set('lynxAutolinkLibraries', libraries)
        libraries.each { LynxLibraryInfo library ->
            settings.include(library.projectPath)
            settings.project(library.projectPath).projectDir = library.androidDir
        }
    }

    // Read the sources property at settings stage across Gradle versions, in
    // three fallback tiers:
    //   1. settings.providers.gradleProperty (newer Gradle)
    //   2. -P command-line project properties (all versions)
    //   3. gradle.properties in the settings dir
    // Returns null if none provide it, which enables all sources by default.
    private static Object readSourcesProperty(Settings settings) {
        String key = LynxLibraryScanner.SOURCES_PROPERTY
        try {
            def value = settings.providers.gradleProperty(key).getOrNull()
            if (value != null) {
                return value
            }
        } catch (Throwable ignored) {
            // Older Gradle has no Settings.providers. The broad catch is safe
            // because the two fallbacks below still resolve the property.
        }
        def fromStartParameter = settings.startParameter.projectProperties.get(key)
        if (fromStartParameter != null) {
            return fromStartParameter
        }
        File gradleProperties = new File(settings.settingsDir, 'gradle.properties')
        if (gradleProperties.isFile()) {
            Properties props = new Properties()
            gradleProperties.withInputStream { props.load(it) }
            return props.getProperty(key)
        }
        null
    }
}
