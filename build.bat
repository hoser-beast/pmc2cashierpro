@echo off
setlocal enabledelayedexpansion
cd /D "%~dp0"

REM @TODO: do we need /Fm.....map???
REM @TODO: make debug and release version commandline switches

REM COMPILER FLAGS
REM    Diagnostics & warnings
REM -nologo = suppress copyright message
REM -diagnostics:column = print column information
REM -WL = one line diagnostics
REM -WX = warnings as errors
REM /W4 = warning level 4
REM /wd4100 unreferenced parameter
REM -FC = full path of source code in diagnostics

REM    Debugging
REM -Zo = generate richer debug information for optimized code
REM -Z7 = debug, fixes some compiler race conditions

REM    Optimization
REM -O2 = maximum optimization (favor speed)
REM -Oi = enable intrinsic functions
REM -favor:AMD64
REM -fp:fast = fast floating point model
REM -fp:except- = do not consider floating-point exceptions when generating code
REM -GS- = disable buffer overrun security checks (leaving this off)
REM -Gs9999999 = control the threshold for stack probes (leaving this off)
REM -MTd = debug multithreaded dll modules
REM -MT = relase multithreaded dll modules

REM    Miscellaneous
REM -Gm- = turn off incremental rebuild
REM -GR- = turn off runtime check information
REM -EHa- = turn off exception handling

REM LINKER FLAGS
REM -incremental:no = turn off incremental linking
REM -opt:ref = eliminate functions and data that are not referenced

set "BUILD_MODE=debug"

if "%~1"=="" (
	echo No mode specified. Building in debug mode.
) else if /I "%~1"=="debug" (
	set "BUILD_MODE=debug"
) else if /I "%~1"=="release" (
	set "BUILD_MODE=release"
) else (
	echo Invalid mode "%~1". Supported modes are debug or release.
	exit /b 1
)

if not exist build mkdir build

pushd build
del *.pdb > NUL 2> NUL

if "%BUILD_MODE%"=="debug" (
	echo Building in debug mode...
	set compiler_flags=-nologo -diagnostics:column -WL -WX -W4 -wd4100 -FC -Oi -MT -Z7 -DDEBUG=1 -favor:AMD64 -fp:fast -fp:except- -Gm- -GR- -EHa
) else if "%BUILD_MODE%"=="release" (
	echo Building in release mode...
	set compiler_flags=-nologo -diagnostics:column -WL -WX -W4 -wd4100 -FC -O2 -Oi -MT -favor:AMD64 -fp:fast -fp:except- -Gm- -GR- -EHa
)

set common_linker_flags=-incremental:no -opt:ref
cl ..\src\classes.c %compiler_flags% /link %common_linker_flags% /out:classes.exe
cl ..\src\crossreferences.c %compiler_flags% /link %common_linker_flags% /out:crossreferences.exe
cl ..\src\customers.c %compiler_flags% /link %common_linker_flags% /out:customers.exe
cl ..\src\history.c %compiler_flags% /link %common_linker_flags% /out:producthistory.exe

popd
exit /b
