{ lib, pkgs, fetchurl, llvmPackages }:

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
    "README.md" # some uts depend on it
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
  nativeCheckInputs = [ llvmPackages.llvm ];
  buildInputs = with pkgs; [ boost liburing ];

  configurePhase = ''
    runHook preConfigure

    DEP_DIR_PATTERN="build/enable_logging/_deps/{}-subbuild/{}-populate-prefix/src"
    ASIO_DIR=$(echo $DEP_DIR_PATTERN | sed -e "s/{}/asio/g")
    GOOGLETEST_DIR=$(echo $DEP_DIR_PATTERN | sed -e "s/{}/googletest/g")
    GSL_DIR=$(echo $DEP_DIR_PATTERN | sed -e "s/{}/gsl/g")
    SPDLOG_DIR=$(echo $DEP_DIR_PATTERN | sed -e "s/{}/spdlog/g")
    mkdir -p $ASIO_DIR $GOOGLETEST_DIR $GSL_DIR $SPDLOG_DIR
    cp -r ${depAsio} $ASIO_DIR/asio-1-28-0.tar.gz
    cp -r ${depGoogletest} $GOOGLETEST_DIR/release-1.12.1.tar.gz
    cp -r ${depGSL} $GSL_DIR/v4.0.0.tar.gz
    cp -r ${depSpdlog} $SPDLOG_DIR/v1.12.0.tar.gz
    cmake --preset enable_logging

    runHook postConfigure
  '';
  buildPhase = ''
    runHook preBuild

    cmake --build --preset ci_test

    runHook postBuild
  '';
  doCheck = true;
  checkPhase =
    let
      libPath = lib.makeLibraryPath [
        llvmPackages.libcxx
        pkgs.liburing
      ];
    in
    ''
      runHook preCheck

      patchelf --set-rpath ${libPath} build/enable_logging/tests/xyco_test_epoll
      patchelf --set-rpath ${libPath} build/enable_logging/tests/xyco_test_uring

      LLVM_PROFILE_FILE="xyco_test_epoll.profraw" SPDLOG_LEVEL=info build/enable_logging/tests/xyco_test_epoll --gtest_break_on_failure --gtest_shuffle --gtest_death_test_style=threadsafe
      llvm-profdata merge -sparse xyco_test_epoll.profraw -o xyco_test_epoll.profdata
      llvm-cov show build/enable_logging/tests/xyco_test_epoll -ignore-filename-regex=_deps -instr-profile=xyco_test_epoll.profdata > coverage_epoll.txt
      LLVM_PROFILE_FILE="xyco_test_uring.profraw" SPDLOG_LEVEL=info build/enable_logging/tests/xyco_test_uring --gtest_break_on_failure --gtest_shuffle --gtest_death_test_style=threadsafe
      llvm-profdata merge -sparse xyco_test_uring.profraw -o xyco_test_uring.profdata
      llvm-cov show build/enable_logging/tests/xyco_test_uring -ignore-filename-regex=_deps -instr-profile=xyco_test_uring.profdata > coverage_uring.txt

      runHook postCheck
    '';
  installPhase = ''
    runHook preInstall

    mkdir $out
    cp build/enable_logging/tests/xyco_test_* $out/
    cp coverage_*.txt $out/

    runHook postInstall
  '';
}
