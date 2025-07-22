# ComfyUI Integration Plugin - ��ͼ�û�ר����С������ű�
param(
    [string]$UEPath = "E:\ue\UE_5.5",
    [string]$ProjectPath = "C:\UnrealProjects\ComfyUITestProject",
    [string]$OutputPath = "C:\UnrealProjects\ComfyUITestProject\Releases",
    [string]$Version = "0.8.0-beta"
)

Write-Host "================================" -ForegroundColor Green
Write-Host "ComfyUI Integration Plugin" -ForegroundColor Green
Write-Host "��ͼ�û�ר����С������ű�" -ForegroundColor Green  
Write-Host "�汾: v$Version" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""

# ���·��
$PluginPath = "$ProjectPath\Plugins\ComfyUIIntegration"
if (!(Test-Path $PluginPath)) {
    Write-Host "����: ���·�������� $PluginPath" -ForegroundColor Red
    exit 1
}

Write-Host "��ʼ������ͼ�û�ר�ð�..." -ForegroundColor Yellow

# 1. �������Ŀ¼
$ReleaseDir = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint"
if (Test-Path $ReleaseDir) {
    Remove-Item $ReleaseDir -Recurse -Force
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
$DestPath = "$ReleaseDir\ComfyUIIntegration"
New-Item -ItemType Directory -Path $DestPath -Force | Out-Null

Write-Host "���Ŀ¼�������" -ForegroundColor Green

# 2. ���Ʊ�Ҫ�ļ����ų�Դ�룩
Write-Host ""
Write-Host "��������ʱ��Ҫ�ļ�..." -ForegroundColor Yellow

# 2.1 ���Ʋ�������ļ�
Write-Host "  ���Ʋ������..." -ForegroundColor Gray
Copy-Item "$PluginPath\ComfyUIIntegration.uplugin" "$DestPath\" -Force

# 2.2 ���Ʊ����Ķ������ļ����ų������ļ���
Write-Host "  ���ƶ������ļ����ų�������Ϣ��..." -ForegroundColor Gray
if (Test-Path "$PluginPath\Binaries") {
    # ����BinariesĿ¼�ṹ
    New-Item -ItemType Directory -Path "$DestPath\Binaries" -Force | Out-Null
    
    # ��������Binaries�ļ������ų�.pdb�����ļ�
    Get-ChildItem "$PluginPath\Binaries" -Recurse | Where-Object {
        $_.Extension -ne ".pdb" -and $_.Name -notlike "*.pdb"
    } | ForEach-Object {
        $relativePath = $_.FullName.Substring($PluginPath.Length + "\Binaries\".Length)
        $destFile = Join-Path "$DestPath\Binaries" $relativePath
        $destDir = Split-Path $destFile -Parent
        
        if (!(Test-Path $destDir)) {
            New-Item -ItemType Directory -Path $destDir -Force | Out-Null
        }
        
        Copy-Item $_.FullName $destFile -Force
    }
    
    Write-Host "    ���ų������ļ� (.pdb)������Լ60MB" -ForegroundColor Green
} else {
    Write-Host "����: δ�ҵ������Ķ������ļ�!" -ForegroundColor Yellow
}

# 2.3 ����ContentĿ¼������У�
if (Test-Path "$PluginPath\Content") {
    Write-Host "  ����Content�ļ�..." -ForegroundColor Gray
    Copy-Item "$PluginPath\Content" "$DestPath\Content" -Recurse -Force
}

# 2.4 ����ConfigĿ¼������У�
if (Test-Path "$PluginPath\Config") {
    Write-Host "  ���������ļ�..." -ForegroundColor Gray
    Copy-Item "$PluginPath\Config" "$DestPath\Config" -Recurse -Force
}

# 2.5 ������ͼ�û�����ĵ�
Write-Host "  �����û��ĵ�..." -ForegroundColor Gray
$BlueprintDocs = @(
    "BLUEPRINT_USER_GUIDE.md"
)

foreach ($Doc in $BlueprintDocs) {
    $SourceDoc = "$PluginPath\$Doc"
    if (Test-Path $SourceDoc) {
        Copy-Item $SourceDoc "$DestPath\" -Force
    }
}

Write-Host "�ļ��������" -ForegroundColor Green

# 3. ������ͼ�û�ר�ð�װ�ű�
Write-Host ""
Write-Host "������װ�ű�..." -ForegroundColor Yellow

$InstallScript = @"
@echo off
chcp 65001 >nul
cls
@echo off
echo ========================================
echo ComfyUI Integration Plugin v$Version
echo �Զ���װ�ű�
echo ========================================
echo.

:INPUT_PATH
set /p "ProjectPath=����������UE5��Ŀ·��: "

if not exist "%ProjectPath%" (
    echo.
    echo [����] ��Ŀ·�������ڣ�
    echo ����·���Ƿ���ȷ��
    echo.
    goto INPUT_PATH
)

:: ����Ƿ�ΪUE��Ŀ
if not exist "%ProjectPath%\*.uproject" (
    echo.
    echo [����] ָ��Ŀ¼��û���ҵ�.uproject�ļ�
    set /p "Continue=�Ƿ������װ��^(y/n^): "
    if /i not "%Continue%"=="y" goto INPUT_PATH
)

:: ���UE�汾������
echo.
echo �����Ŀ������...
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
    echo [����] ��Ŀ���ܲ����ݣ�����ʹ��UE 5.3+�汾
    echo �Ƿ������װ��
    set /p "Continue=����^(y^)�����˳�^(n^): "
    if /i not "%Continue%"=="y" exit /b 0
)

:: ����PluginsĿ¼
if not exist "%ProjectPath%\Plugins" (
    echo ���� Plugins Ŀ¼...
    mkdir "%ProjectPath%\Plugins"
)

:: ����Ƿ��Ѱ�װ�ɰ汾
if exist "%ProjectPath%\Plugins\ComfyUIIntegration" (
    echo.
    echo ��⵽�Ѱ�װ�Ĳ���汾
    set /p "Overwrite=�Ƿ񸲸ǰ�װ��^(y/n^): "
    if /i not "%Overwrite%"=="y" (
        echo ��װ��ȡ����
        pause
        exit /b 0
    )
    echo �����Ƴ��ɰ汾...
    rmdir /s /q "%ProjectPath%\Plugins\ComfyUIIntegration"
)

echo.
echo ========================================
echo ���ڰ�װ���...
echo ========================================
xcopy /s /y /i "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [����] �ļ�����ʧ�ܣ�
    pause
    exit /b 1
)

