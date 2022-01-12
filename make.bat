@ECHO OFF
ECHO.

SET SOLUTION=godtify.sln

IF "%~1"=="" GOTO COMMANDS
IF "%~1"=="all" GOTO MAKE_ALL
IF "%~1"=="release" GOTO MAKE_RELEASE
IF "%~1"=="debug" GOTO MAKE_DEBUG
IF "%~1"=="clean" GOTO MAKE_CLEAN

:COMMANDS
ECHO COMMANDS
GOTO DONE

:MAKE_ALL
call make.bat debug
call make.bat release
GOTO DONE

:MAKE_RELEASE
mkdir build\release
cd build\release
cmake -A x64 -DCMAKE_BUILD_TYPE=Release ..\..
where /q msbuild
IF ERRORLEVEL 1 (
    ECHO No developer tools found. Is this the Developer Command Prompt?
) ELSE (
    msbuild %SOLUTION% /p:Configuration=Release;OutDir=..\..\bin\Release\ -maxcpucount
)
cd ..
cd ..
GOTO DONE

:MAKE_DEBUG
ECHO MAKE_DEBUG
mkdir build\debug
cd build\debug
cmake -A x64 -DCMAKE_BUILD_TYPE=Debug ..\..
where /q msbuild
IF ERRORLEVEL 1 (
    ECHO No developer tools found. Is this the Developer Command Prompt?
) ELSE (
    msbuild %SOLUTION% /p:Configuration=Debug;OutDir=..\..\bin\Debug\;OutputPath=..\..\bin\Debug\ -maxcpucount
)
cd ..
cd ..
GOTO DONE

:MAKE_CLEAN
rmdir /s /Q build\debug
rmdir /s /Q build\release
rmdir /s /Q bin\debug
rmdir /s /Q bin\release
GOTO DONE

:DONE
