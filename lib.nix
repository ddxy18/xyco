{ lib, pkgs, microsoft-gsl, spdlog, IOAPI }:

pkgs.stdenv.mkDerivation rec {
  name = "xyco";

  src = lib.sourceByRegex ./. [
    "^include.*"
    "^src.*"
    "CMakeLists.txt"
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

  # It is used to propagate the configure flags to the dependencies. But unlike
  # `propagatedBuildInputs`, it needs to be concatenated manually.
  cmakeFlags = [ "-DXYCO_IO_API=${IOAPI}" ];
  preInstall = "mkdir $out";
  installPhase = ''
    runHook preInstall

    cp -r ${src}/* $out/

    runHook postInstall
  '';
}
