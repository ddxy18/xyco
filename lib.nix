{ lib, pkgs, llvmPackages, microsoft-gsl, spdlog, IOAPI }:

llvmPackages.libcxxStdenv.mkDerivation {
  name = "xyco";

  src = lib.sourceByRegex ./. [
    "^include.*"
    "^src.*"
    "CMakeLists.txt"
  ];

  nativeBuildInputs = with pkgs; [
    cmake
    ninja
    (llvmPackages.clang-tools.override { enableLibcxx = true; })
    llvmPackages.lld
  ];
  buildInputs = [
    pkgs.boost
    pkgs.liburing
    microsoft-gsl
    spdlog
  ];
  # Since CMake hasn't supported link against precompiled C++20 modules, all dependents need to
  # depend on the source code directly. These libraries are propogated to hide internal details from
  # dependents.
  propagatedBuildInputs = [
    pkgs.boost
    pkgs.liburing
    microsoft-gsl
    spdlog
  ];

  cmakeFlags = [ "-DXYCO_IO_API=${IOAPI}" ];
}
