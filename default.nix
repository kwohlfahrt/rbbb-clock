{ nixpkgs ? import <nixpkgs> {} }:

nixpkgs.callPackage ./firmware.nix {
    avrgcc = nixpkgs.pkgsCross.avr.buildPackages.gcc;
    avrbinutils = nixpkgs.pkgsCross.avr.buildPackages.binutils;
}
