{ self
, mkShell
, gcc
, clang-tools
, meson
, ninja
, pkg-config
, clang
, doxygen
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    gcc
    # clang
    clang-tools
    meson
    ninja
    pkg-config
    doxygen
  ];
}
