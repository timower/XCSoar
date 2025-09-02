{
  stdenv,
  lib,
  fetchurl,
  quilt,
  perl,
  pkg-config,
  dbus,
  alsa-lib,
  freetype,
  libpng,
  libinput,
  lua54Packages,
  libjpeg,
  glm,
  libsodium,
  c-ares,
  fmt,
  libdrm,
  mesa,
  curl,
  gettext,
  imagemagick,
  libxslt,
  librsvg,
  sox,
  dejavu_fonts,
  libGL,
  SDL2,
  python3,
  cmake,
  ninja,
  libtool,
  automake,
  autoconf,
  jdk8,
  vorbis-tools,
  zip,
  pkg-config-unwrapped,
  libgbm,

  apple-sdk,

  buildPackages,
  pkgsBuildBuild,
  androidenv,

  useSDL ? false,
  android ? false,
  useGLES ? false,
  debug ? false,
  testing ? false,
}:
let
  vendor-libs = [
    (fetchurl {
      url = "https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.bz2";
      sha256 = "sha256-r1e+JctMT0tBPtaS/jeK/7Q1LqUPvilKEe9Uj01SfYk=";
    })
  ]
  ++ lib.optionals android [
    (fetchurl {
      url = "https://github.com/fmtlib/fmt/archive/11.1.4.tar.gz";
      sha256 = "ac366b7b4c2e9f0dde63a59b3feb5ee59b67974b14ee5dc9ea8ad78aa2c1ee1e";
    })
    (fetchurl {
      url = "https://download.libsodium.org/libsodium/releases/libsodium-1.0.20.tar.gz";
      sha256 = "ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19";
    })
    (fetchurl {
      url = "https://www.openssl.org/source/openssl-3.1.7.tar.gz";
      sha256 = "053a31fa80cf4aebe1068c987d2ef1e44ce418881427c4464751ae800c31d06c";
    })
    (fetchurl {
      url = "https://github.com/c-ares/c-ares/releases/download/cares-1_24_0/c-ares-1.24.0.tar.gz";
      sha256 = "c517de6d5ac9cd55a9b72c1541c3e25b84588421817b5f092850ac09a8df5103";
    })
    (fetchurl {
      url = "https://curl.se/download/curl-8.5.0.tar.xz";
      sha256 = "42ab8db9e20d8290a3b633e7fbb3cec15db34df65fd1015ef8ac1e4723750eeb";
    })
    (fetchurl {
      url = "http://www.lua.org/ftp/lua-5.4.6.tar.gz";
      sha256 = "7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88";
    })
    (fetchurl {
      url = "https://www.sqlite.org/2023/sqlite-autoconf-3420000.tar.gz";
      sha256 = "7abcfd161c6e2742ca5c6c0895d1f853c940f203304a0b49da4e1eca5d088ca6";
    })
    (fetchurl {
      url = "http://download.osgeo.org/proj/proj-9.4.0.tar.gz";
      sha256 = "3643b19b1622fe6b2e3113bdb623969f5117984b39f173b4e3fb19a8833bd216";
    })
    (fetchurl {
      url = "http://download.osgeo.org/libtiff/tiff-4.6.0.tar.xz";
      sha256 = "e178649607d1e22b51cf361dd20a3753f244f022eefab1f2f218fc62ebaf87d2";
    })
    (fetchurl {
      url = "http://download.osgeo.org/geotiff/libgeotiff/libgeotiff-1.7.4.tar.gz";
      sha256 = "c598d04fdf2ba25c4352844dafa81dde3f7fd968daa7ad131228cd91e9d3dc47";
    })
  ];

  androidComposition = androidenv.composeAndroidPackages {
    platformVersions = [ "33" ];
    buildToolsVersions = [ "33.0.2" ];
    includeNDK = true;
    ndkVersion = "26.3.11579264";
  };
  androidSdkDir = "${androidComposition.androidsdk}/libexec/android-sdk";
  androidNdkDir = "${androidSdkDir}/ndk-bundle";

  toMakeFlag = bool: if bool then "y" else "n";
