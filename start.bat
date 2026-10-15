@echo off
chcp 65001 >nul
setlocal EnableExtensions

for %%I in ("%~dp0..\RAG\ue-rag") do set "ROOT=%%~fI"
set "PY=%ROOT%\.venv\Scripts\python.exe"

if not exist "%PY%" (
  echo 找不到 Python: %PY%
  pause
  exit /b 1
)
if not exist "%ROOT%\agent\chat.py" (
  echo 找不到入口: %ROOT%
  pause
  exit /b 1
)

cd /d "%ROOT%"

if not "%~1"=="" (
  set "CHOICE=%~1"
  goto dispatch
)

:menu
echo.
echo  UE-RAG
echo  1  普通对话     检索后回答。不改工程，不编译
echo  2  单循环       一句话拆成步骤，每步同一张图
echo  3  多窗口       规划者拆窗口，每扇单独跑一张图，最后集成
echo  4  续接 run     续接最近一个没跑完的多窗口 run
echo  0  退出
echo.
set /p CHOICE=选择: 

:dispatch
if "%CHOICE%"=="1" goto chat
if /I "%CHOICE%"=="chat" goto chat
if "%CHOICE%"=="2" goto loop
if /I "%CHOICE%"=="loop" goto loop
if "%CHOICE%"=="3" goto multi
if /I "%CHOICE%"=="multi" goto multi
if "%CHOICE%"=="4" goto resume
if /I "%CHOICE%"=="resume" goto resume
if "%CHOICE%"=="0" goto bye
if /I "%CHOICE%"=="q" goto bye
echo 只能选 1、2、3、4 或 0
goto menu

:chat
"%PY%" "%ROOT%\agent\chat.py"
goto menu

:loop
"%PY%" "%ROOT%\agent\run_multi.py"
goto menu

:multi
"%PY%" "%ROOT%\agent\run_windows.py"
goto menu

:resume
"%PY%" "%ROOT%\agent\run_windows.py" --resume latest
goto menu

:bye
echo 退出
endlocal
exit /b 0
