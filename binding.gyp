{
  "targets": [
    {
      "target_name": "grandiose",
      "sources": [
        "src/grandiose_util.cc",
        "src/grandiose_find.cc",
        "src/grandiose_send.cc",
        "src/grandiose_send_video.cc",
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
        ['OS=="mac"', {
          'conditions': [
            ['target_arch=="x64"', {
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS': [
                  '-std=c++11',
                  '-stdlib=libc++',
                  '-fexceptions'
                ],
                'OTHER_LDFLAGS': [
                  "-Wl,-rpath,<@(module_root_dir)/build/Release"
                ]
              },
              "link_settings": {
                "libraries": [
                  "<(module_root_dir)/build/Release/libndi.4.dylib"
                ],
                "copies": [
                  {
                    "destination": "build/Release/",
                    "files": [
                      "<!@(ls -1 lib/mac_x64/libndi.4.dylib)"
                    ]
                  }
                ]
              },
            }]
          ],
          # "copies": [{
          #   "destination": "build/Release",
          #   "files": [
          #     "lib/mac_x64/libndi.4.dylib"
          #   ]
          # }]
        }],
        ['OS=="linux"', {
          'conditions': [
            ['target_arch=="x86" or target_arch=="x64"', {
              "libraries": ['-L/lib/x86_64-linux-gnu', '-lndi']
            }]
          ]
        }]
      ]
    }
  ]
}
