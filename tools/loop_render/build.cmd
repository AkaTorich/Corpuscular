@echo off
rem LoopRender build (Release). Code\Libraries must be mapped to V: (the plugins' build.cmd leave it mapped).
setlocal
set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Insiders"
set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not exist "V:\JUCE\CMakeLists.txt" subst V: "%~dp0..\..\..\..\Libraries"
subst Q: /d >nul 2>&1
subst Q: "%~dp0."
pushd Q:\
"%CMAKE%" -S . -B build -G "Visual Studio 18 2026" -A x64
if errorlevel 1 goto fail
"%CMAKE%" --build build --config Release --parallel 4 -- /nologo /v:minimal
if errorlevel 1 goto fail
popd
subst Q: /d
echo Done: build\LoopRender_artefacts\Release\LoopRender.exe
exit /b 0
:fail
popd
subst Q: /d
echo BUILD FAILED
exit /b 1
