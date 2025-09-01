{
  description = "XCSoar glide computer";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.self.submodules = true;

  outputs =
    { self, nixpkgs }:
    let
      # TODO: forAllSystems
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        # For android packages.
        config.allowUnfree = true;
        config.android_sdk.accept_license = true;
      };
      mkXCsoar = pkgs.callPackage ./xcsoar.nix;
    in
    {
      packages."${system}" = {
        default = mkXCsoar {
          useSDL = true;
        };

        android = mkXCsoar {
          android = true;
        };

        android-testing = mkXCsoar {
          android = true;
          testing = true;
        };

        gles = mkXCsoar {
          useGLES = true;
        };

        gles-armv7 = pkgs.pkgsCross.armv7l-hf-multiplatform.callPackage ./xcsoar.nix {
          useGLES = true;
        };

      };
    };
}
