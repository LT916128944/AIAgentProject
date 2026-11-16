@echo off
rem 重定向了 stdin（UE_RAG_STDIN）时不 chcp：chcp 会丢弃已缓冲的重定向输入。
rem chcp 必须在任何中文之前：cmd 逐行按当前代码页解码，本文件存 UTF-8。
if not defined UE_RAG_STDIN chcp 65001 >nul
setlocal EnableExtensions

rem ============================================================================
rem AIAgentProject\start.bat -- 一键起入口页（双击即用）
rem
rem 人做 1 个动作：双击本文件。
rem 脚本做 2 步：
rem   1) 起 server\entry.py（127.0.0.1:8011）
rem   2) 端口真的 listen 之后由服务自己打开浏览器（entry.py::_open_when_ready）
rem
rem 为什么不是页面起服务：file:// 下的 HTML 没有拉起本地进程的 API，
rem 所以只能「服务起页面」，不能「页面起服务」。轮询端口那一步在 entry.py 里，
rem 本脚本不重复实现，避免两处各判一次就绪。
rem
rem 参数：
rem   nopause   跑完不暂停（脚本 / CI 用，与 shortcuts\cpp.bat 同名约定）
rem   其余参数原样交给 entry.py，例如：start.bat --no-open
rem                                   start.bat --port 8012
rem
rem 服务已在 8011 上跑着时：entry.py 不再抢端口，直接把页面打开并退 0。
rem ============================================================================

rem %~dp0 必须在 shift 之前取走：shift 会把 %0 一起左移（实测 shift 之后 %~dp0 变成
rem 当前工作目录），双击时 cwd 是 C:\Windows\System32，后面所有相对路径就全错。
for %%I in ("%~dp0..\RAG\ue-rag") do set "ROOT=%%~fI"
set "PY=%ROOT%\.venv\Scripts\python.exe"

set "NOPAUSE="
set "FORWARD="
set "PORT=8011"
:parse
if "%~1"=="" goto parsed
if /I "%~1"=="nopause" goto arg_nopause
if /I "%~1"=="--port" goto arg_port
rem 兼容 --port=8012 写法：argparse 两种都认。
set "A=%~1"
if /I "%A:~0,7%"=="--port=" goto arg_port_eq
set FORWARD=%FORWARD% %1
shift
goto parse
:arg_nopause
set "NOPAUSE=1"
shift
goto parse
:arg_port
rem --port 单独处理：端口由 PORT 统一传，避免 `--port 8011 --port 8012` 这种
rem 「靠 argparse 取最后一个」的隐性依赖；重复给时取最后一个，与 argparse 同口径。
if not "%~2"=="" set "PORT=%~2"
shift
shift
goto parse
:arg_port_eq
set "PORT=%A:~7%"
shift
goto parse
:parsed

if not exist "%ROOT%\server\entry.py" (
  echo [错误] 找不到 ue-rag 仓库：%ROOT%
  echo        两个仓库必须保持同级：
  echo            AIWork\AIAgentProject\   本仓库
  echo            AIWork\RAG\ue-rag\       https://github.com/LT916128944/ue-rag
  set "ERR=1"
  goto done
)

if not exist "%PY%" (
  echo [错误] 找不到虚拟环境：%PY%
  echo        换机或首次使用先跑 RAG\ue-rag\shortcuts\setup.ps1
  echo        它做三件事：找 Python 3.12、建 .venv、按 requirements.txt 装依赖。
  set "ERR=1"
  goto done
)

rem .env 缺了不拦：入口页的装配预览是纯函数，没有密钥也能用。
rem 真正会失败的是起 run（模型调用 401），那时 data/ui/logs 里写得很清楚。
if not exist "%ROOT%\.env" (
  echo [注意] 没有 %ROOT%\.env
  echo        复制 .env.example 为 .env，至少填 DEEPSEEK_API_KEY。
  echo        检索库没建过的话，再跑一次 RAG\ue-rag\shortcuts\ingest.bat
  echo        入口页可以先开，起 run 会因为没有密钥失败。
  echo.
)

cd /d "%ROOT%"
echo ============================================================================
echo  UE-RAG 入口页  http://127.0.0.1:%PORT%/   Ctrl+C 停止服务
echo  工作目录 %ROOT%
echo ============================================================================
"%PY%" "%ROOT%\server\entry.py" --port %PORT%%FORWARD%
set "ERR=%ERRORLEVEL%"
if not "%ERR%"=="0" (
  echo.
  echo [错误] 入口服务退出，退出码 %ERR%。
  echo        端口被别的程序占着时，换一个：start.bat --port 8012
)

:done
if defined NOPAUSE exit /b %ERR%
echo.
echo 按任意键关闭窗口。
pause >nul
exit /b %ERR%
