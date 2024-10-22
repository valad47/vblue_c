{pkgs ? import <nixpkgs> {}}:
pkgs.mkShell {
    name = "vblue_c-devenv";
    nativeBuildInputs = with pkgs; [
        gcc
        git
        gnumake
        cmake
        pkg-config
    ];
    buildInputs = with pkgs; [
        gtk4.dev
		raylib
    ];
    shellHook = ''
        echo Entered development enviroment...
    '';
}
