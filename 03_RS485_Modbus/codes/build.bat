@echo off
setlocal

where gcc >nul 2>nul
if errorlevel 1 (
    echo gcc not found. Please install GCC or use this code as reading material.
    exit /b 1
)

gcc -Wall -Wextra -std=c99 -c rs485_frame_demo.c -o rs485_frame_demo.o
if errorlevel 1 exit /b 1

gcc -Wall -Wextra -std=c99 -c modbus_crc16.c -o modbus_crc16.o
if errorlevel 1 exit /b 1

gcc -Wall -Wextra -std=c99 -c modbus_rtu_demo.c -o modbus_rtu_demo.o
if errorlevel 1 exit /b 1

echo C example syntax check passed.
endlocal
