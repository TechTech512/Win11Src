cd \nt\minkernel\ntos\init
build -ZM 2
cd \nt\onecore\sdktools\imagehlp
build -ZM 4
cd \nt\onecore\sdktools\kdexts
build -ZM 4
cd \nt\onecore\sdktools\ntsd
build -ZM 4

rem --- Rebuild userkdx.dll every build (ianja)
cd \nt\minkernel\ntos\w32\ntuser\kdexts\kd
build -ZM 3
