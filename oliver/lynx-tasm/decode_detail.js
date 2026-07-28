// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

'use strict';

function buildTemplateDecodeDetail({
  decoded = {},
  sizeDecoded = {},
  templateBuffer,
  backend = 'napi',
}) {
  const buffer = toBuffer(templateBuffer);
  const parsedStrings = parseStringSection(buffer, sizeDecoded);
  const scriptRecords = collectScriptRecords(decoded, sizeDecoded, parsedStrings.items);
  const categoriesByStringIndex = buildStringCategories(
    parsedStrings.items,
    sizeDecoded,
    scriptRecords,
  );
  const rawContents = buildRawContents({
    decoded,
    sizeDecoded,
    parsedStrings,
    scriptRecords,
    categoriesByStringIndex,
  });
  const strings = buildStrings(
    parsedStrings,
    sizeDecoded,
    categoriesByStringIndex,
    rawContents,
    scriptRecords,
  );
  const totalBytes = readNumber(
    sizeDecoded,
    'total-size',
    readNumber(decoded, 'total-size', buffer.length),
  );

  return dropUndefined({
    schemaVersion: 1,
    status: parsedStrings.parsedFromBinary ? 'ok' : 'partial',
    backend,
    template: buildTemplateInfo(decoded, sizeDecoded),
    size: {
      totalBytes,
      sections: sizeCollection(buildBinarySections(sizeDecoded)),
      items: sizeCollection(buildSizeItems(sizeDecoded, rawContents), totalBytes),
    },
    content: {
      components: buildComponents(decoded),
      css: buildCssContent(strings),
      scripts: buildScripts(scriptRecords, rawContents),
      lepus: buildLepus(sizeDecoded, rawContents),
      strings,
      customSections: contentCollection(buildCustomSections(decoded, rawContents)),
      rawContents: contentCollection(rawContents.items),
    },
  });
}

function buildTemplateDecodeDetailError(error, backend) {
  return dropUndefined({
    schemaVersion: 1,
    status: `error:${normalizeErrorStatus(error)}`,
    backend,
  });
}

function createTemplateDecodeDetailApi({
  supportNapi,
  createModule,
  decodeNapi,
  decodeBinaryInfoNapi,
  decodeWasmJson,
  toTemplateBuffer = toBuffer,
}) {
  function decodeDetailNapiOrThrow(templateJS) {
    const templateArray = toTemplateBuffer(templateJS);
    const decoded = decodeNapi(templateArray);
    const sizeDecoded = decodeBinaryInfoNapi(templateArray);
    return buildTemplateDecodeDetail({
      decoded,
      sizeDecoded,
      templateBuffer: templateArray,
      backend: 'napi',
    });
  }

  async function decodeDetailWasmOrThrow(templateJS) {
    const templateArray = toTemplateBuffer(templateJS);
    const Module = await createModule();
    const decoded = decodeWasmJson(Module, templateArray, '_decode', 'decode');
    const sizeDecoded = decodeWasmJson(
      Module,
      templateArray,
      '_decode_template_binary_info',
      'template binary info decode',
    );
    return buildTemplateDecodeDetail({
      decoded,
      sizeDecoded,
      templateBuffer: templateArray,
      backend: 'wasm',
    });
  }

  function decode_detail_napi(templateJS) {
    try {
      return decodeDetailNapiOrThrow(templateJS);
    } catch (error) {
      return buildTemplateDecodeDetailError(error, 'napi');
    }
  }

  async function decode_detail_wasm(templateJS) {
    try {
      return await decodeDetailWasmOrThrow(templateJS);
    } catch (error) {
      return buildTemplateDecodeDetailError(error, 'wasm');
    }
  }

  async function decode_detail(templateJS, backend = 'auto') {
    if (backend === 'napi') {
      return decode_detail_napi(templateJS);
    }
    if (backend === 'wasm') {
      return decode_detail_wasm(templateJS);
    }
    if (backend !== 'auto') {
      return buildTemplateDecodeDetailError(
        new Error(`Invalid backend: ${backend}`),
      );
    }
    if (supportNapi()) {
      try {
        return decodeDetailNapiOrThrow(templateJS);
      } catch (error) {
        if (!isNapiLoadError(error)) {
          return buildTemplateDecodeDetailError(error, 'napi');
        }
      }
    }
    return decode_detail_wasm(templateJS);
  }

  return {
    decode_detail,
    decode_detail_napi,
    decode_detail_wasm,
  };
}

