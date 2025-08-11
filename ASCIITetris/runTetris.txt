gcc -o Tetris.exe main.c Tetris.c
if %errorlevel%==0 (
    Tetris.exe
)
pause
