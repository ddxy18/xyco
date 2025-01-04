{
  description = "xyos";

  inputs = {
    systems.url = "github:nix-systems/x86_64-linux";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
      in
      {
        packages.unit-test = pkgs.callPackage ./unit-test.nix { llvmPackages = pkgs.llvmPackages_18; };
        packages.lint-epoll = pkgs.callPackage ./lint.nix { llvmPackages = pkgs.llvmPackages_18; IOAPI = "epoll"; };
        packages.lint-uring = pkgs.callPackage ./lint.nix { llvmPackages = pkgs.llvmPackages_18; IOAPI = "uring"; };
      });
}