function buildTemplateInfo(decoded, sizeDecoded) {
  const compileOptions = objectOrEmpty(decoded.compilerOptions);
  return {
    appType: decoded['app-type'] || sizeDecoded['app-type'] || 'unknown',
    arch: inferArch(compileOptions),
    templateMode: compileOptions.enable_flexible_template_ ? 'flexible' : 'legacy',
    lepusType: inferLepusType(decoded, sizeDecoded, compileOptions),
    engineVersion: decoded['engine-version'] || sizeDecoded['engine-version'],
    targetSdkVersion: compileOptions.target_sdk_version_,
    compileOptions,
    templateInfo: objectOrEmpty(decoded.templateInfo || decoded['template-info']),
    trialOptions: objectOrEmpty(decoded.trialOptions || decoded['trial-options']),
  };
}

function inferArch(compileOptions) {
  if (compileOptions.enable_lynx_air_ || compileOptions.lynx_air_mode_ > 0) {
    return 'air';
  }
  if (compileOptions.enable_fiber_arch_ || compileOptions.arch_option_ === 1) {
    return 'fiber';
  }
  if (Object.prototype.hasOwnProperty.call(compileOptions, 'radon_mode_')) {
    return 'radon';
  }
  return 'unknown';
}

function inferLepusType(decoded, sizeDecoded, compileOptions) {
  const isLepusNg = decoded['is-lepusng-binary'] ?? sizeDecoded['is-lepusng-binary'];
  if (isLepusNg === true || compileOptions.enable_lepus_ng_ === true) {
    return 'lepus-ng';
  }
  if (isLepusNg === false || compileOptions.enable_lepus_ng_ === false) {
    return 'lepus';
  }
  return 'unknown';
}

function buildBinarySections(sizeDecoded) {
  return Object.entries(objectOrEmpty(sizeDecoded.sections))
    .filter(([, section]) => section && typeof section === 'object')
    .map(([name, section], index) => {
      const range = buildRange(section);
      const bytes = readNumber(section, 'size', range ? range.size : 0);
      return {
        name,
        bytes,
        range: range || {start: 0, end: bytes, size: bytes},
        known: !/^UNKNOWN/i.test(name),
        order: index,
      };
    })
    .sort((a, b) => {
      const startDelta = a.range.start - b.range.start;
      return startDelta || a.order - b.order;
    })
    .map((item, order) => ({...item, order}));
}

