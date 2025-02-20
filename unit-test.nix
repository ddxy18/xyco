{ lib, pkgs, llvmPackages, gtest, microsoft-gsl, spdlog, IOAPI }:

llvmPackages.libcxxStdenv.mkDerivation {
  name = "xyco";

  src = lib.sourceByRegex ./. [
    "^include.*"
    "^src.*"
    "^tests.*"
    "CMakeLists.txt"
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
    gtest
    microsoft-gsl
    spdlog
  ];

  cmakeFlags = [
    "-DXYCO_ENABLE_LOGGING=ON"
    "-DXYCO_ENABLE_TESTS=ON"
    "-DXYCO_IO_API=${IOAPI}"
  ];

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

      cd ..

      patchelf --set-rpath ${libPath} build/tests/xyco_test

      LLVM_PROFILE_FILE="xyco_test.profraw" SPDLOG_LEVEL=info build/tests/xyco_test --gtest_break_on_failure --gtest_shuffle --gtest_death_test_style=threadsafe
      llvm-profdata merge -sparse xyco_test.profraw -o xyco_test.profdata
      llvm-cov show build/tests/xyco_test -ignore-filename-regex=_deps -instr-profile=xyco_test.profdata > coverage.txt

      runHook postCheck
    '';
  installPhase = ''
    runHook preInstall

    mkdir $out
    cp build/tests/xyco_test $out/
    cp coverage.txt $out/

    runHook postInstall
  '';
}
