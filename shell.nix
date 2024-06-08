
{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "nrf51-dev-environment";

  buildInputs = with pkgs; [
    inetutils
    openocd
    cmake
    ninja
    python3 # The nRF tools often require Python
    gcc-arm-embedded
  ];
  nativeBuildInputs = [
      pkgs.libusb1
  ];


  # Set the required environment variables
  shellHook = ''
  '';
}
