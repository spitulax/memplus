{ self
, mkShell
, man-pages
, gcc
, clang-tools
, meson
, ninja
, pkg-config
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    man-pages
    gcc
    clang-tools
    meson
    ninja
    pkg-config
  ];
}
