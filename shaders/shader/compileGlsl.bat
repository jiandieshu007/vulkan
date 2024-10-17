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
        echo Error code: %errorlevel%
        pause  rem 暂停以便查看错误信息
        goto :end  rem 跳转到脚本末尾，不直接退出
    )
)

echo All shaders compiled successfully.

:end
endlocal
pause
