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

  cmakeFlags = [
    "-DXYCO_IO_API=${IOAPI}"
  ];
}
