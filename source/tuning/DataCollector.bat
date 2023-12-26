@echo off

if "%~4" == "" (
    echo Invalid number of given filepaths. Should be:
    echo {first engine filepath} {second engine filepath} {opening book filepath} {number of games mult. by 2}
    exit /b
)

set "engine_1=C:\Users\User\Desktop\lichess-bot-master\engines\version\data_storing_enabled\Austerlitz"

if "%~1" neq "default" (
    set "engine_1=%~1"
)

set "book=C:\Users\User\Desktop\lichess-bot-master\engines\Titans.bin"

if "%~3" neq "default" (
    set "book=%~3"
)

echo Tournament started
cutechess-cli -engine cmd=%engine_1% -engine cmd=%2 -each proto=uci tc=1+0.06 book=%book% -rounds 2 -games %4 -draw movenumber=125 movecount=50 score=300