{
  "targets": [
    {
      "target_name": "grandiose",
      "sources": [
        "src/grandiose_util.cc",
        "src/grandiose_find.cc",
        "src/grandiose_send.cc",
        "src/grandiose_receive.cc",
        "src/grandiose.cc"
      ],
      "include_dirs": [ "include" ],
      "conditions":[
        ["OS=='win'", {
          "copies":[
            {
              "destination": "build/Release",
              "files": [
                "lib/win_x64/Processing.NDI.Lib.x64.dll"
              ]
            }
          ],
          "link_settings": {
            "libraries": [ "Processing.NDI.Lib.x64.lib" ],
            "library_dirs": [ "lib/win_x64" ]
          },
        }],
        ["OS=='linux'", {
          "copies":[
            {
              "destination": "build/Release",
              "files": [
                "lib/linux_x64/libndi.so",
                "lib/linux_x64/libndi.so.5",
                "lib/linux_x64/libndi.so.5.1.1",
              ]
            }
          ],
          "link_settings": {
            "libraries": [
              "<(module_root_dir)/build/Release/libndi.so.5"
            ],
            "ldflags": [
              "-L<@(module_root_dir)/build/Release",
              "-Wl,-rpath,<@(module_root_dir)/build/Release"
            ]
          },
        }],
        ["OS=='mac'", {
          "copies":[
            {
              "destination": "build/Release",
              "files": [
                "lib/mac_x64/libndi.5.dylib"
              ]
            }
          ],
        }]
      ]
    }
  ]
}
