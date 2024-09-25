@echo off
setlocal

rem Set the directory containing the shader files (current directory)
set SHADER_DIR=.

rem Find all .vert, .frag, .comp files and compile them to .spv
for /r "%SHADER_DIR%" %%f in (*.vert *.frag *.comp) do (
    echo Compiling %%f...
    glslangValidator -V %%f -o %%f.spv
    if errorlevel 1 (
        echo Error compiling %%f
        exit /b 1
    )
)

echo All shaders compiled successfully.
endlocal
pause