echo.
echo ========================================
echo ��װ�ɹ���
echo ========================================
echo.
echo ComfyUI Integration Plugin �Ѱ�װ��������Ŀ�У�
echo.
echo "�������Ĳ��裺"
echo "1. �����������༭��"
echo "2. ��������Ŀ"
echo "3. �ڲ˵����ҵ���Tools ^> ComfyUI Integration"
echo.
echo ʹ����ʾ��
echo "- ȷ��ComfyUI������������ ^(Ĭ��: http://127.0.0.1:8188^)"
echo "- �鿴�û�ָ��: ComfyUIIntegration\BLUEPRINT_USER_GUIDE.md"
echo.
echo ע�⣺
echo �������˵�û�г��֣��뵽 Edit ^> Plugins ������ "ComfyUI Integration"
echo.
pause
"@

$InstallScript | Out-File "$ReleaseDir\һ����װ.bat" -Encoding UTF8

# 4. ������ͼ�û�ר��README
# Write-Host "����ר��˵���ĵ�..." -ForegroundColor Yellow

# $BlueprintReadme = @"
# # ComfyUI Integration Plugin v$Version
# ## ��ͼ�û�ר�ð汾

# ? **רΪ��ͼ��Ŀ�Ż��������汾**

# ### ? �ص�

# - ? **������**�����������б����ļ����������70%+
# - ? **���弴��**��������뻷�����ʺ�������ͼ������  
# - ? **��������**����������AI���ɹ��ܺ��µ���ק����
# - ? **һ����װ**������"��ͼ��Ŀһ����װ.bat"����

# ### ? 5�밲װ

# 1. **˫������**��`��ͼ��Ŀһ����װ.bat`
# 2. **����·��**������UE5��ͼ��Ŀ·��
# 3. **���**�������༭������ʹ��

# ### ? �¹�������

# **? ��ק�ʲ�����**
# - ֱ�Ӵ������������ק���������
# - ʵʱ�Ӿ�������������֤
# - �����������Ч��

# **? ����AI������**
# - ����ͼ (Text to Image)
# - ͼ��ͼ (Image to Image) 
# - ����3D��ͼ��3D
# - ��������

