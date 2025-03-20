@echo off

echo %DATE:~0,4%%DATE:~5,2%%DATE:~8,2%
echo %TIME%

:: project name
set project_name=%~1
:: input elf file path
set output_path=%~2
:: compiler include file path
set compiler_include_path=%~3
:: this script path
set bat_script_path=%~0

if "%project_name%" == "" goto parameter_error
if "%output_path%" == "" goto parameter_error
if "%compiler_include_path%" == "" goto parameter_error

echo %project_name%
echo %output_path%
echo %compiler_include_path%

set ielfdumparm_cmd=%compiler_include_path%\arm\bin\ielfdumparm.exe
set output_prefix=%output_path%\%project_name%
set elf_path=%output_prefix%.out

ielfdumparm.exe "%elf_path%" "%output_prefix%.txt" -a --disasm_data
ielftool.exe "%elf_path%" "%output_prefix%.bin" --bin

set python_script_path=%bat_script_path:~0,-22%\post_process.py
set python_script_path=%output_path%\..\..\..\..\..\..\components\tools\post_process.py

python "%python_script_path%" "%project_name%" "%output_path%"
exit /b 0

:parameter_error
echo "missing input parameters"
exit /b 1
