{
  description = "A library to help with memory allocation in C";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs, ... }:
    let
      inherit (nixpkgs) lib;
      systems = [ "x86_64-linux" "aarch64-linux" ];
      eachSystem = f: lib.genAttrs systems f;
      pkgsFor = eachSystem (system:
        import nixpkgs {
          inherit system;
        });
    in
    {
      devShells = eachSystem (system:
        let
          pkgs = pkgsFor.${system};
        in
        rec {
          default = clang;
          gcc = pkgs.callPackage ./nix/shell.nix { inherit self; comp = pkgs.gcc; };
          clang = pkgs.callPackage ./nix/shell.nix { inherit self; comp = pkgs.llvmPackages_latest.clang; };
        }
      );
    };
}
