// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License, Version 2.0 (see LICENSE).
// Fixture conversion only. Do not derive expected values from a producer.
constexpr char kComposeDrawRecordingRegistrySha256[] = "f0789374f0e1e4c25af25f0ea36407373381bccf24b0131b0529af8c6971efe5";
struct ComposeDrawRecordingFixture {
  const char* family;
  const char* sha256;
  const char* json;
};
constexpr ComposeDrawRecordingFixture kComposeDrawRecordingFixtures[] = {
  {"recording-v1", "d57a50770ad48b1ad0fffe187908e04159763e5e4542f41513a651a71b1a32c0", R"fixture({
  "family": "recording-v1",
  "cases": [
    {
      "id": "rect",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 2, 4, 6, 8]],
        [[[1, 2148606515], [0.5, 0, true, null], [1], []]],
        [],
        ["paint.blend.SrcOver", "paint.solid", "paint.srgb", "path.rect"],
        []
      ],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      },
      "probes": [
        [0, 3, 5, true],
        [0, 1, 5, false],
        [0, 8.1, 5, false]
      ]
    },
    {
      "id": "elliptical-normalization",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[2, 0, 0, 12, 8, 12, 4, 12, 8, 4, 8, 4, 4]],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        ["paint.blend.SrcOver", "paint.solid", "paint.srgb", "path.roundrect"],
        []
      ],
      "expected": {
        "normalizedRadii": [[6, 2, 6, 4, 2, 4, 2, 2]],
        "paths": [[2, 0, 0, 12, 8, 12, 4, 12, 8, 4, 8, 4, 4]],
        "currentPolyfill": "unsupported"
      },
      "probes": [
        [0, 0.1, 0.1, false],
        [0, 6, 0.1, true],
        [0, 11.9, 0.1, false],
        [0, 6, 4, true]
      ]
    },
    {
      "id": "translucent-ring",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 2, 0]],
        [
          [2, 0, 0, 40, 20, 4, 4, 4, 4, 4, 4, 4, 4],
          [2, 2, 2, 36, 16, 2, 2, 2, 2, 2, 2, 2, 2],
          [3, 0, 1]
        ],
        [[[1, 2148606515], [0.5, 0, true, null], [1], []]],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.border-clip",
          "path.roundrect"
        ],
        []
      ],
      "expected": {
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "currentPolyfill": "supported"
      },
      "probes": [
        [2, 1, 10, true],
        [2, 20, 10, false],
        [2, 41, 10, false]
      ]
    },
    {
      "id": "general-difference",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 2, 0]],
        [
          [1, 0, 0, 10, 10],
          [1, 8, 0, 10, 10],
          [3, 0, 1]
        ],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.border-clip",
          "path.rect"
        ],
        []
      ],
      "expected": { "currentPolyfill": "unsupported" },
      "probes": [
        [2, 5, 5, true],
        [2, 9, 5, false],
        [2, 15, 5, false]
      ]
    },
    {
      "id": "collapsed-hole",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 1, 0]],
        [
          [1, 0, 0, 4, 4],
          [3, 0, -1]
        ],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.border-clip",
          "path.rect"
        ],
        []
      ],
      "expected": { "currentPolyfill": "supported" },
      "probes": [[1, 2, 2, true]]
    },
    {
      "id": "zero-area-stroke",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 0, 0, 0, 10]],
        [
          [
            [1, 2148606515],
            [0, 0, true, null],
            [2, 2, 1, 2, 4, [2, 4], -1, null],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "paint.stroke",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "operations": ["DrawPath"],
        "styles": [[2, 2, 1, 2, 4, [2, 4], -1, null]],
        "caps": ["Round"],
        "joins": ["Bevel"],
        "sourceAlpha": [[128]],
        "drawAlpha": [0],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "hairline",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [
          [
            4,
            1,
            [
              [1, 0, 0],
              [2, 10, 0]
            ]
          ]
        ],
        [
          [
            [1, 4294967295],
            [1, 0, true, null],
            [2, 0, 0, 0, 4, [], 0, null],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "paint.stroke",
          "path.verbs"
        ],
        []
      ],
      "expected": {
        "fillRules": ["EvenOdd"],
        "styles": [[2, 0, 0, 0, 4, [], 0, null]],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "exact-verbs",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [
          [
            4,
            0,
            [
              [1, 0, 0],
              [2, 2, 0],
              [3, 3, 0, 3, 1],
              [4, 3, 2, 2, 3, 1, 3],
              [5, 0, 3, 0, 2, 0.5],
              [6],
              [1, 1, 1],
              [2, 2, 1],
              [6]
            ]
          ]
        ],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        ["paint.blend.SrcOver", "paint.solid", "paint.srgb", "path.verbs"],
        []
      ],
      "expected": {
        "fillRules": ["NonZero"],
        "verbs": [
          [
            "MoveTo",
            "LineTo",
            "QuadTo",
            "CubicTo",
            "ConicTo",
            "Close",
            "MoveTo",
            "LineTo",
            "Close"
          ]
        ],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "balanced-state-higher-minor",
      "wire": [
        "lynx.draw.recording",
        1,
        99,
        [[2], [4, 1, 0, 0, 1, 0, 0], [3]],
        [],
        [],
        [],
        ["drawing.state"],
        [["diagnostic", ["ok", true, 1]]]
      ],
      "expected": {
        "operations": ["Save", "Concat", "Restore"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "empty-clip",
      "mode": "op18",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[5, 0, 0, true]],
        [[1, 0, 0, 0, 10]],
        [],
        [],
        ["drawing.clip", "path.rect"],
        []
      ],
      "expected": {
        "operations": ["ClipPath"],
        "currentPolyfill": "supported"
      },
      "probes": [[0, 0, 5, false]]
    },
    {
      "id": "callback-empty-clip",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[5, 0, 0, true]],
        [[1, 0, 0, 0, 10]],
        [],
        [],
        ["drawing.clip", "path.rect"],
        []
      ],
      "expected": {
        "commands": [[5, 0, 0, true]],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "linear-hard-stops",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 0, 0, 20, 10]],
        [
          [
            [
              2,
              40,
              0,
              -10,
              0,
              [2148606515, 4294967295, 4278190335],
              [0, 0.5, 0.5],
              0,
              [1, 0, 0, 1, 0, 0]
            ],
            [0.5, 0, true, null],
            [1],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.linear",
          "paint.srgb",
          "paint.tile.Clamp",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "tileModes": ["Clamp"],
        "sourceAlpha": [[128, 255, 255]],
        "effectiveArgb": [[1074864691, 2164260863, 2147483903]],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "radial",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 0, 0, 10, 10]],
        [
          [
            [
              3,
              2,
              3,
              4,
              [4294967295, 4278190080],
              [0, 1],
              2,
              [1, 0, 0, 1, 0, 0]
            ],
            [1, 2, true, null],
            [1],
            []
          ]
        ],
        [],
        [
          "paint.blend.DstIn",
          "paint.radial",
          "paint.srgb",
          "paint.tile.Mirror",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "sourceKinds": ["Radial"],
        "tileModes": ["Mirror"],
        "blendModes": ["DstIn"],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "sweep",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 0, 0, 10, 10]],
        [
          [
            [4, 2, 3, [4294967295, 4278190080], [0, 1], [1, 0, 0, 1, 2, 3]],
            [1, 1, true, [1, 4294967295, 1]],
            [1],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcIn",
          "paint.color-filter",
          "paint.srgb",
          "paint.sweep",
          "paint.transform",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "sourceKinds": ["Sweep"],
        "blendModes": ["SrcIn"],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "content-zero",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[10]],
        [],
        [],
        [],
        ["drawing.content"],
        []
      ],
      "expected": {
        "contentMode": true,
        "contentCalls": 0,
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "content-repeat",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[10], [8], [8]],
        [],
        [],
        [],
        ["drawing.content"],
        []
      ],
      "expected": {
        "contentMode": true,
        "contentCalls": 2,
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "layer",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[6, [0, 0, 20, 10], 0], [7]],
        [],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        ["drawing.layer", "paint.blend.SrcOver", "paint.solid", "paint.srgb"],
        []
      ],
      "expected": {
        "operations": ["BeginLayer", "EndLayer"],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "shadow",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[9, 0, 0, -2, 3, 4, -1, 0, 1]],
        [[2, 0, 0, 40, 20, 2, 2, 3, 3, 4, 4, 5, 5]],
        [[[1, 2148606515], [1, 0, true, null], [1], []]],
        [],
        [
          "drawing.shadow",
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.roundrect"
        ],
        []
      ],
      "expected": {
        "commands": [[9, 0, 0, -2, 3, 4, -1, 0, 1]],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "asymmetric-rtl",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[2, 0, 0, 20, 10, 4, 4, 1, 1, 2, 2, 3, 3]],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        ["paint.blend.SrcOver", "paint.solid", "paint.srgb", "path.roundrect"],
        []
      ],
      "expected": {
        "normalizedRadii": [[4, 4, 1, 1, 2, 2, 3, 3]],
        "currentPolyfill": "supported"
      },
      "probes": [
        [0, 0.5, 0.5, false],
        [0, 19.5, 0.5, true]
      ]
    },
    {
      "id": "normalized-circle",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[2, 0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        ["paint.blend.SrcOver", "paint.solid", "paint.srgb", "path.roundrect"],
        []
      ],
      "expected": {
        "normalizedRadii": [[5, 5, 5, 5, 5, 5, 5, 5]],
        "currentPolyfill": "supported"
      },
      "probes": [
        [0, 0.25, 0.25, false],
        [0, 0, 5, true],
        [0, 5, 5, true],
        [0, -0.25, 5, false]
      ]
    },
    {
      "id": "translated-uniform-hole",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 2, 0]],
        [
          [1, 1000000, 0, 40, 20],
          [1, 1000002, 2, 36, 16],
          [3, 0, 1]
        ],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.border-clip",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "operations": ["DrawPath"],
        "currentPolyfill": "supported"
      },
      "probes": [
        [2, 1000001, 10, true],
        [2, 1000020, 10, false]
      ]
    },
    {
      "id": "matrix-order",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [
          [2],
          [4, 1, 0, 0, 1, 10, 20],
          [4, 2, 0, 0, 3, 0, 0],
          [1, 0, 0],
          [3],
          [1, 0, 0]
        ],
        [[1, 0, 0, 4, 4]],
        [[[1, 4294967295], [1, 0, true, null], [1], []]],
        [],
        [
          "drawing.state",
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "drawMatrices": [
          [2, 0, 0, 3, 10, 20],
          [1, 0, 0, 1, 0, 0]
        ],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "linear-forward-scope",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 0, 0]],
        [[1, 2, 4, 6, 8]],
        [
          [
            [
              2,
              0,
              0,
              0,
              10,
              [2148606515, 4294967295],
              [0, 1],
              0,
              [1, 0, 0, 1, 0, 0]
            ],
            [0.5, 0, true, null],
            [1],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.linear",
          "paint.srgb",
          "paint.tile.Clamp",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "sources": [
          [
            2,
            0,
            0,
            0,
            10,
            [2148606515, 4294967295],
            [0, 1],
            0,
            [1, 0, 0, 1, 0, 0]
          ]
        ],
        "sourceAlpha": [[128, 255]],
        "drawAlpha": [0.5],
        "currentPolyfill": "supported"
      },
      "probes": [
        [0, 3, 5, true],
        [0, 1, 5, false],
        [0, 8.1, 5, false]
      ]
    },
    {
      "id": "linear-offbounds-hole",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[1, 2, 0]],
        [
          [1, 0, 0, 40, 20],
          [1, 2, 2, 36, 16],
          [3, 0, 1]
        ],
        [
          [
            [
              2,
              -10,
              0,
              50,
              0,
              [2148606515, 4294901760, 4278255360, 4278190335],
              [0, 0.25, 0.25, 1],
              0,
              [1, 0, 0, 1, 0, 0]
            ],
            [0.5, 0, true, null],
            [1],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.linear",
          "paint.srgb",
          "paint.tile.Clamp",
          "path.border-clip",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "sources": [
          [
            2,
            -10,
            0,
            50,
            0,
            [2148606515, 4294901760, 4278255360, 4278190335],
            [0, 0.25, 0.25, 1],
            0,
            [1, 0, 0, 1, 0, 0]
          ]
        ],
        "sourceAlpha": [[128, 255, 255, 255]],
        "drawAlpha": [0.5],
        "currentPolyfill": "supported"
      },
      "probes": [
        [2, 1, 10, true],
        [2, 20, 10, false],
        [2, 41, 10, false],
        [2, 3, 3, false],
        [2, 1, 1, true]
      ]
    },
    {
      "id": "balanced-state",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [[2], [4, 1, 0, 0, 1, 0, 0], [3]],
        [],
        [],
        [],
        ["drawing.state"],
        [["diagnostic", ["ok", true, 1]]]
      ],
      "expected": {
        "operations": ["Save", "Concat", "Restore"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "stroke-tail",
      "wire": [
        "lynx.draw.recording",
        1,
        0,
        [
          [1, 0, 0],
          [1, 1, 1]
        ],
        [
          [1, 2, 4, 6, 8],
          [1, 0, 0, 0, 10]
        ],
        [
          [[1, 2148606515], [0.5, 0, true, null], [1], []],
          [
            [1, 2148606515],
            [0, 0, true, null],
            [2, 0, 0, 0, 4, [], 0, null],
            []
          ]
        ],
        [],
        [
          "paint.blend.SrcOver",
          "paint.solid",
          "paint.srgb",
          "paint.stroke",
          "path.rect"
        ],
        []
      ],
      "expected": {
        "operations": ["DrawPath", "DrawPath"],
        "sourceAlpha": [[128], [128]],
        "drawAlpha": [0.5, 0],
        "currentPolyfill": "unsupported"
      },
      "probes": [
        [0, 3, 5, true],
        [0, 1, 5, false],
        [0, 8.1, 5, false]
      ]
    }
  ],
  "errors": [
    {
      "id": "feature-list-type",
      "base": "rect",
      "changes": [[[7], "path.rect"]],
      "error": "type"
    },
    {
      "id": "feature-name-type",
      "base": "rect",
      "changes": [[[7], [42]]],
      "error": "type"
    },
    {
      "id": "finite-radius-sum-overflow",
      "base": "normalized-circle",
      "changes": [
        [
          [4, 0],
          [
            2,
            0,
            0,
            10,
            10,
            1e308,
            1e308,
            1e308,
            1e308,
            1e308,
            1e308,
            1e308,
            1e308
          ]
        ]
      ],
      "error": "overflow"
    },
    {
      "id": "unknown-major",
      "base": "rect",
      "changes": [[[1], 2]],
      "error": "version"
    },
    {
      "id": "unknown-discriminator",
      "base": "rect",
      "changes": [[[0], "lynx.draw.other"]],
      "error": "version"
    },
    {
      "id": "unknown-tag",
      "base": "rect",
      "changes": [[[3, 0, 0], 11]],
      "error": "tag"
    },
    {
      "id": "unknown-enum",
      "base": "rect",
      "changes": [[[5, 0, 1, 1], 3]],
      "error": "enum"
    },
    {
      "id": "duplicate-metadata",
      "base": "rect",
      "changes": [
        [
          [8],
          [
            ["x", 1],
            ["x", 2]
          ]
        ]
      ],
      "error": "metadata"
    },
    {
      "id": "bounds-ref",
      "base": "rect",
      "changes": [[[3, 0, 1], 1]],
      "error": "reference"
    },
    {
      "id": "negative-ref",
      "base": "rect",
      "changes": [[[3, 0, 1], -1]],
      "error": "reference"
    },
    {
      "id": "fractional-ref",
      "base": "rect",
      "changes": [[[3, 0, 1], 0.5]],
      "error": "integer"
    },
    {
      "id": "forward-ref",
      "base": "translucent-ring",
      "changes": [
        [
          [4, 0],
          [3, 1, -1]
        ]
      ],
      "error": "reference"
    },
    {
      "id": "path-cycle",
      "base": "translucent-ring",
      "changes": [
        [
          [4, 2],
          [3, 2, -1]
        ]
      ],
      "error": "reference"
    },
    {
      "id": "bad-argb",
      "base": "rect",
      "changes": [[[5, 0, 0, 1], 4294967296]],
      "error": "range"
    },
    {
      "id": "negative-width",
      "base": "rect",
      "changes": [[[4, 0, 3], -1]],
      "error": "range"
    },
    {
      "id": "bool-is-not-id",
      "base": "rect",
      "changes": [[[3, 0, 1], true]],
      "error": "integer"
    },
    {
      "id": "paint-arity",
      "base": "rect",
      "changes": [
        [
          [5, 0, 1],
          [1, 0, true]
        ]
      ],
      "error": "arity"
    },
    {
      "id": "stroke-borderclip",
      "base": "translucent-ring",
      "changes": [
        [
          [5, 0, 2],
          [2, 1, 0, 0, 4, [], 0, null]
        ]
      ],
      "error": "stroke-borderclip"
    },
    {
      "id": "stroke-odd-dashes",
      "base": "zero-area-stroke",
      "changes": [[[5, 0, 2, 5], [2]]],
      "error": "dash"
    },
    {
      "id": "conic-weight",
      "base": "exact-verbs",
      "changes": [[[4, 0, 2, 4, 5], 0]],
      "error": "range"
    },
    {
      "id": "contour-needs-move",
      "base": "hairline",
      "changes": [
        [
          [4, 0, 2, 0],
          [2, 0, 0]
        ]
      ],
      "error": "contour"
    },
    {
      "id": "restore-underflow",
      "base": "balanced-state-higher-minor",
      "changes": [[[3], [[3]]]],
      "error": "state"
    },
    {
      "id": "unbalanced-save",
      "base": "balanced-state-higher-minor",
      "changes": [[[3], [[2]]]],
      "error": "state"
    },
    {
      "id": "crossed-layer-save",
      "base": "layer",
      "changes": [[[3], [[6, [0, 0, 1, 1], 0], [2], [7], [3]]]],
      "error": "state"
    },
    {
      "id": "content-without-mode",
      "base": "content-repeat",
      "changes": [[[3], [[8]]]],
      "error": "content"
    },
    {
      "id": "content-mode-not-first",
      "base": "content-repeat",
      "changes": [[[3], [[2], [10], [3]]]],
      "error": "content"
    },
    {
      "id": "negative-blur",
      "base": "shadow",
      "changes": [[[3, 0, 5], -1]],
      "error": "range"
    },
    {
      "id": "coincident-gradient",
      "base": "linear-hard-stops",
      "changes": [[[5, 0, 0, 3], 40]],
      "error": "gradient"
    },
    {
      "id": "decreasing-stops",
      "base": "linear-hard-stops",
      "changes": [
        [
          [5, 0, 0, 6],
          [0, 1, 0.5]
        ]
      ],
      "error": "gradient"
    },
    {
      "id": "higher-minor-unknown-extension",
      "base": "balanced-state-higher-minor",
      "changes": [[[3], [[255, "unknown", 1, [], []]]]],
      "error": "unsupported"
    },
    {
      "id": "extension-resource-bounds",
      "base": "rect",
      "changes": [[[3], [[255, "unknown", 1, [], [0]]]]],
      "error": "reference"
    },
    {
      "id": "pointer-object",
      "base": "rect",
      "changes": [[[8], [["native", { "pointer": 123 }]]]],
      "error": "value-type"
    },
    {
      "id": "unsupported-tail",
      "base": "rect",
      "changes": [
        [
          [3],
          [
            [1, 0, 0],
            [255, "unknown", 1, [], []]
          ]
        ]
      ],
      "error": "unsupported"
    }
  ],
  "variants": [
    {
      "id": "missing-required",
      "base": "rect",
      "changes": [[[7], ["paint.blend.SrcOver", "paint.solid", "paint.srgb"]]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "duplicate-feature",
      "base": "rect",
      "changes": [[[7], ["path.rect", "path.rect"]]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "unsorted-feature",
      "base": "rect",
      "changes": [[[7], ["path.rect", "paint.srgb"]]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "extra-unknown-feature",
      "base": "rect",
      "changes": [[[7], ["future"]]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "empty-features",
      "base": "rect",
      "changes": [[[7], []]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "empty-feature-name",
      "base": "rect",
      "changes": [[[7], [""]]],
      "expected": {
        "operations": ["DrawPath"],
        "paths": [[1, 2, 4, 6, 8]],
        "sourceAlpha": [[128]],
        "drawAlpha": [0.5],
        "effectiveArgb": [[1074864691]],
        "blendModes": ["SrcOver"],
        "currentPolyfill": "supported"
      }
    },
    {
      "id": "translated-nonuniform-general-difference",
      "base": "translated-uniform-hole",
      "changes": [[[4, 1, 2], 2.5]],
      "expected": {
        "paths": [
          [1, 1000000, 0, 40, 20],
          [1, 1000002, 2.5, 36, 16],
          [3, 0, 1]
        ],
        "currentPolyfill": "unsupported"
      },
      "probes": [
        [2, 1000020, 2.25, true],
        [2, 1000020, 10, false],
        [2, 999999, 10, false]
      ]
    },
    {
      "id": "alpha-clamp-high",
      "base": "rect",
      "changes": [[[5, 0, 1, 0], 2]],
      "expected": { "drawAlpha": [1], "effectiveArgb": [[2148606515]] }
    },
    {
      "id": "alpha-clamp-low",
      "base": "rect",
      "changes": [[[5, 0, 1, 0], -1]],
      "expected": {
        "drawAlpha": [0],
        "effectiveArgb": [[1122867]],
        "sourceAlpha": [[128]]
      }
    },
    {
      "id": "matrix-state",
      "base": "balanced-state-higher-minor",
      "changes": [
        [
          [3, 1],
          [4, 1, 0, 0, 1, 2, 3]
        ]
      ],
      "expected": { "currentPolyfill": "unsupported" }
    },
    {
      "id": "repeated",
      "base": "linear-hard-stops",
      "changes": [
        [[5, 0, 0, 7], 1],
        [
          [7],
          [
            "paint.blend.SrcOver",
            "paint.linear",
            "paint.srgb",
            "paint.tile.Repeated",
            "path.rect"
          ]
        ]
      ],
      "expected": {
        "tileModes": ["Repeated"],
        "currentPolyfill": "unsupported"
      }
    },
    {
      "id": "decal",
      "base": "linear-hard-stops",
      "changes": [
        [[5, 0, 0, 7], 3],
        [
          [7],
          [
            "paint.blend.SrcOver",
            "paint.linear",
            "paint.srgb",
            "paint.tile.Decal",
            "path.rect"
          ]
        ]
      ],
      "expected": { "tileModes": ["Decal"], "currentPolyfill": "unsupported" }
    },
    {
      "id": "square-round",
      "base": "zero-area-stroke",
      "changes": [
        [[5, 0, 2, 2], 2],
        [[5, 0, 2, 3], 1]
      ],
      "expected": { "caps": ["Square"], "joins": ["Round"] }
    },
    {
      "id": "filter-matrix",
      "base": "rect",
      "changes": [
        [
          [5, 0, 1, 3],
          [2, [1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0]]
        ],
        [
          [7],
          [
            "paint.blend.SrcOver",
            "paint.color-filter",
            "paint.solid",
            "paint.srgb",
            "path.rect"
          ]
        ]
      ],
      "expected": { "currentPolyfill": "unsupported" }
    }
  ],
  "valueTrees": {
    "slots": [
      "command-extension",
      "path-extension",
      "source-extension",
      "resource",
      "effect",
      "path-effect",
      "filter-extension",
      "metadata"
    ],
    "cases": [
      {
        "id": "depth64",
        "depth": 64,
        "structure": "ok",
        "semantic": "unsupported",
        "metadataSemantic": "ok"
      },
      {
        "id": "depth65",
        "depth": 65,
        "structure": "depth",
        "semantic": "depth",
        "metadataSemantic": "depth"
      },
      {
        "id": "cyclic-value",
        "cycle": true,
        "structure": "cycle",
        "semantic": "cycle",
        "metadataSemantic": "cycle"
      }
    ]
  },
  "producerCases": [
    "rect",
    "elliptical-normalization",
    "translucent-ring",
    "general-difference",
    "collapsed-hole",
    "zero-area-stroke",
    "hairline",
    "exact-verbs",
    "empty-clip",
    "callback-empty-clip",
    "linear-hard-stops",
    "radial",
    "sweep",
    "content-zero",
    "content-repeat",
    "layer",
    "shadow",
    "asymmetric-rtl",
    "normalized-circle",
    "translated-uniform-hole",
    "linear-forward-scope",
    "linear-offbounds-hole",
    "balanced-state",
    "stroke-tail"
  ],
  "polyfillAcceptance": [
    {
      "id": "recording-v1/rect",
      "family": "recording-v1",
      "case": "rect",
      "scales": [1, 2, 3],
      "boxes": [[2, 4, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0]],
      "draws": [["fill", 1074864691, 1]]
    },
    {
      "id": "recording-v1/translucent-ring",
      "family": "recording-v1",
      "case": "translucent-ring",
      "scales": [1, 2, 3],
      "boxes": [
        [0, 0, 40, 20, 4, 4, 4, 4, 4, 4, 4, 4],
        [2, 2, 36, 16, 2, 2, 2, 2, 2, 2, 2, 2]
      ],
      "draws": [["border", 1074864691, 1, 2]]
    },
    {
      "id": "recording-v1/collapsed-hole",
      "family": "recording-v1",
      "case": "collapsed-hole",
      "scales": [1, 2, 3],
      "boxes": [[0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0]],
      "draws": [["fill", 4294967295, 1]]
    },
    {
      "id": "recording-v1/balanced-state-higher-minor",
      "family": "recording-v1",
      "case": "balanced-state-higher-minor",
      "scales": [1, 2, 3],
      "boxes": [],
      "draws": []
    },
    {
      "id": "recording-v1/balanced-state",
      "family": "recording-v1",
      "case": "balanced-state",
      "scales": [1, 2, 3],
      "boxes": [],
      "draws": []
    },
    {
      "id": "recording-v1/empty-clip",
      "family": "recording-v1",
      "case": "empty-clip",
      "scales": [1, 2, 3],
      "boxes": [],
      "draws": [["clip", [0, 0, 0, 10, 0, 0, 0, 0, 0, 0, 0, 0]]]
    },
    {
      "id": "recording-v1/linear-hard-stops",
      "family": "recording-v1",
      "case": "linear-hard-stops",
      "scales": [1, 2, 3],
      "boxes": [
        [0, 0, 20, 10, 0, 0, 0, 0, 0, 0, 0, 0],
        [-10, 0, 50, 10, 0, 0, 0, 0, 0, 0, 0, 0]
      ],
      "draws": [
        [
          "linear",
          270,
          2,
          1,
          0,
          [1074864691, 1074864691, 2164260863, 2147483903, 2147483903],
          [0, 0, 0.5, 0.5, 1]
        ]
      ]
    },
    {
      "id": "recording-v1/linear-forward-scope",
      "family": "recording-v1",
      "case": "linear-forward-scope",
      "scales": [1, 2, 3],
      "boxes": [
        [2, 4, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 8, 12, 0, 0, 0, 0, 0, 0, 0, 0]
      ],
      "draws": [
        [
          "linear",
          180,
          2,
          1,
          0,
          [1074864691, 1074864691, 2164260863, 2164260863],
          [0, 0, 0.8333333333333334, 1]
        ]
      ]
    },
    {
      "id": "recording-v1/linear-offbounds-hole",
      "family": "recording-v1",
      "case": "linear-offbounds-hole",
      "scales": [1, 2, 3],
      "boxes": [
        [0, 0, 40, 20, 0, 0, 0, 0, 0, 0, 0, 0],
        [-10, 0, 60, 20, 0, 0, 0, 0, 0, 0, 0, 0],
        [2, 2, 36, 16, 0, 0, 0, 0, 0, 0, 0, 0]
      ],
      "draws": [
        [
          "linear",
          90,
          2,
          1,
          4,
          [
            1074864691,
            1074864691,
            2164195328,
            2147548928,
            2147483903,
            2147483903
          ],
          [0, 0, 0.25, 0.25, 1, 1]
        ]
      ]
    },
    {
      "id": "recording-v1/shadow",
      "family": "recording-v1",
      "case": "shadow",
      "scales": [1, 2, 3],
      "boxes": [
        [0, 0, 40, 20, 2, 2, 3, 3, 4, 4, 5, 5],
        [-1, 4, 38, 18, 1, 1, 2, 2, 3, 3, 4, 4]
      ],
      "draws": [["shadow", 2148606515, 4, 2, 1, 0]]
    },
    {
      "id": "recording-v1/asymmetric-rtl",
      "family": "recording-v1",
      "case": "asymmetric-rtl",
      "scales": [1, 2, 3],
      "boxes": [[0, 0, 20, 10, 4, 4, 1, 1, 2, 2, 3, 3]],
      "draws": [["fill", 4294967295, 1]]
    },
    {
      "id": "recording-v1/normalized-circle",
      "family": "recording-v1",
      "case": "normalized-circle",
      "scales": [1, 2, 3],
      "boxes": [[0, 0, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]],
      "draws": [["fill", 4294967295, 1]]
    },
    {
      "id": "recording-v1/translated-uniform-hole",
      "family": "recording-v1",
      "case": "translated-uniform-hole",
      "scales": [1, 2, 3],
      "boxes": [
        [1000000, 0, 40, 20, 0, 0, 0, 0, 0, 0, 0, 0],
        [1000002, 2, 36, 16, 0, 0, 0, 0, 0, 0, 0, 0]
      ],
      "draws": [["border", 4294967295, 1, 2]]
    }
  ]
}
)fixture"},
};
