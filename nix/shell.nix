{ self
, mkShell
, gcc
, clang-tools
, meson
, ninja
, pkg-config
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    gcc
    clang-tools
    meson
    ninja
    pkg-config
  ];
}
