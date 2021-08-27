{ version, meta }:

{ lib, stdenv, cmake, ninja, python2, boost
, coriolis-bootstrap, coriolis-vlsisapd, coriolis-hurricane
, coriolis-crlcore }:

let boostWithPython = boost.override { enablePython = true; python = python2; }; in

stdenv.mkDerivation {
  pname = "coriolis-flute";

  src = ../flute;

  buildInputs = [ python2 boostWithPython coriolis-bootstrap coriolis-vlsisapd coriolis-hurricane coriolis-crlcore ];
  nativeBuildInputs = [ cmake ninja ];

  inherit version meta;
}
