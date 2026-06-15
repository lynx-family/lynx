// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.Dependencies;
import com.google.devtools.ksp.symbol.KSFile;
import com.squareup.javapoet.JavaFile;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.util.Collection;
import java.util.LinkedHashSet;
import java.util.Set;

final class SourceWriter {
  private SourceWriter() {}

  static void write(CodeGenerator codeGenerator, JavaFile javaFile, Collection<KSFile> sourceFiles)
      throws IOException {
    Set<KSFile> uniqueFiles = new LinkedHashSet<>(sourceFiles);
    Dependencies dependencies =
        new Dependencies(false, uniqueFiles.toArray(new KSFile[uniqueFiles.size()]));
    OutputStream output = codeGenerator.createNewFile(
        dependencies, javaFile.packageName, javaFile.typeSpec.name, "java");
    try (Writer writer = new OutputStreamWriter(output, StandardCharsets.UTF_8)) {
      javaFile.writeTo(writer);
    }
  }
}
