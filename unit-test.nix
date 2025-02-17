{ lib, pkgs, llvmPackages, asio, gtest, microsoft-gsl, spdlog }:

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
    "README.md" # some uts depend on it
  ];

  nativeBuildInputs = with pkgs; [
    autoPatchelfHook
    cmake
    ninja
    (llvmPackages.clang-tools.override { enableLibcxx = true; })
    llvmPackages.lld
  ];
  nativeCheckInputs = [ llvmPackages.llvm ];
  buildInputs = [
    pkgs.boost
    pkgs.liburing
    asio
    gtest
    microsoft-gsl
    spdlog
  ];

  configurePhase = ''
    runHook preConfigure

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
