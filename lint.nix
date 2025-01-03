{ lib, pkgs, fetchurl, llvmPackages, IOAPI }:

llvmPackages.libcxxStdenv.mkDerivation rec {
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

  depAsio = fetchurl
    {
      name = "asio";
      url = "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-0.tar.gz";
      sha256 = "ImQ4sHmAma0qICVjqDVxzgbdE7Vw2P3tSEDbwfl/oyg=";
    };
  depGoogletest = fetchurl
    {
      name = "googletest";
      url = "https://github.com/google/googletest/archive/refs/tags/release-1.12.1.tar.gz";
      sha256 = "gZZP5XjpvXyU39sJyOTW5nWeGZZ+OX2+pI0cEORdDfI=";
    };
  depGSL = fetchurl
    {
      name = "GSL";
      url = "https://github.com/microsoft/GSL/archive/refs/tags/v4.0.0.tar.gz";
      sha256 = "8OMssQZU/qka1WveiRcNeM+/Q2PuCwHY8JfeK6SfbOk=";
    };
  depSpdlog = fetchurl
    {
      name = "spdlog";
      url = "https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz";
      sha256 = "Tczy0Q9BDB4v6v+Jlmv8SaGrsp728IJGM1sRDgAeCak=";
    };

  nativeBuildInputs = with pkgs; [ autoPatchelfHook cmake ninja (llvmPackages.clang-tools.override { enableLibcxx = true; }) llvmPackages.lld ];
  buildInputs = with pkgs; [ boost liburing ];

  configurePhase = ''
    runHook preConfigure

    LOGGING_OFF_DEP_DIR_PATTERN="build/ci_linting_logging_off/_deps/{}-subbuild/{}-populate-prefix/src"
    ASIO_DIR=$(echo $LOGGING_OFF_DEP_DIR_PATTERN | sed -e "s/{}/asio/g")
    GOOGLETEST_DIR=$(echo $LOGGING_OFF_DEP_DIR_PATTERN | sed -e "s/{}/googletest/g")
    GSL_DIR=$(echo $LOGGING_OFF_DEP_DIR_PATTERN | sed -e "s/{}/gsl/g")
    SPDLOG_DIR=$(echo $LOGGING_OFF_DEP_DIR_PATTERN | sed -e "s/{}/spdlog/g")
    mkdir -p $ASIO_DIR $GOOGLETEST_DIR $GSL_DIR $SPDLOG_DIR
    cp -r ${depAsio} $ASIO_DIR/asio-1-28-0.tar.gz
    cp -r ${depGoogletest} $GOOGLETEST_DIR/release-1.12.1.tar.gz
    cp -r ${depGSL} $GSL_DIR/v4.0.0.tar.gz
    cp -r ${depSpdlog} $SPDLOG_DIR/v1.12.0.tar.gz

    LOGGING_ON_DEP_DIR_PATTERN="build/ci_linting_logging_on/_deps/{}-subbuild/{}-populate-prefix/src"
    ASIO_DIR=$(echo $LOGGING_ON_DEP_DIR_PATTERN | sed -e "s/{}/asio/g")
    GOOGLETEST_DIR=$(echo $LOGGING_ON_DEP_DIR_PATTERN | sed -e "s/{}/googletest/g")
    GSL_DIR=$(echo $LOGGING_ON_DEP_DIR_PATTERN | sed -e "s/{}/gsl/g")
    SPDLOG_DIR=$(echo $LOGGING_ON_DEP_DIR_PATTERN | sed -e "s/{}/spdlog/g")
    mkdir -p $ASIO_DIR $GOOGLETEST_DIR $GSL_DIR $SPDLOG_DIR
    cp -r ${depAsio} $ASIO_DIR/asio-1-28-0.tar.gz
    cp -r ${depGoogletest} $GOOGLETEST_DIR/release-1.12.1.tar.gz
    cp -r ${depGSL} $GSL_DIR/v4.0.0.tar.gz
    cp -r ${depSpdlog} $SPDLOG_DIR/v1.12.0.tar.gz
    
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
