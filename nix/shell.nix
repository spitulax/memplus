{ self
, mkShell
, meson
, ninja
, pkg-config
, doxygen
, llvmPackages_22
, lib

, comp
}:
mkShell {
  name = "memplus-shell";
  buildInputs = [
    (lib.hiPrio llvmPackages_22.clang-tools)
    comp
    llvmPackages_22.libllvm
    meson
    ninja
    pkg-config
    doxygen
  ];
}
