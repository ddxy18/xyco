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
        llvmPackages = pkgs.llvmPackages_18;
        microsoft-gsl = pkgs.microsoft-gsl;
        asio = pkgs.asio;
        spdlog = pkgs.spdlog.overrideAttrs {
          cmakeFlags = [ "-DSPDLOG_USE_STD_FORMAT=ON" ];
        };
        gtest = pkgs.gtest.override {
          stdenv = llvmPackages.libcxxStdenv;
          static = true;
        };
      in
      {
        packages.lib-epoll = pkgs.callPackage ./lib.nix {
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          IOAPI = "epoll";
        };
        packages.lib-uring = pkgs.callPackage ./lib.nix {
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          IOAPI = "io_uring";
        };
        packages.unit-test-epoll = pkgs.callPackage ./unit-test.nix {
          llvmPackages = llvmPackages;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          IOAPI = "epoll";
        };
        packages.unit-test-uring = pkgs.callPackage ./unit-test.nix {
          llvmPackages = llvmPackages;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          IOAPI = "io_uring";
        };
        packages.lint-logging-off-epoll = pkgs.callPackage ./lint.nix {
          llvmPackages = llvmPackages;
          asio = asio;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          logging = false;
          IOAPI = "epoll";
        };
        packages.lint-logging-on-epoll = pkgs.callPackage ./lint.nix {
          llvmPackages = llvmPackages;
          asio = asio;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          logging = true;
          IOAPI = "epoll";
        };
        packages.lint-logging-off-uring = pkgs.callPackage ./lint.nix {
          llvmPackages = llvmPackages;
          asio = asio;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          logging = false;
          IOAPI = "io_uring";
        };
        packages.lint-logging-on-uring = pkgs.callPackage ./lint.nix {
          llvmPackages = llvmPackages;
          asio = asio;
          gtest = gtest;
          microsoft-gsl = microsoft-gsl;
          spdlog = spdlog;
          logging = true;
          IOAPI = "io_uring";
        };
      });
}
