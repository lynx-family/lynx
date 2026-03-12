#!/usr/bin/env node
"use strict";

const fs = require("fs");
const path = require("path");
const { decode_napi, decode_wasm, encode, supportNapi } = require("./index");

function printHelp() {
  console.log(
    [
      "Usage:",
      "  lynx-tasm encode -i <input.json> -o <output.tasm>",
      "  lynx-tasm decode -i <input.tasm> -o <output.json>",
      "",
      "Options:",
      "  -i, --input   Input file path (required)",
      "  -o, --output  Output file path (required)",
      "  -h, --help    Show help",
      "",
      "Examples:",
      "  lynx-tasm encode -i ./template.json -o ./template.tasm",
      "  lynx-tasm decode -i ./template.tasm -o ./template.json",
    ].join("\n")
  );
}

function parseArgs(argv) {
  let input = "";
  let output = "";
  let help = false;
  const positional = [];

  for (let i = 0; i < argv.length; i++) {
    const arg = argv[i];
    if (arg === "-i" || arg === "--input") {
      input = argv[++i] || "";
    } else if (arg === "-o" || arg === "--output") {
      output = argv[++i] || "";
    } else if (arg === "-h" || arg === "--help") {
      help = true;
    } else {
      positional.push(arg);
    }
  }

  return { input, output, help, positional };
}

async function main() {
  const { input, output, help, positional } = parseArgs(process.argv.slice(2));

  if (help || positional.length === 0) {
    printHelp();
    process.exit(help ? 0 : 1);
  }

  const command = positional[0];
  if (command !== "decode" && command !== "encode") {
    console.error(`Unknown command: ${command}`);
    printHelp();
    process.exit(1);
  }

  if (!input) {
    console.error("Missing required argument: --input");
    printHelp();
    process.exit(1);
  }
  if (!output) {
    console.error("Missing required argument: --output");
    printHelp();
    process.exit(1);
  }

  const inputPath = path.resolve(process.cwd(), input);
  const outputPath = path.resolve(process.cwd(), output);
  if (command === "decode") {
    const inputBuffer = fs.readFileSync(inputPath);
    let result;
    if (supportNapi()) {
      try {
        result = await decode_napi(inputBuffer);
      } catch (err) {
        const isMissingNapiBinary =
          err &&
          err.code === "MODULE_NOT_FOUND" &&
          String(err.message || "").includes("lepus.node");
        if (!isMissingNapiBinary) {
          throw err;
        }
        result = await decode_wasm(inputBuffer);
      }
    } else {
      result = await decode_wasm(inputBuffer);
    }
    const content = JSON.stringify(result, null, 2);
    fs.writeFileSync(outputPath, `${content}\n`, "utf8");
    console.error(`Decode succeeded. Output written to: ${outputPath}`);
    return;
  }

  const inputJson = JSON.parse(fs.readFileSync(inputPath, "utf8"));
  const result = await encode(inputJson);
  fs.writeFileSync(outputPath, result.buffer);
  console.error(`Encode succeeded. Output written to: ${outputPath}`);
}

main().catch((err) => {
  console.error(err && err.message ? err.message : String(err));
  process.exit(1);
});
