@echo off
chcp 65001 >nul
echo [Process] Build DLL...

g++ main.cpp glad_4.6/src/glad.c -mconsole -Iglfw_3.4/include -Iglad_4.6/include -Iglm_0.9.9.8 -Lglfw_3.4/lib-mingw-w64 -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -shared -o main.dll

if %errorlevel%==0 (
    copy glfw_3.4\lib-mingw-w64\glfw3.dll . >nul
    echo [Success] DLL+glfw3.dll Ready.
) else (
    echo [Failure] Failure to Build DLL!
    pause
    exit /b 1
)

echo [Process] Build exe File...
g++ main.cpp glad_4.6/src/glad.c -mconsole -Iglfw_3.4/include -Iglad_4.6/include -Iglm_0.9.9.8 -L. -Lglfw_3.4/lib-mingw-w64 -lmain -lglfw3 -lopengl32 -lgdi32 -luser32 -lshell32 -o main_test.exe

if %errorlevel%==0 (
    echo [Success] Execute OpenGL Window!
    main_test.exe
) else (
    echo [Failure] Failure to build exe File!
)

pause