function buildSizeItems(sizeDecoded, rawContents) {
  const totalBytes = readNumber(sizeDecoded, 'total-size', 0);
  const sizeItems = [];
  const jsFiles = getJsSizeFiles(sizeDecoded);
  let jsPathBytes = 0;
  let jsContentBytes = 0;

  for (const file of jsFiles) {
    if (!file || typeof file !== 'object') {
      continue;
    }
    const bytes = readNumber(file, 'content_len', 0);
    if (typeof file.path === 'string' && bytes > 0) {
      sizeItems.push({
        name: file.path,
        bytes,
        kind: 'script',
        sourceSection: 'STRING',
        rawContentRef: rawContents.rawByStringIndex.get(file.content_index),
        details: {
          pathIndex: file.path_index,
          contentIndex: file.content_index,
        },
      });
      jsContentBytes += bytes;
    }
    jsPathBytes += readNumber(file, 'path_len', 0);
  }

  if (jsPathBytes > 0) {
    sizeItems.push({
      name: 'STRING.js_paths',
      bytes: jsPathBytes,
      kind: 'string',
      sourceSection: 'STRING',
    });
  }

  const rootLepus = sizeDecoded.chunks && sizeDecoded.chunks['root-lepus'];
  const rootRange = getSectionRange(sizeDecoded, 'ROOT_LEPUS');
  if (rootLepus && typeof rootLepus.payload_bytes === 'number') {
    sizeItems.push({
      name: 'ROOT_LEPUS.payload',
      bytes: rootLepus.payload_bytes,
      kind: 'lepus',
      range: rootRange,
      sourceSection: 'ROOT_LEPUS',
      rawContentRef: rawContents.lepusRootRef,
      details: {
        lepusngCodeBytes: rootLepus.lepusng_code_len,
        lengthFieldBytes: rootLepus.length_field_bytes,
      },
    });
  } else if (rootRange) {
    sizeItems.push({
      name: 'ROOT_LEPUS.section',
      bytes: rootRange.size,
      kind: 'lepus',
      range: rootRange,
      sourceSection: 'ROOT_LEPUS',
      rawContentRef: rawContents.lepusRootRef,
    });
  }

  const stringUsage = sizeDecoded.chunks && sizeDecoded.chunks['string-usage'];
  const otherBreakdown = objectOrEmpty(stringUsage && stringUsage.other_breakdown);
  for (const [type, stat] of Object.entries(otherBreakdown)) {
    if (!stat || typeof stat.bytes !== 'number') {
      continue;
    }
    sizeItems.push({
      name: `STRING.other.${type}`,
      bytes: stat.bytes,
      kind: 'string',
      sourceSection: 'STRING',
      details: {
        classification: type,
        count: stat.count,
      },
    });
  }

  const stringSection = sizeDecoded.sections && sizeDecoded.sections.STRING;
  if (stringSection && typeof stringSection.size === 'number') {
    const lengthFields = readNumber(stringSection, 'total_length_field_bytes', 0);
    const otherBytesSum = stringUsage ? readNumber(stringUsage, 'other_bytes_sum', 0) : 0;
    const stringAccounted = jsContentBytes + jsPathBytes + otherBytesSum + lengthFields;
    const stringOverhead = stringSection.size - stringAccounted;
    if (stringOverhead > 0) {
      sizeItems.push({
        name: 'STRING.section_overhead',
        bytes: stringOverhead,
        kind: 'string',
        sourceSection: 'STRING',
      });
    }
    if (lengthFields > 0) {
      sizeItems.push({
        name: 'STRING.length_fields',
        bytes: lengthFields,
        kind: 'string',
        sourceSection: 'STRING',
      });
    }
  }

  addWholeSectionSizeItem(sizeItems, sizeDecoded, 'JS', 'section');
  addWholeSectionSizeItem(sizeItems, sizeDecoded, 'CSS', 'css');
  addWholeSectionSizeItem(sizeItems, sizeDecoded, 'CONFIG', 'section');

  const includedSections = new Set(['STRING', 'JS', 'CSS', 'CONFIG', 'ROOT_LEPUS']);
  for (const [sectionName, section] of Object.entries(objectOrEmpty(sizeDecoded.sections))) {
    if (includedSections.has(sectionName) || !section || typeof section.size !== 'number') {
      continue;
    }
    sizeItems.push({
      name: `SECTION.${sectionName}`,
      bytes: section.size,
      kind: sectionName === 'CUSTOM_SECTIONS' ? 'custom-section' : 'section',
      range: buildRange(section),
      sourceSection: sectionName,
    });
  }

  sizeItems.sort((a, b) => b.bytes - a.bytes);
  const accountedBytes = sizeItems.reduce((sum, item) => sum + (item.bytes || 0), 0);
  const overhead = totalBytes - accountedBytes;
  if (overhead > 0) {
    sizeItems.push({
      name: 'BINARY.overhead',
      bytes: overhead,
      kind: 'other',
    });
    sizeItems.sort((a, b) => b.bytes - a.bytes);
  }

  return sizeItems;
}

function addWholeSectionSizeItem(sizeItems, sizeDecoded, sectionName, kind) {
  const section = sizeDecoded.sections && sizeDecoded.sections[sectionName];
  if (!section || typeof section.size !== 'number') {
    return;
  }
  sizeItems.push({
    name: `${sectionName}.section`,
    bytes: section.size,
    kind,
    range: buildRange(section),
    sourceSection: sectionName,
  });
}

function collectScriptRecords(decoded, sizeDecoded, stringItems) {
  const stringsByIndex = new Map(stringItems.map((item) => [item.index, item.value]));
  const byPath = new Map();
  const jsSizeFiles = getJsSizeFiles(sizeDecoded);

  for (let i = 0; i < jsSizeFiles.length; i += 1) {
    const file = jsSizeFiles[i];
    if (!file || typeof file !== 'object') {
      continue;
    }
    const path = typeof file.path === 'string'
      ? file.path
      : stringsByIndex.get(file.path_index) || `script-${i}`;
    byPath.set(path, {
      path,
      pathStringIndex: file.path_index,
      pathBytes: file.path_len,
      contentStringIndex: file.content_index,
      contentBytes: file.content_len,
      content: stringsByIndex.get(file.content_index),
      kind: 'source',
    });
  }

  const backgroundScripts = normalizeArray(decoded['background-thread-script']);
  for (const script of backgroundScripts) {
    if (!script || typeof script !== 'object') {
      continue;
    }
    const scriptPath = typeof script.path === 'string'
      ? script.path
      : `background-script-${byPath.size}`;
    const previous = byPath.get(scriptPath) || {path: scriptPath};
    const content = typeof script.content === 'string' ? script.content : previous.content;
    byPath.set(scriptPath, {
      ...previous,
      path: scriptPath,
      kind: script.type === 'bytecode' ? 'js-bytecode' : previous.kind || 'source',
      content,
      contentBytes: previous.contentBytes
        || (content ? Buffer.byteLength(content, 'utf8') : undefined),
    });
  }

  return Array.from(byPath.values());
}

