$ELFIO_DIR = "elfio"

md -Force -Path bin > $null

c++ -I"`"$($ELFIO_DIR)`"" `
    "main.cpp" `
    -o "bin\hv2.exe" `
    -m64 -std=c++2a -g -O3 -Wall -pedantic