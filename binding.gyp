{
  "targets": [
    {
      "target_name": "nodejs-nur",
      "msvs_settings": {
        "VCCLCompilerTool": {
          "ExceptionHandling": 1,
          "AdditionalOptions": [
            "-std:c++20"
          ]
        }
      },
      "defines": [
        "NAPI_VERSION=8",
        "UNICODE",
        "_UNICODE"
      ],
      "sources": [
        "src/addon/NUR/nur-main/nur-main.cpp",
        "src/addon/NUR/nur-main/nur-wrapper-util.cpp",
        "src/addon/NUR/nur-node/nur-node.cpp",
        "src/addon/NUR/nur-node/nur-node-util.cpp",
        "src/addon/NUR/index.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "src/addon/include",
        "NUR-includes"
      ],
      "libraries": [
        "<(module_root_dir)/NUR-lib/x64/NURAPI.lib"
      ],
      "dependencies": [
        "<!(node -p \"require('node-addon-api').gyp\")"
      ]
    },
    {
      "target_name": "copy_dll",
      "type": "none",
      "dependencies": [
        "nodejs-nur"
      ],
      "copies": [
        {
          "destination": "build/Release",
          "files": [
            "<(module_root_dir)/NUR-lib/x64/NURAPI.dll"
          ]
        }
      ]
    }
  ]
}
