@echo off

pushd w:\editor

set Optimize=%1
set Run=%2

set CommonCompilerFlags= -DLL -MTd -nologo -Gm- -GR- -EHa- -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505 -wd4706 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1 -D_CRT_SECURE_NO_WARNINGS -FC -Z7 -Fm
set CommonLinkerFlags= -incremental:no -opt:ref winmm.lib user32.lib gdi32.lib opengl32.lib

if %Optimize%==1 (
    set OptimizeFlags= -Ox
) else (
    set OptimizeFlags= -Od -Oi
)

if not defined DevEnvDir (
    call shell.bat
)

IF NOT EXIST build mkdir build
pushd build

@del /Q *.pdb > NUL 2> NUL
@del /Q *.gmi > NUL 2> NUL

set FilesToCompile= ..\src\ED_win32.cpp ..\src\ED_core.cpp ..\src\ED_math.cpp ..\src\raytrace\ED_raytrace.cpp

set run_compilation= cl -Feeditor.exe -I..\src %OptimizeFlags% %CommonCompilerFlags% %FilesToCompile% /link %CommonLinkerFlags%

%run_compilation% || exit /b

if %Run%==1 (
    editor.exe
)

popd
popd
