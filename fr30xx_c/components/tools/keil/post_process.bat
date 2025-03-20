@echo off

echo %DATE:~0,4%%DATE:~5,2%%DATE:~8,2%
echo %TIME%

set output_path=%cd%\output

:: project name
set project_name=%~1
:: input elf file path
set elf_path=%~2
:: compiler include file path
set compiler_include_path=%~3
:: this script path
set bat_script_path=%~0

if "%project_name%" == "" goto parameter_error
if "%elf_path%" == "" goto parameter_error
if "%compiler_include_path%" == "" goto parameter_error

::echo %output_path%
::echo %project_name%
::echo %elf_path%
::echo %compiler_include_path:~0,-8%

set fromelf_cmd=%compiler_include_path:~0,-8%\bin\fromelf.exe
set output_prefix=%output_path%\%project_name%

"%fromelf_cmd%" --text -c -o "%output_prefix%.txt" "%elf_path%"
"%fromelf_cmd%" --vhx --32X1  -c -o "%output_prefix%.hex" "%elf_path%"
"%fromelf_cmd%" --bin -o "%output_prefix%.bin" "%elf_path%"

set python_script_path=%bat_script_path:~0,-22%\post_process.py

python "%python_script_path%" "%project_name%" "%output_path%"
exit /b 0

:parameter_error
echo "missing input parameters"
exit /b 1
