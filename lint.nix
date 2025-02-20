{ lib, pkgs, llvmPackages, asio, gtest, microsoft-gsl, spdlog, logging, IOAPI }:

llvmPackages.libcxxStdenv.mkDerivation {
  name = "xyco";

  src = lib.sourceByRegex ./. [
    "^benchmark.*"
    "^examples.*"
    "^include.*"
    "^src.*"
    "^tests.*"
    "CMakeLists.txt"
    "CMakePresets.json"
    ".clang-format"
    ".clang-tidy"
  ];

  nativeBuildInputs = with pkgs; [
    autoPatchelfHook
    cmake
    ninja
    (llvmPackages.clang-tools.override { enableLibcxx = true; })
    llvmPackages.lld
  ];
  buildInputs = [
    pkgs.boost
    pkgs.liburing
    asio
    gtest
    microsoft-gsl
    spdlog
  ];

  cmakeFlags = [
    "-DXYCO_ENABLE_BENCHMARK=ON"
    "-DXYCO_ENABLE_EXAMPLES=ON"
    "-DXYCO_ENABLE_TESTS=ON"
    "-DXYCO_ENABLE_LINTING=ON"
    "-DXYCO_ENABLE_LOGGING=${if logging then "ON" else "OFF"}"
    "-DXYCO_IO_API=${IOAPI}"
  ];

  doCheck = true;
  checkPhase = ''
    runHook preCheck

    cd ..
    find . -type f \( -not -path "./build/*" -and \( -name \*.cc -or -name \*.h -or -name \*.ccm \) \) | xargs clang-format --Werror --fcolor-diagnostics --dry-run --verbose
    cd build

    runHook postCheck
  '';
}
