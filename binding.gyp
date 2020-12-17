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
                "lib/linux_x64/libndi.so.4",
                "lib/linux_x64/libndi.so.4.5.3",
              ]
            }
          ],
        }],
        ["OS=='mac'", {
          "copies":[
            {
              "destination": "build/Release",
              "files": [
                "lib/mac_x64/libndi.4.dylib"
              ]
            }
          ],
        }]
      ]
    }
  ]
}
