@echo off
chcp 65001 >nul
cls
@echo off
echo ========================================
echo ComfyUI Integration Plugin v0.8.0-release
echo 自动安装脚本
echo ========================================
echo.

:INPUT_PATH
set /p "ProjectPath=请输入您的UE5项目路径: "

if not exist "%ProjectPath%" (
    echo.
    echo [错误] 项目路径不存在！
    echo 请检查路径是否正确。
    echo.
    goto INPUT_PATH
)

:: 检查是否为UE项目
if not exist "%ProjectPath%\*.uproject" (
    echo.
    echo [警告] 指定目录下没有找到.uproject文件
    set /p "Continue=是否继续安装？^(y/n^): "
    if /i not "%Continue%"=="y" goto INPUT_PATH
)

:: 检查UE版本兼容性
echo.
echo 检查项目兼容性...
for %%f in ("%ProjectPath%\*.uproject") do (
    findstr /C:"5.3" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.4" "%%f" >nul  
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.5" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
    findstr /C:"5.5" "%%f" >nul
    if not errorlevel 1 set UE_COMPATIBLE=1
)

if not defined UE_COMPATIBLE (
    echo [警告] 项目可能不兼容，建议使用UE 5.3+版本
    echo 是否继续安装？
    set /p "Continue=继续^(y^)还是退出^(n^): "
    if /i not "%Continue%"=="y" exit /b 0
)

:: 创建Plugins目录
if not exist "%ProjectPath%\Plugins" (
    echo 创建 Plugins 目录...
    mkdir "%ProjectPath%\Plugins"
)

:: 检查是否已安装旧版本
if exist "%ProjectPath%\Plugins\ComfyUIIntegration" (
    echo.
    echo 检测到已安装的插件版本
    set /p "Overwrite=是否覆盖安装？^(y/n^): "
    if /i not "%Overwrite%"=="y" (
        echo 安装已取消。
        pause
        exit /b 0
    )
    echo 正在移除旧版本...
    rmdir /s /q "%ProjectPath%\Plugins\ComfyUIIntegration"
)

echo.
echo ========================================
echo 正在安装插件...
echo ========================================
xcopy /s /y /i "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [错误] 文件复制失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 安装成功！
echo ========================================
echo.
echo ComfyUI Integration Plugin 已安装到您的项目中！
echo.
echo "接下来的步骤："
echo "1. 启动虚幻引擎编辑器"
echo "2. 打开您的项目"
echo "3. 在菜单栏找到：Tools ^> ComfyUI Integration"
echo.
echo 使用提示：
echo "- 确保ComfyUI服务正在运行 ^(默认: http://127.0.0.1:8188^)"
echo "- 查看用户指南: ComfyUIIntegration\BLUEPRINT_USER_GUIDE.md"
echo.
echo 注意：
echo 如果插件菜单没有出现，请到 Edit ^> Plugins 中启用 "ComfyUI Integration"
echo.
pause
