{ lib, pkgs, fetchurl, llvmPackages }:

# stdenv.mkDerivation now accepts a list of named parameters that describe
# the package itself.

llvmPackages.libcxxStdenv.mkDerivation rec {
  name = "xyco";

  # good source filtering is important for caching of builds.
  # It's easier when subprojects have their own distinct subfolders.
  src = lib.sourceByRegex ./. [
    "^benchmark.*"
    "^examples.*"
    "^include.*"
    "^src.*"
    "^tests.*"
    "CMakeLists.txt"
    "CMakePresets.json"
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

  HTTP_PROXY = "http://127.0.0.1:7890";
  HTTPS_PROXY = "http://127.0.0.1:7890";

  # We now list the dependencies similar to the devShell before.
  # Distinguishing between `nativeBuildInputs` (runnable on the host
  # at compile time) and normal `buildInputs` (runnable on target
  # platform at run time) is an important preparation for cross-compilation.
  nativeBuildInputs = with pkgs; [ autoPatchelfHook cmake ninja llvmPackages.clang-tools llvmPackages.lld ];
  buildInputs = with pkgs; [ boost liburing ];

  configurePhase = ''
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
  '';
  buildPhase = ''cmake --build --preset ci_test'';
  installPhase = ''
    mkdir $out
    cp build/enable_logging/tests/xyco_test_* $out/
  '';
}
