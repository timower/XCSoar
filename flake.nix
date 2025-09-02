{
  description = "XCSoar glide computer";

  inputs.nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  inputs.self.submodules = true;

  outputs =
    { self, nixpkgs }:
    let
      # System types to support.
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"

        # Darwin will require a linux builder to build the kernel and rootfs.
        # See linux-builder in the nixpkgs manual.
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = import nixpkgs {
            inherit system;
            # For android packages.
            config.allowUnfree = true;
            config.android_sdk.accept_license = true;
          };
          mkXCsoar = pkgs.callPackage ./xcsoar.nix;
        in
        {
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
        }
      );
    };
}