function buildStringCategories(stringItems, sizeDecoded, scriptRecords) {
  const categoriesByIndex = new Map();
  const buckets = buildClassificationBuckets(sizeDecoded);
  const scriptContentIndices = new Set();

  for (const script of scriptRecords) {
    if (typeof script.contentStringIndex === 'number') {
      scriptContentIndices.add(script.contentStringIndex);
    }
  }

  for (const item of stringItems) {
    const categories = [];
    if (scriptContentIndices.has(item.index)) {
      categories.push('script');
    }
    const classification = consumeClassification(buckets, item.value, item.contentBytes);
    if (classification === 'css_selector') {
      categories.push('css-selector');
    } else if (classification === 'identifier') {
      categories.push('identifier');
    } else if (classification === 'url') {
      categories.push('url');
    }
    if (categories.length === 0) {
      categories.push('other');
    }
    categoriesByIndex.set(item.index, unique(categories));
  }

  return categoriesByIndex;
}

function buildRawContents({
  decoded,
  sizeDecoded,
  parsedStrings,
  scriptRecords,
}) {
  const items = [];
  const rawByStringIndex = new Map();
  const rawByScriptPath = new Map();
  const rawByCustomSection = new Map();
  const scriptByContentIndex = new Map();
  const usedIds = new Set();

  for (const script of scriptRecords) {
    if (typeof script.contentStringIndex === 'number') {
      scriptByContentIndex.set(script.contentStringIndex, script);
    }
  }

  for (const stringItem of parsedStrings.items) {
    const script = scriptByContentIndex.get(stringItem.index);
    if (!script) {
      continue;
    }
    const id = uniqueRawContentId(
      `content:js-source:${script.path || stringItem.index}`,
      usedIds,
    );
    const raw = {
      id,
      role: 'js-source',
      label: script.path,
      contentBytes: stringItem.contentBytes,
      source: {
        stringIndex: stringItem.index,
        section: 'STRING',
      },
      content: {
        contentBytes: stringItem.contentBytes,
        encoding: 'utf8',
        data: stringItem.value,
      },
    };
    items.push(raw);
    const ref = toRawContentRef(raw);
    rawByStringIndex.set(stringItem.index, ref);
    rawByScriptPath.set(script.path, ref);
  }

  for (const script of scriptRecords) {
    if (rawByScriptPath.has(script.path) || typeof script.content !== 'string') {
      continue;
    }
    const id = uniqueRawContentId(`content:js-source:${script.path}`, usedIds);
    const contentBytes = Buffer.byteLength(script.content, 'utf8');
    const raw = {
      id,
      role: script.kind === 'js-bytecode' ? 'js-bytecode' : 'js-source',
      label: script.path,
      contentBytes,
      source: {
        stringIndex: script.contentStringIndex,
        section: 'STRING',
      },
      content: {
        contentBytes,
        encoding: 'utf8',
        data: script.content,
      },
    };
    items.push(raw);
    const ref = toRawContentRef(raw);
    rawByScriptPath.set(script.path, ref);
    if (typeof script.contentStringIndex === 'number') {
      rawByStringIndex.set(script.contentStringIndex, ref);
    }
  }

  let lepusRootRef;
  const mainThreadScript = objectOrEmpty(decoded['main-thread-script']);
  const lepusBuffer = toByteBuffer(mainThreadScript.lepus_code);
  if (lepusBuffer) {
    const raw = {
      id: uniqueRawContentId('content:lepus-bytecode:root', usedIds),
      role: 'lepus-bytecode',
      label: 'root lepus bytecode',
      contentBytes: lepusBuffer.length,
      range: getSectionRange(sizeDecoded, 'ROOT_LEPUS'),
      source: {
        section: 'ROOT_LEPUS',
      },
      content: {
        contentBytes: lepusBuffer.length,
        encoding: 'base64',
        data: lepusBuffer.toString('base64'),
      },
    };
    items.push(raw);
    lepusRootRef = toRawContentRef(raw);
  }

  const customSections = objectOrEmpty(decoded['custom-sections']);
  for (const [key, value] of Object.entries(customSections)) {
    const encoded = encodeRawData(value);
    const raw = {
      id: uniqueRawContentId(`content:custom-section:${key}`, usedIds),
      role: 'custom-section',
      label: key,
      contentBytes: encoded.contentBytes,
      source: {
        section: 'CUSTOM_SECTIONS',
      },
      content: encoded,
    };
    items.push(raw);
    rawByCustomSection.set(key, toRawContentRef(raw));
  }

  return {
    items,
    rawByStringIndex,
    rawByScriptPath,
    rawByCustomSection,
    lepusRootRef,
  };
}

