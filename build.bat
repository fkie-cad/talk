@echo off

set prog_name=%~n0
set uprog_dir="%~dp0"
set verbose=1

set /a prog=0

set /a debug=0
set /a release=0
set /a bitness=64
set /a pdb=0
set /a debug_print=0
set /a rtl=0
set platform=x64
set configuration=Debug

set prog_proj=talk.vcxproj

set /a DP_FLAG=1
set /a EP_FLAG=2
set /a IP_FLAG=4




GOTO :ParseParams

:ParseParams

    REM IF "%~1"=="" GOTO Main
    if [%1]==[/?] goto help
    if [%1]==[/h] goto help
    if [%1]==[/help] goto help

    IF /i "%~1"=="/talk" (
        SET /a prog=1
        goto reParseParams
    )

    IF /i "%~1"=="/d" (
        SET /a debug=1
        goto reParseParams
    )
    IF /i "%~1"=="/r" (
        SET /a release=1
        goto reParseParams
    )

    IF /i "%~1"=="/dp" (
        SET /a "debug_print=%debug_print%|DP_FLAG"
        goto reParseParams
    )
    IF /i "%~1"=="/ep" (
        SET /a "debug_print=%debug_print%|EP_FLAG"
        goto reParseParams
    )
    IF /i "%~1"=="/ip" (
        SET /a "debug_print=%debug_print%|IP_FLAG"
        goto reParseParams
    )

    IF /i "%~1"=="/pdb" (
        SET /a pdb=1
        goto reParseParams
    )

    IF /i "%~1"=="/b" (
        SET /a bitness=%~2
        SHIFT
        goto reParseParams
    ) ELSE (
        echo Unknown option : "%~1"
    )
    
    :reParseParams
    SHIFT
    if [%1]==[] goto main

GOTO :ParseParams


:main

set /a "s=%debug%+%release%"
if %s% == 0 (
    set /a debug=0
    set /a release=1
)

if %bitness% == 64 (
    set platform=x64
)
if %bitness% == 32 (
    set platform=x86
)
if not %bitness% == 32 (
    if not %bitness% == 64 (
        echo ERROR: Bitness /b has to be 32 or 64!
        EXIT /B 1
    )
)

set /a "s=%prog%"
if %s% == 0 (
    set /a prog=1
)

if %verbose% == 1 (
    echo prog: %prog%
    echo.
    echo debug: %debug%
    echo release: %release%
    echo bitness: %bitness%
    echo pdb: %pdb%
    echo dprint: %debug_print%
    echo rtl: %rtl%
)

if %prog%==1 call :build %prog_proj%

exit /B 0



:build
    SETLOCAL
        set proj=%~1
        if %debug%==1 call :buildEx %proj%,%platform%,Debug,%debug_print%,%rtl%,%pdb%
        if %release%==1 call :buildEx %proj%,%platform%,Release,%debug_print%,%rtl%,%pdb%
    ENDLOCAL
    
    EXIT /B %ERRORLEVEL%
    
:buildEx
    SETLOCAL
        set proj=%~1
        set platform=%~2
        set conf=%~3
        set /a dpf=%~4
        set rtl=%~5
        set pdb=%~6
        
        ::set /a "dp=%dpf%&DP_FLAG"
        set /a dp=%dpf%
        set /a "ep=%dpf%&EP_FLAG"
        
        if %dp% == 2 (
            set /a dp=0
        )
        if not %ep% == 0 (
            set /a ep=1
        )

        if %rtl% == 1 (
            set rtl=%conf%
        ) else (
            set rtl=None
        )

        echo build
        echo  - Project=%proj%
        echo  - Platform=%platform%
        echo  - Configuration=%conf%
        echo  - DebugPrint=%dp%
        echo  - RuntimeLib=%rtl%
        echo  - DebugPrint=%dp%
        echo  - ErrorPrint=%ep%
        echo  - pdb=%pdb%
        echo.
        
        msbuild %proj% /p:Platform=%platform% /p:Configuration=%conf% /p:DebugPrint=%dp% /p:ErrorPrint=%ep% /p:RuntimeLib=%rtl% /p:PDB=%pdb%
        echo.
        echo ----------------------------------------------------
        echo.
        echo.
    ENDLOCAL
    
    EXIT /B %ERRORLEVEL%


:usage
    echo Usage: %prog_name% [/talk] [/d] [/r] [/dp] [/ep] [/b 32^|64] [/pdb] [/rtl]
    echo Default: %prog_name% [/talk /r /b 64]
    exit /B 0
    
:help
    call :usage
    echo.
    echo Targets:
    echo /talk: Build Talk.
    echo.
    echo Options:
    echo /d: Build in debug mode.
    echo /r: Build in release mode.
    echo /b: Bitness of exe. 32^|64. Default: 64.
    echo /dp: Debug print flag.
    echo /ep: Error print flag.
    echo /pdb: Compile with pdbs.
    echo /rtl: Compile with RuntimeLibrary.
    echo.
    echo /h: Print this.
    echo.
    exit /B 0