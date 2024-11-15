echo off
setlocal EnableDelayedExpansion

REM Usage:
REM   Add post build step of:
REM      <path\to\this>\MakeImage.bat ${ProjDirPath} ${ProjName} ${BuildArtifactFileName}

set ProjDir=%1
set ProjName=%2
set BuildArtifact=%3
set BinDir=%~dp0

echo Executing on Project Build Directory: "%cd%"
echo Using build file "%BuildArtifact%"

echo Extracting Binary File to "%ProjName%.bin"
fromelf --bincombined --output="%ProjName%.bin" %BuildArtifact%

echo Extracting entry point from ELF to "%ProjName%Entry.txt"
%BinDir%readelf.exe -h "%BuildArtifact%" | %BinDir%grep.exe Entry | %BinDir%sed.exe --regexp-extended 's/.*Entry.+:[[:space:]]*//' > "%ProjName%Entry.txt

set /p EntryPoint=<"%ProjName%Entry.txt
echo Entry point: %EntryPoint%

echo Generating image with u-boot header
%BinDir%mkimage.exe -A arm -O U-Boot -T kernel -C none -a %EntryPoint% -e %EntryPoint% -n "%ProjName% App for DE1-SoC" -d %ProjName%.bin %ProjDir%\user-mkpimage.bin
