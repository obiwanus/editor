@echo off

pushd w:\editor

set CommonCompilerFlags= -DLL -MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4127 -wd4201 -wd4100 -wd4189 -wd4505 -wd4706 -DBUILD_INTERNAL=1 -DBUILD_SLOW=1 -DBUILD_WIN32=1 -D_CRT_SECURE_NO_WARNINGS -FC -Z7 -Fm
set CommonLinkerFlags= -incremental:no -opt:ref winmm.lib user32.lib gdi32.lib opengl32.lib

if not defined DevEnvDir (
    call shell.bat
)

IF NOT EXIST build mkdir build
pushd build

@del /Q *.pdb > NUL 2> NUL
@del /Q *.gmi > NUL 2> NUL

set FilesToCompile= ..\base\ED_win32.cpp ..\base\ED_core.cpp ..\base\ED_math.cpp

cl -Feeditor.exe %CommonCompilerFlags% %FilesToCompile% /link %CommonLinkerFlags%

popd
popd