function buildCssContent(strings) {
  return {
    fragments: contentCollection([]),
    selectors: strings.classified.cssSelector,
    urls: strings.classified.url,
    identifiers: strings.classified.identifier,
  };
}

function buildScripts(scriptRecords, rawContents) {
  const items = scriptRecords
    .filter((script) => rawContents.rawByScriptPath.has(script.path))
    .map((script) => dropUndefined({
      path: script.path,
      pathStringIndex: script.pathStringIndex,
      kind: script.kind === 'js-bytecode' ? 'bytecode' : script.kind || 'source',
      contentBytes: script.contentBytes
        || rawContents.rawByScriptPath.get(script.path).contentBytes,
      rawContentRef: rawContents.rawByScriptPath.get(script.path)
        || rawContents.rawByStringIndex.get(script.contentStringIndex),
    }));
  return {
    files: contentCollection(items),
  };
}

function buildLepus(sizeDecoded, rawContents) {
  const items = [];
  const rootRange = getSectionRange(sizeDecoded, 'ROOT_LEPUS');
  const rootLepus = sizeDecoded.chunks && sizeDecoded.chunks['root-lepus'];
  const rootBytes = rootLepus && typeof rootLepus.payload_bytes === 'number'
    ? rootLepus.payload_bytes
    : rootRange && rootRange.size;
  if (rootBytes) {
    items.push({
      path: 'root',
      contentBytes: rootBytes,
      range: rootRange,
      rawContentRef: rawContents.lepusRootRef,
    });
  }

  const lepusChunks = sizeDecoded.chunks && sizeDecoded.chunks.lepus;
  for (const [name, rangeLike] of Object.entries(objectOrEmpty(lepusChunks))) {
    const range = buildRange(rangeLike);
    items.push({
      path: name,
      contentBytes: range ? range.size : readNumber(rangeLike, 'size', 0),
      range,
    });
  }

  return {
    root: rawContents.lepusRootRef
      ? {
          contentBytes: rawContents.lepusRootRef.contentBytes,
          range: rootRange,
          rawContentRef: rawContents.lepusRootRef,
        }
      : undefined,
    chunks: contentCollection(items.filter((item) => item.path !== 'root')),
  };
}

function buildStrings(
  parsedStrings,
  sizeDecoded,
  categoriesByStringIndex,
  rawContents,
  scriptRecords,
) {
  const stringSection = sizeDecoded.sections && sizeDecoded.sections.STRING;
  const scriptPathIndexMap = new Map();
  const scriptContentIndexMap = new Map();
  for (const script of scriptRecords) {
    if (typeof script.pathStringIndex === 'number') {
      scriptPathIndexMap.set(script.pathStringIndex, script);
    }
    if (typeof script.contentStringIndex === 'number') {
      scriptContentIndexMap.set(script.contentStringIndex, script);
    }
  }

  const classified = {
    url: [],
    cssSelector: [],
    identifier: [],
    other: [],
  };
  const entries = parsedStrings.items.map((item) => {
    const categories = categoriesByStringIndex.get(item.index) || ['other'];
    const rawContentRef = rawContents.rawByStringIndex.get(item.index);
    const role = inferStringRole({
      categories,
      scriptPath: scriptPathIndexMap.get(item.index),
      scriptContent: scriptContentIndexMap.get(item.index),
    });
    const entry = dropUndefined({
      value: rawContentRef ? undefined : item.value,
      contentBytes: item.contentBytes,
      index: item.index,
      source: inferStringSource(role, categories),
      role,
      rawContentRef,
    });
    const ref = toStringRef(entry);
    if (role === 'url') {
      classified.url.push(ref);
    } else if (role === 'css-selector') {
      classified.cssSelector.push(ref);
    } else if (role === 'identifier') {
      classified.identifier.push(ref);
    } else if (role !== 'js-source') {
      classified.other.push(ref);
    }
    return entry;
  });
  const totalContentBytes = readNumber(
    stringSection || {},
    'total_string_bytes',
    entries.reduce((sum, item) => sum + item.contentBytes, 0),
  );
  return {
    count: readNumber(stringSection || {}, 'string_count', entries.length),
    totalContentBytes,
    emptyCount: readNumber(stringSection || {}, 'empty_count', 0),
    entries: contentCollection(entries, totalContentBytes),
    classified: {
      url: contentCollection(classified.url),
      cssSelector: contentCollection(classified.cssSelector),
      identifier: contentCollection(classified.identifier),
      other: contentCollection(classified.other),
    },
  };
}