in
stdenv.mkDerivation {
  pname = "xcsoar";
  version = lib.removeSuffix "\n" (builtins.readFile ./VERSION.txt);

  enableParallelBuilding = true;

  src = ./.;

  depsBuildBuild = [ buildPackages.stdenv.cc ];

  nativeBuildInputs = [
    python3
    quilt
    perl
    gettext
    imagemagick
    libxslt
    librsvg
    sox
  ]
  ++ lib.optionals (!android) [
    pkg-config
  ]
  ++ lib.optionals android [
    cmake
    ninja
    libtool
    automake
    autoconf
    vorbis-tools
    jdk8
    zip
    pkg-config-unwrapped
  ];

  # Ninja is used indirectly
  dontUseNinjaBuild = true;
  dontUseNinjaInstall = true;
  dontUseNinjaCheck = true;
  dontUseCmakeConfigure = true;

  buildInputs =
    lib.optionals (!android) [
      lua54Packages.lua
      freetype
      libpng
      libjpeg
      libsodium
      c-ares
      curl
      fmt

      libGL
      glm
    ]
    ++ lib.optionals (stdenv.hostPlatform.isLinux) [
      libinput
      alsa-lib
      dbus.dev
      libdrm
      mesa
    ]
    ++ lib.optionals useSDL [ SDL2 ]
    ++ lib.optionals useGLES [ libgbm ];

  makeFlags = [
    "WERROR=n" # Supress deprecated declaration errors..
    "DEBUG=${toMakeFlag debug}"
    "TESTING=${toMakeFlag testing}"

    # Fixes darwin builds (gcc doesn't exist) and cross builds.
    "HOSTCC=${pkgsBuildBuild.stdenv.cc}/bin/cc"
    "HOSTCXX=${pkgsBuildBuild.stdenv.cc}/bin/c++"
  ]
  ++ lib.optionals useGLES [
    "ENABLE_MESA_KMS=y"
    "GLES2=y"
    "GEOTIFF=n"
  ]
  ++ lib.optionals useSDL [
    "ENABLE_SDL=y"
    "GEOTIFF=n"
  ]
  ++ lib.optionals android [
    "TARGET=ANDROIDAARCH64"
    "ANDROID_SDK=${androidSdkDir}"
    "ANDROID_NDK=${androidNdkDir}"
  ]
  ++ lib.optionals (!android) [
    "CC=${stdenv.cc.targetPrefix}cc"
    "CXX=${stdenv.cc.targetPrefix}c++"
  ]
  ++ lib.optionals (!android && stdenv.hostPlatform.isDarwin) [
    "TARGET=${if stdenv.hostPlatform.isAarch then "MACOS" else "OSX64"}"
    "HOST_TRIPLET=${stdenv.hostPlatform.config}"
    "DARWIN_SDK=${apple-sdk.sdkroot}"
  ];

  postPatch = ''
    patchShebangs tools/BinToC.pl
    patchShebangs build/thirdparty.py

    # Make sure the locale files are found in the package.
    substituteInPlace ./src/Language/LanguageGlue.cpp \
      --replace-fail "/usr/share/locale" "$out/share/locale"

    # /usr/bin/env perl in openssl doesnt work in the sandbox.
    substituteInPlace ./build/python/build/openssl.py \
       --replace-fail "configure = [" "configure = [ 'perl',"

    # Hardcode a default nix store font.
    substituteInPlace ./src/ui/canvas/custom/Files.cpp \
      --replace-fail "/usr" "${dejavu_fonts}"

    # Montage fails to find fonts, but that should not be fatal.
    substituteInPlace ./build/imagemagick.mk \
      --replace-fail "tile 2x1" "tile 2x1 -font ${dejavu_fonts}/share/fonts/truetype/DejaVuSans.ttf"

    # Allow the PKG_CONFIG variable to be set in the environment.
    substituteInPlace ./build/pkgconfig.mk \
       --replace-fail "PKG_CONFIG = pkg-config" "PKG_CONFIG ?= pkg-config"

    substituteInPlace ./build/python/build/autotools.py \
       --replace-fail "glibtoolize" "libtoolize"
  '';

  configurePhase = ''
    install -d output/download
  ''
  + lib.strings.concatMapStrings (pkg: "cp ${pkg} output/download/${pkg.name}\n") vendor-libs
  + ''
    make boost
  '';

  preBuild = ''
    export XDG_CACHE_HOME="$(mktemp -d)"
  ''
  + lib.optionalString android ''
    # For the debug keystore
    export HOME="$XDG_CACHE_HOME"
  '';

  installTargets = "install-bin install-mo";
  installFlags = [ "prefix=$(out)" ];

  installPhase = lib.optionalString android ''
    mkdir -p $out
    cp output/ANDROID/bin/XCSoar-debug.apk $out/
  '';

  dontStrip = debug;
}
