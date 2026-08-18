{ self
, mkShell
, meson
, ninja
, pkg-config
, doxygen
, llvmPackages_latest
, lib

, comp
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    (lib.hiPrio llvmPackages_latest.clang-tools)
    comp
    llvmPackages_latest.libllvm
    meson
    ninja
    pkg-config
    doxygen
  ];
}