function buildComponents(decoded) {
  const staticComponents = [];
  const dynamicComponents = [];
  const dynamicDeclarations = [];
  addComponentEntries(staticComponents, decoded.components, 'component');
  addComponentEntries(dynamicComponents, decoded['dynamic-components'], 'dynamic-component');
  addComponentEntries(
    dynamicDeclarations,
    decoded['component-declarations'],
    'component-declaration',
  );
  const source = staticComponents.length || dynamicComponents.length || dynamicDeclarations.length
    ? 'legacy-section'
    : 'unavailable';
  return {
    source,
    staticComponents: itemCollection(staticComponents),
    dynamicComponents: itemCollection(dynamicComponents),
    dynamicDeclarations: itemCollection(dynamicDeclarations),
  };
}

function addComponentEntries(items, source, kind) {
  if (!source) {
    return;
  }
  if (Array.isArray(source)) {
    source.forEach((value, index) => {
      items.push({
        name: value && typeof value.name === 'string' ? value.name : `${kind}-${index}`,
        path: value && typeof value.path === 'string' ? value.path : undefined,
        id: index,
        properties: value && typeof value === 'object' ? value.properties : undefined,
        config: value && typeof value === 'object' ? value.config : undefined,
      });
    });
    return;
  }
  if (typeof source === 'object') {
    Object.entries(source).forEach(([name, value]) => {
      items.push({
        name,
        path: value && typeof value.path === 'string' ? value.path : undefined,
        properties: value && typeof value === 'object' ? value.properties : undefined,
        config: value && typeof value === 'object' ? value.config : undefined,
      });
    });
  }
}

function buildCustomSections(decoded, rawContents) {
  return Object.entries(objectOrEmpty(decoded['custom-sections'])).map(([key, value]) => ({
    key,
    encoding: typeof value === 'string' ? 'utf8' : 'json',
    contentBytes: estimateContentBytes(value),
    rawContentRef: rawContents.rawByCustomSection.get(key),
  }));
}

function parseStringSection(templateBuffer, sizeDecoded) {
  const section = sizeDecoded.sections && sizeDecoded.sections.STRING;
  if (!templateBuffer || !section || typeof section.start !== 'number' || typeof section.end !== 'number') {
    return {parsedFromBinary: false, items: []};
  }

  const count = readNumber(section, 'string_count', 0);
  const expectedContentBytes = readNumber(section, 'total_string_bytes', undefined);
  const expectedLengthFieldBytes = readNumber(section, 'total_length_field_bytes', undefined);
  const sectionSize = readNumber(section, 'size', section.end - section.start);
  const candidates = [];
  const inferredHeaderBytes =
    expectedContentBytes !== undefined && expectedLengthFieldBytes !== undefined
      ? sectionSize - expectedContentBytes - expectedLengthFieldBytes
      : undefined;

  if (Number.isInteger(inferredHeaderBytes) && inferredHeaderBytes >= 0 && inferredHeaderBytes < 64) {
    candidates.push(inferredHeaderBytes);
  }
  for (let offset = 0; offset < 12; offset += 1) {
    if (!candidates.includes(offset)) {
      candidates.push(offset);
    }
  }

  for (const headerBytes of candidates) {
    const parsed = tryParseStringSectionAtOffset({
      templateBuffer,
      section,
      count,
      expectedContentBytes,
      expectedLengthFieldBytes,
      headerBytes,
    });
    if (parsed) {
      return parsed;
    }
  }

  return {parsedFromBinary: false, items: []};
}

