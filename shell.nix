
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "strtt-dev-environment";

  buildInputs = with pkgs; [
    cmake
    ninja
    gcc-arm-embedded
  ];
  nativeBuildInputs = [
      pkgs.libusb1
  ];


  # Set the required environment variables
  shellHook = ''
  '';
}
