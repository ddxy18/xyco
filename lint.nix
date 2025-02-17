{ lib, pkgs, llvmPackages, asio, gtest, microsoft-gsl, spdlog, IOAPI }:

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
    microsoft-gsl
    asio
    spdlog
    gtest
  ];

  configurePhase = ''
    runHook preConfigure

    cmake --preset ci_linting_logging_off
    cmake --preset ci_linting_logging_on

    runHook postConfigure
  '';
  buildPhase = ''
    runHook preBuild

    cmake --build --preset ${IOAPI}_ci_linting_logging_off
    cmake --build --preset ${IOAPI}_ci_linting_logging_on

    runHook postBuild
  '';
  doCheck = true;
  checkPhase = ''
    runHook preCheck

    find . -type f \( -not -path "./build/*" -and \( -name \*.cc -or -name \*.h -or -name \*.ccm \) \) | xargs clang-format --Werror --fcolor-diagnostics --dry-run --verbose

    runHook postCheck
  '';
  installPhase = ''
    runHook preInstall

    MAIN_BIN_PATTERN=build/ci_linting_{}/xyco_${IOAPI}_main
    UT_BIN_PATTERN=build/ci_linting_{}/tests/xyco_test_${IOAPI}
    EXAMPLE_BIN_PATTERN=build/ci_linting_{}/examples/xyco_${IOAPI}_http_server
    BENCHMARK_BIN_PATTERN=build/ci_linting_{}/benchmark/xyco_${IOAPI}_echo_server
    BENCHMARK_ASIO_BIN_PATTERN=build/ci_linting_{}/benchmark/asio_echo_server

    mkdir -p $out/logging_off $out/logging_on
    MAIN_BIN=$(echo $MAIN_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    UT_BIN=$(echo $UT_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    EXAMPLE_BIN=$(echo $EXAMPLE_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    BENCHMARK_BIN=$(echo $BENCHMARK_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    BENCHMARK_ASIO_BIN=$(echo $BENCHMARK_ASIO_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    cp $MAIN_BIN $UT_BIN $EXAMPLE_BIN $BENCHMARK_BIN $BENCHMARK_ASIO_BIN $out/logging_off

    MAIN_BIN=$(echo $MAIN_BIN_PATTERN | sed -e "s/{}/logging_off/g")
    UT_BIN=$(echo $UT_BIN_PATTERN | sed -e "s/{}/logging_on/g")
    EXAMPLE_BIN=$(echo $EXAMPLE_BIN_PATTERN | sed -e "s/{}/logging_on/g")
    BENCHMARK_BIN=$(echo $BENCHMARK_BIN_PATTERN | sed -e "s/{}/logging_on/g")
    BENCHMARK_ASIO_BIN=$(echo $BENCHMARK_ASIO_BIN_PATTERN | sed -e "s/{}/logging_on/g")
    cp $MAIN_BIN $UT_BIN $EXAMPLE_BIN $BENCHMARK_BIN $BENCHMARK_ASIO_BIN $out/logging_on

    runHook postInstall
  '';
}
