{ self
, mkShell
, clang-tools
, meson
, ninja
, pkg-config
, doxygen
, lib

, comp
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    (lib.hiPrio clang-tools)
    comp
    meson
    ninja
    pkg-config
    doxygen
  ];
}
