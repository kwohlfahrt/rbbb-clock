{ stdenv, avrgcc, avrbinutils, callPackage }:

stdenv.mkDerivation {
  name = "keyboard.hex";
  src = ./firmware;
  nativeBuildInputs = [ avrgcc avrbinutils ];

  installPhase = ''
    mkdir -p $out
    cp clock.hex $out
  '';
}
