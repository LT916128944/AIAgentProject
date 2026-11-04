@echo off
rem stdin 被重定向（UE_RAG_STDIN）时禁用 chcp：chcp 会丢弃已缓冲的重定向输入，导致入口读不到需求行
if not defined UE_RAG_STDIN chcp 65001 >nul
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
if not "%~1"=="" exit /b %ERRORLEVEL%
goto menu

:loop
if defined UE_RAG_STDIN (
  set "PYTHONIOENCODING=utf-8"
  "%PY%" "%ROOT%\agent\run_multi.py" < "%UE_RAG_STDIN%"
) else (
  "%PY%" "%ROOT%\agent\run_multi.py"
)
if not "%~1"=="" exit /b %ERRORLEVEL%
goto menu

:multi
call :warn_live_runs
if errorlevel 1 exit /b 1
if not defined UE_RAG_STDIN goto multi_interactive
if not exist "%UE_RAG_STDIN%" (
  if exist "%UE_RAG_STDIN%.consumed" (
    echo 输入已被另一个进程占用
    exit /b 1
  )
  echo 找不到输入文件: %UE_RAG_STDIN%
  exit /b 1
)
rem 先改名再重定向。文件已被 cmd 打开时 Windows 拒绝改名，独占必须发生在打开之前。
for %%F in ("%UE_RAG_STDIN%") do ren "%%~fF" "%%~nxF.consumed"
if errorlevel 1 (
  echo 输入已被另一个进程占用
  exit /b 1
)
for %%F in ("%UE_RAG_STDIN%") do set "UE_RAG_STDIN=%%~dpF%%~nxF.consumed"
set "UE_RAG_STDIN_CLAIMED=1"
set "PYTHONIOENCODING=utf-8"
"%PY%" "%ROOT%\agent\run_windows.py" --plain < "%UE_RAG_STDIN%"
exit /b %ERRORLEVEL%

:multi_interactive
"%PY%" "%ROOT%\agent\run_windows.py"
if not "%~1"=="" exit /b %ERRORLEVEL%
goto menu

:resume
"%PY%" "%ROOT%\agent\run_windows.py" --resume latest
if not "%~1"=="" exit /b %ERRORLEVEL%
goto menu

:bye
echo 退出
endlocal
exit /b 0

:warn_live_runs
"%PY%" -c "from agent.run_guard import warn_live_runs; raise SystemExit(warn_live_runs())"
if not errorlevel 2 goto warn_ok
echo 有仍在心跳的 run。继续可能和它抢同一句需求。输入 Y 放行，其他键退出。
set /p GO=放行?
if /I "%GO%"=="Y" goto warn_ok
exit /b 1
:warn_ok
exit /b 0