# ### ? ϵͳҪ��

# - ? **UE5��ͼ��Ŀ** (5.3+)
# - ? **Windows 10/11**
# - ? **ComfyUI����** (���ػ�Զ��)
# - ? **����Visual Studio����빤��**

# ### ? ʹ��ָ��

# - **��������**��BLUEPRINT_USER_GUIDE.md
# - **��ϸ��װ**��INSTALLATION_GUIDE.md
# - **����˵��**��RELEASE_NOTES_v$Version.md

# ### ? �汾�Ա�

# | ��Ŀ | ������ | ��ͼר�ð� |
# |------|--------|-----------|
# | �ļ���С | ~15MB | ~4MB |
# | ����Դ�� | ? | ? |
# | ��ͼ��Ŀ֧�� | ? | ? |
# | ���������� | 100% | 100% |
# | ��װ���Ӷ� | �е� | ���� |

# ### ? ���ٿ�ʼ

# 1. **����ComfyUI**: `python main.py --listen 127.0.0.1 --port 8188`
# 2. **��UE5**: Tools > ComfyUI Integration
# 3. **��ʼ����**: ѡ������ > ������ʾ�� > ���ɣ�

# ### ? ����

# ��ͼ�û��ķ����ر𱦹���������ǣ�
# - ��װ������Σ�
# - �����Ƿ���ã�  
# - ����Ҫʲô�Ľ���

# ---

# **? ��AIΪ������ͼ��Ŀע�봴�⣡**

# *�汾: v$Version-Blueprint | ����ʱ��: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")*
# "@

# $BlueprintReadme | Out-File "$ReleaseDir\README.md" -Encoding UTF8

# 5. ��֤��Ҫ�ļ�
Write-Host ""
Write-Host "��֤��װ��������..." -ForegroundColor Yellow

$RequiredFiles = @(
    "$DestPath\ComfyUIIntegration.uplugin",
    "$DestPath\Binaries\Win64\UnrealEditor-ComfyUIIntegration.dll"
)

$AllFilesExist = $true
foreach ($File in $RequiredFiles) {
    if (Test-Path $File) {
        Write-Host "  ����: $(Split-Path $File -Leaf)" -ForegroundColor Green
    } else {
        Write-Host "  ȱʧ: $(Split-Path $File -Leaf)" -ForegroundColor Red
        $AllFilesExist = $false
    }
}

if (!$AllFilesExist) {
    Write-Host "����: �ؼ��ļ�ȱʧ!" -ForegroundColor Red
    exit 1
}

# 6. ����ѹ����
Write-Host ""
Write-Host "������ͼר�÷�����..." -ForegroundColor Yellow

$ZipPath = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($ReleaseDir, $ZipPath)

$ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)

# 7. ����У���
$Hash = Get-FileHash $ZipPath -Algorithm SHA256
$ChecksumFile = "$OutputPath\ComfyUIIntegration-v$Version-Blueprint.sha256"
$Hash.Hash | Out-File $ChecksumFile -Encoding ASCII

# 8. ������ʱ�ļ�
Remove-Item $ReleaseDir -Recurse -Force

# ��ɱ���
Write-Host ""
Write-Host "================================" -ForegroundColor Green
Write-Host "��ͼר�ð汾������!" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""
Write-Host "? ����ļ�:" -ForegroundColor Cyan
Write-Host "��ͼר�ð�: $ZipPath" -ForegroundColor White
Write-Host "У���ļ�: $ChecksumFile" -ForegroundColor White
Write-Host ""
Write-Host "? ��С�Ա�:" -ForegroundColor Cyan
$FullPackageSize = if (Test-Path "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip") {
    [math]::Round((Get-Item "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip").Length / 1MB, 2)
} else { "δ֪" }
Write-Host "������: $($FullPackageSize) MB" -ForegroundColor Gray
Write-Host "��ͼ��: $($ZipSizeMB) MB" -ForegroundColor White
if ($FullPackageSize -ne "δ֪") {
    $ReductionPercent = [math]::Round((1 - $ZipSizeMB / $FullPackageSize) * 100, 1)
    Write-Host "����: $($ReductionPercent)%" -ForegroundColor Green
}
Write-Host ""
Write-Host "? ��ͼ�û����ڿ���������������װ����!" -ForegroundColor Green
