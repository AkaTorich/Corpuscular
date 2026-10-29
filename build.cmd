@echo off
rem Corpuscular build (Release): VST3 plugin and standalone app.
rem JUCE tools crash on non-ASCII paths, so the project is mapped to a drive letter with subst.
setlocal
set "VSDIR=C:\Program Files\Microsoft Visual Studio\18\Insiders"
set "CMAKE=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
set "DRIVE=X:"

subst %DRIVE% /d >nul 2>&1
subst %DRIVE% "%~dp0."
if errorlevel 1 (
    echo Cannot map drive %DRIVE%
    exit /b 1
)

pushd %DRIVE%\
"%CMAKE%" -S . -B build -G "Visual Studio 18 2026" -A x64
if errorlevel 1 goto fail
"%CMAKE%" --build build --config Release --parallel 4 -- /nologo /v:minimal
if errorlevel 1 goto fail

popd
subst %DRIVE% /d
echo Done: build\CorpuscularPlugin_artefacts\Release\VST3 and build\CorpuscularApp_artefacts\Release
exit /b 0

:fail
popd
subst %DRIVE% /d
echo BUILD FAILED
exit /b 1
