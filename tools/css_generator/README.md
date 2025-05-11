# CSS Generator

This package serves as the source of truth for all CSS APIs defined in the Lynx platform. It's used to generate C++ code in the engine for Lynx contributors, as well as Types consumed by the Lynx frontend developers.

## Table of Contents

- [CSS Generator](#css-generator)
  - [Table of Contents](#table-of-contents)
  - [CSS Property Definition](#css-property-definition)
    - [Property vs Attribute](#property-vs-attribute)
    - [Vendor Prefix](#vendor-prefix)
    - [Adding a New Property](#adding-a-new-property)
  - [TypeScript Type Generation](#typescript-type-generation)
    - [Type Generation Rules](#type-generation-rules)
    - [Build Process](#build-process)
    - [Usage](#usage)
  - [Schema](#schema)
  - [Tutorial: Implementing a CSS Property](#tutorial-implementing-a-css-property)

## CSS Property Definition

### Property vs Attribute

Properties are features of an element that you can change the value of, to make it styled differently (e.g. `color`, `font-size`). If the feature is not related to element's style, or it is strongly coupled with a specific element type (e.g. `initial-scroll-offset` on `<scroll-view>`), it is recommended to add it as an attribute. Most times, the style property should have a prototype in the W3C CSS specification.

If the implementation of the property is non-standardized, or it is under experimental, it should be a prefixed property.

### Vendor Prefix

**-x-** is the vendor prefix used by Lynx to distinguish CSS properties that are specific to Lynx and not part of the Web standard. They provide Web developers with some layout and styling capabilities specific to Lynx, such as:

```css
.linear {
    /* similar to when Grid layout was -ms-grid, -webkit-grid: */
    display: -x-linear;
    display: linear; /* should also work */
    -x-linear-orientation: horizontal;
}
```

### Adding a New Property

Once you have decided to add a new property, you can follow the steps below:

1. Add a definition file under the [`css_defines`](./css_defines) directory. The file name should begin with an incremental number indicating the ID of the property. We use the ID at runtime to map the property's parser, getter and setter functions to avoid string comparison. So the ID should be unique, and should not be modified after it is added. The consistency of the ID will be checked according to [`property_index.json`](./property_index.json). The new ID and property name will be automatically added to the index file by [`css_parser_generator.py`](./css_parser_generator.py).

2. Execute `python tools/css_generator/css_parser_generator.py`

3. Implement the code in generated files.  
   There are a couple of functions that cannot be automatically implemented by the script. According to the complexity of the property's value, you may need to implement the following functions:
   1. [Parser](../../core/renderer/css/parser/background_box_handler.h): The parser function is used to parse the value of the property.
   2. [ComputedCSSStyle](../../core/renderer/css/computed_css_style.cc): The setter and getter functions should be manually implemented according to the output of parser. If the property will be consumed by the platform UI layer, you have to add it to the macro `FOREACH_PLATFORM_PROPERTY` in the header file.

## TypeScript Type Generation

### Type Generation Rules

The type definitions in `js_libraries/types/types/common/csstype.d.ts` are automatically generated from CSS define files in the `css_defines` directory. The generation process follows these rules:

1. For enum types (properties with `"type": "enum"`):
   - Uses the `values` array to generate a union type of string literals
   - Example: `display?: 'none' | 'flex' | 'grid' | 'linear' | 'relative' | 'block' | 'auto';`

2. For properties with keywords:
   - Uses the `keywords` array to generate a union type of string literals
   - Adds `(string & {})` for open-ended string types
   - Example: `animationTimingFunction?: 'linear' | 'ease-in' | 'ease-out' | ... | (string & {});`

3. For other types:
   - Uses `string` type
   - Example: `color?: string;`

### Build Process

The build process generates TypeScript type definitions and copies them to the types package:

1. `npm run gen:types` - Generates types in the `dist/` directory (gitignored)
2. `npm run copy:types` - Copies generated types to `js_libraries/types/types/common/`
3. `npm run build` - Runs both steps in sequence

Note: The `dist/` directory is gitignored because it's an intermediate build artifact. The final types are always in `js_libraries/types/types/common/csstype.d.ts`.

### Usage

```bash
# Validate all CSS defines
npm run validate

# Test validation with invalid CSS defines
npm run test

# Generate types and copy to types package
npm run build

# Generate types in dist/ for testing (without copying)
npm run gen:types
```

## Schema

The schema in `css_define_json_schema/css_define_with_doc.schema.json` defines the structure of CSS define files and is used for both validation and type generation.

## Tutorial: Implementing a CSS Property

The following example demonstrates how to add a new CSS property to the system:

```json
{
    "name": "test",
    "id": 213,
    "type": "complex",
    "default_value": "auto",
    "version": "1.0",
    "author": "wangerpao",
    "consumption_status": "layout-only",
    "desc": "left offset",
    "keywords": ["foo", "bar", "foobar"],
    "values": [
        {
            "value": "test-value",
            "version": "1.0"
        }
    ],
    "links": [
        {
            "url": "<reference docs>",
            "desc": "description of the reference"
        },
        {
            "url": "123"
        }
    ],
    "note": [
        {
            "literal": "this is a note",
            "level": "tip"
        },
        {
            "literal": "This is a warning for user of this property.",
            "level": "warning"
        }
    ],
    "__compat": {
        "description": "<Description of this compat data entry>",
        "lynx_path": "<path to api reference in lynx website> docs/zh/api/css/properties/left)",
        "mdn_url": "<path to mdn definition> https://developer.mozilla.org/zh-CN/docs/Web/CSS/left",
        "spec_url": ["<path to w3c specification file>"],
        "status": {
            "deprecated": false,
            "experimental": false
        },
        "support": {
            "android": {
                "version_added": "1.0"
            },
            "ios": {
                "version_added": "1.0"
            }
        }
    }
}
```

1. Create a new file named `999-test.json` and copy the above JSON into it.

2. Run `css_parser_generator.py` to generate the necessary files.

   The `css_property_id.h` file will be autogenerated under `core/renderer/css`. You will find your newly added property `test` appended to the end of the macro `FOREACH_ALL_PROPERTY`, as well as the property enum class `CSSPropertyID`.

3. Implement the parser for the property value:
   - If the type is one of `color`, `length`, `time`, `enum`, `border-width`, `border-style`, `bool`, `timing-function` or `animation-property`, the parser will be auto-generated
   - Otherwise, manually add it to `core/renderer/css/parser` directory

   Create two new files, `test_handler.h` and `test_handler.cc` under the directory:

   ```cpp
   #include "core/renderer/css/parser/handler_defines.h"

   namespace lynx {
   namespace tasm {
   namespace TestHandler {

   HANDLER_REGISTER_DECLARE();

   }  // namespace TestHandler
   }  // namespace tasm
   }  // namespace lynx
   ```

   Register your parsing functions into the 'array' to your property id, and implement the parser functions, which converts the input string value into a CSSValue object and put it into the 'output' map. It is recommended that you implement the parser based on CSSStringParser, as it already has some basic tokenizers and lexical checks.

   ```cpp
   #include "core/renderer/css/parser/test_handler.h"

   #include <string>
   #include <utility>

   #include "base/include/debug/lynx_assert.h"
   #include "core/renderer/css/parser/css_string_parser.h"
   #include "core/renderer/css/unit_handler.h"
   #include "core/renderer/tasm/config.h"

   namespace lynx {
   namespace tasm {
   namespace TestHandler {

   HANDLER_IMPL() {
     CSS_HANDLER_FAIL_IF_NOT(input.IsString(), configs.enable_css_strict_mode,
                            TYPE_MUST_BE, CSSProperty::GetPropertyNameCStr(key),
                            STRING_TYPE)

     CSSStringParser parser = CSSStringParser::FromLepusString(input, configs);
     parser.SetIsLegacyParser(configs.enable_legacy_parser);
     output[kPropertyIDTest] = parser.ParseTest();
     return true;
   }

   HANDLER_REGISTER_IMPL() {
     array[kPropertyIDTest] = &Handle;
   }

   }  // namespace TestHandler
   }  // namespace tasm
   }  // namespace lynx
   ```

4. Register the parser by adding your customized handler to `core/renderer/css/parser/unit_handler.cc`:

   ```cpp
   UnitHandler::UnitHandler() {
     TestHandler::Register(interceptors_);
   }
   ```

5. Implement setter & getter for ComputedCSSStyle:

   After converting the raw string into a CSSValue, in a ComputedCSSValue, it should be a data struct based on primitive types. Implement the value conversion from CSSValue, and calculate it if your value is context related (e.g. length value with `sp` unit is related to root element's `font-size`).

   If your property will be consumed by platform layer, add it to macro `FOREACH_PLATFORM_PROPERTY`. Then implement your function in `prop_bundle_style_writer`, to enable the runtime to put your computed value into prop bundle, and send it to platform layer.

   ```cpp
   #define FOREACH_PLATFORM_PROPERTY(V) \
     V(Test)

   bool ComputedCSSValue::SetTest(const tasm::CSSValue& value, bool reset);

   // In prop_bundle_style_writer
   static void TestWriterFunc(PropBundle* bundle, CSSPropertyID id,
                            starlight::ComputedCSSStyle* style);

   static constexpr std::array<WriterFunc, kPropertyEnd> kWriter = [] {
     std::array<WriterFunc, kPropertyEnd> writer = {nullptr};
     for (CSSPropertyID id : kPlatformIDs) {
       writer[id] = &DefaultWriterFunc;
     }
     writer[kPropertyIDTest] = &TestWriterFunc;
     return writer;
   }();
   ```
