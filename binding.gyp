{
  "targets": [
    {
      "target_name": "addon",
      "sources": [ "addon.cc", "annoyindexwrapper.cc" ],
      "include_dirs": [
        "<!(node -e \"require('nan')\")"
      ],
      "defines": [
        # Comment out the next line for single threaded builds
        "ANNOYLIB_MULTITHREADED_BUILD"
      ]
    }
  ]
}