function tryParseStringSectionAtOffset({
  templateBuffer,
  section,
  count,
  expectedContentBytes,
  expectedLengthFieldBytes,
  headerBytes,
}) {
  let cursor = section.start + headerBytes;
  const items = [];
  let totalContentBytes = 0;
  let totalLengthFieldBytes = 0;

  try {
    for (let index = 0; index < count; index += 1) {
      const lengthResult = readUnsignedVariableLengthInteger(
        templateBuffer,
        cursor,
        section.end,
      );
      cursor = lengthResult.next;
      totalLengthFieldBytes += lengthResult.bytes;
      const end = cursor + lengthResult.value;
      if (end > section.end) {
        return undefined;
      }
      items.push({
        index,
        value: templateBuffer.toString('utf8', cursor, end),
        contentBytes: lengthResult.value,
      });
      totalContentBytes += lengthResult.value;
      cursor = end;
    }
  } catch (_) {
    return undefined;
  }

  if (cursor !== section.end) {
    return undefined;
  }
  if (expectedContentBytes !== undefined && totalContentBytes !== expectedContentBytes) {
    return undefined;
  }
  if (
    expectedLengthFieldBytes !== undefined
    && totalLengthFieldBytes !== expectedLengthFieldBytes
  ) {
    return undefined;
  }

  return {
    parsedFromBinary: true,
    headerBytes,
    items,
  };
}

function readUnsignedVariableLengthInteger(buffer, start, limit) {
  let value = 0;
  let shift = 0;
  let cursor = start;

  while (cursor < limit) {
    const byte = buffer[cursor];
    cursor += 1;
    value += (byte & 0x7f) * Math.pow(2, shift);
    if ((byte & 0x80) === 0) {
      return {
        value,
        bytes: cursor - start,
        next: cursor,
      };
    }
    shift += 7;
  }

  throw new Error('Unexpected EOF while reading variable-length integer');
}

function buildClassificationBuckets(sizeDecoded) {
  const stringUsage = sizeDecoded.chunks && sizeDecoded.chunks['string-usage'];
  const byType = stringUsage && objectOrEmpty(stringUsage.other_all_by_type);
  const buckets = new Map();

  for (const [type, values] of Object.entries(byType || {})) {
    if (!Array.isArray(values)) {
      continue;
    }
    for (const value of values) {
      if (!value || typeof value.name !== 'string' || typeof value.len !== 'number') {
        continue;
      }
      const key = stringBucketKey(value.name, value.len);
      const bucket = buckets.get(key) || [];
      bucket.push(type);
      buckets.set(key, bucket);
    }
  }

  return buckets;
}

function consumeClassification(buckets, value, contentBytes) {
  const bucket = buckets.get(stringBucketKey(value, contentBytes));
  if (!bucket || bucket.length === 0) {
    return undefined;
  }
  return bucket.shift();
}

function stringBucketKey(value, contentBytes) {
  return `${contentBytes}\u0000${value}`;
}

function inferStringRole({categories, scriptPath, scriptContent}) {
  if (scriptContent) {
    return 'js-source';
  }
  if (scriptPath) {
    return 'path';
  }
  if (categories.includes('css-selector')) {
    return 'css-selector';
  }
  if (categories.includes('identifier')) {
    return 'identifier';
  }
  if (categories.includes('url')) {
    return 'url';
  }
  return 'other';
}

function inferStringSource(role, categories) {
  if (role === 'js-source' || role === 'path') {
    return 'js';
  }
  if (role === 'css-selector' || role === 'identifier' || role === 'url') {
    return 'css';
  }
  if (categories && !categories.includes('other')) {
    return 'heuristic';
  }
  return 'string-table';
}

function getJsSizeFiles(sizeDecoded) {
  const jsChunk = sizeDecoded.chunks && sizeDecoded.chunks.js;
  return Array.isArray(jsChunk && jsChunk.files) ? jsChunk.files : [];
}

function getSectionRange(sizeDecoded, name) {
  const section = sizeDecoded.sections && sizeDecoded.sections[name];
  return buildRange(section);
}

function buildRange(section) {
  if (!section || typeof section.start !== 'number' || typeof section.end !== 'number') {
    return undefined;
  }
  return {
    start: section.start,
    end: section.end,
    size: typeof section.size === 'number' ? section.size : section.end - section.start,
  };
}

function sizeCollection(items, totalBytes) {
  return {
    totalCount: items.length,
    totalBytes: totalBytes === undefined
      ? items.reduce((sum, item) => sum + (item.bytes || 0), 0)
      : totalBytes,
    items: dropUndefined(items),
  };
}

function contentCollection(items, totalContentBytes) {
  return {
    totalCount: items.length,
    totalContentBytes: totalContentBytes === undefined
      ? items.reduce((sum, item) => sum + (item.contentBytes || 0), 0)
      : totalContentBytes,
    items: dropUndefined(items),
  };
}

