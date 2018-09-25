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
      "link_settings": {
        "libraries": [ "Processing.NDI.Lib.x64.lib" ],
        "library_dirs": [ "lib/win_x64" ]
      },
      "copies": [
        {
          "destination": "build/Release",
          "files": [
            "lib/win_x64/Processing.NDI.Lib.x64.dll"
          ]
        }
      ]
    }
  ]
}
