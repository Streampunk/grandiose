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
      'conditions': [
        ['OS=="win"', {
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
          ]},
        ],
        ['OS=="linux"', {
          'conditions': [
            ['target_arch=="x86" or target_arch=="x64"', {
              "libraries": ['-L/lib/x86_64-linux-gnu', '-lndi']
            }]
          ]}
        ]
      ]
    }
  ]
}