function itemCollection(items) {
  return {
    totalCount: items.length,
    items: dropUndefined(items),
  };
}

function encodeRawData(value) {
  const buffer = toByteBuffer(value);
  if (buffer) {
    return {
      contentBytes: buffer.length,
      encoding: 'base64',
      data: buffer.toString('base64'),
    };
  }
  if (typeof value === 'string') {
    return {
      contentBytes: Buffer.byteLength(value, 'utf8'),
      encoding: 'utf8',
      data: value,
    };
  }
  return {
    contentBytes: estimateContentBytes(value),
    encoding: 'json',
    data: value,
  };
}

function toBuffer(value) {
  if (Buffer.isBuffer(value)) {
    return value;
  }
  if (value instanceof Uint8Array) {
    return Buffer.from(value);
  }
  if (Array.isArray(value)) {
    return Buffer.from(value);
  }
  return Buffer.alloc(0);
}

function toByteBuffer(value) {
  if (Buffer.isBuffer(value)) {
    return value;
  }
  if (value instanceof Uint8Array) {
    return Buffer.from(value);
  }
  if (Array.isArray(value)) {
    return Buffer.from(value);
  }
  return undefined;
}

function normalizeArray(value) {
  if (Array.isArray(value)) {
    return value;
  }
  if (value && typeof value === 'object') {
    return Object.values(value);
  }
  return [];
}

function objectOrEmpty(value) {
  return value && typeof value === 'object' && !Array.isArray(value) ? value : {};
}

function readNumber(object, key, fallback) {
  return object && typeof object[key] === 'number' ? object[key] : fallback;
}

function estimateContentBytes(value) {
  if (typeof value === 'string') {
    return Buffer.byteLength(value, 'utf8');
  }
  const buffer = toByteBuffer(value);
  if (buffer) {
    return buffer.length;
  }
  const json = JSON.stringify(value);
  return Buffer.byteLength(json === undefined ? String(value) : json, 'utf8');
}

function unique(values) {
  return Array.from(new Set(values));
}

function uniqueRawContentId(base, usedIds) {
  const normalized = String(base || 'content:unknown')
    .replace(/\s+/g, '-')
    .slice(0, 240);
  let id = normalized || 'content:unknown';
  let index = 2;
  while (usedIds.has(id)) {
    id = `${normalized}#${index}`;
    index += 1;
  }
  usedIds.add(id);
  return id;
}

function toRawContentRef(raw) {
  return {
    id: raw.id,
    contentBytes: raw.contentBytes,
    encoding: raw.content.encoding,
    label: raw.label,
  };
}

function toStringRef(item) {
  const ref = {
    index: item.index,
    contentBytes: item.contentBytes,
    role: item.role,
  };
  if (item.rawContentRef) {
    ref.rawContentRef = item.rawContentRef;
  }
  return ref;
}

function dropUndefined(value) {
  if (Array.isArray(value)) {
    return value.map(dropUndefined);
  }
  if (!value || typeof value !== 'object') {
    return value;
  }
  const output = {};
  for (const [key, child] of Object.entries(value)) {
    if (child !== undefined) {
      output[key] = dropUndefined(child);
    }
  }
  return output;
}

function normalizeErrorStatus(error) {
  const message = error && error.message ? error.message : String(error || 'unknown');
  if (/Cannot find module|invalid ELF|wrong ELF|incompatible architecture|mach-o/i.test(message)) {
    return 'module-load-failed';
  }
  if (/Invalid Buffer|Not Valid Buffer/i.test(message)) {
    return 'invalid-buffer';
  }
  if (/template binary info/i.test(message)) {
    return 'template-size-decode-failed';
  }
  if (/decode/i.test(message)) {
    return 'decode-failed';
  }
  const normalized = message
    .replace(/^Error:\s*/i, '')
    .replace(/\s+/g, '-')
    .replace(/[^A-Za-z0-9_.:-]+/g, '-')
    .replace(/^-+|-+$/g, '')
    .slice(0, 120);
  return normalized || 'unknown';
}

function isNapiLoadError(error) {
  const message = error && error.message ? error.message : String(error || '');
  return /Cannot find module|invalid ELF|wrong ELF|incompatible architecture|mach-o/i.test(message);
}

module.exports = {
  buildTemplateDecodeDetail,
  buildTemplateDecodeDetailError,
  createTemplateDecodeDetailApi,
};
