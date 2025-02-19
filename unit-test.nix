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

    cmake --preset ${IOAPI}_ci_test

    runHook postConfigure
  '';
  buildPhase = ''
    runHook preBuild

    cmake --build --preset ${IOAPI}_ci_test

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

      patchelf --set-rpath ${libPath} build/${IOAPI}_ci_test/tests/xyco_test

      LLVM_PROFILE_FILE="xyco_test.profraw" SPDLOG_LEVEL=info build/${IOAPI}_ci_test/tests/xyco_test --gtest_break_on_failure --gtest_shuffle --gtest_death_test_style=threadsafe
      llvm-profdata merge -sparse xyco_test.profraw -o xyco_test.profdata
      llvm-cov show build/${IOAPI}_ci_test/tests/xyco_test -ignore-filename-regex=_deps -instr-profile=xyco_test.profdata > coverage_${IOAPI}.txt

      runHook postCheck
    '';
  installPhase = ''
    runHook preInstall

    mkdir $out
    cp build/${IOAPI}_ci_test/tests/xyco_test $out/
    cp coverage_${IOAPI}.txt $out/

    runHook postInstall
  '';
}
