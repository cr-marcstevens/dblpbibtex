# Building using Visual Studio 2017

## Download

1. Setup Visual Studio 2017 and use Visual Studio Installer to install the latest update
2. Download and extract libcurl sources from https://github.com/curl/curl/releases
3. Download and extract dblpbibtex sources from https://github.com/cr-marcstevens/dblpbibtex/releases

## Build libcurl

From Start Menu run `x64 Native Tools Command Prompt for VS 2017`

1. `cd` into libcurl source directory
2. run `buildconf.bat`
3. `cd winbuild`
4. run `nmake /f Makefile.vc mode=static MACHINE=x64`
5. copy subdirs `bin`, `include`, `lib` from `<curldir>\builds\libcurl-vc-x64-release-static-ipv6-sspi-winssl` to `<dblpbibtexdir>\vs2017\libcurl`

## Build DBLPBIBTEX

1. Open `<dblpbibtexdir>\vs2017\dblpbibtex.sln` in Visual Studio
2. Hit Build->Build Solution
3. The executable will be `<dblpbibtexdir>\Release\dblpbibtex.exe`
