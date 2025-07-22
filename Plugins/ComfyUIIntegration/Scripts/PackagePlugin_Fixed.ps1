# ComfyUI Integration Plugin ����ű�

param(
    [string]$UEPath = "E:\ue\UE_5.5",
    [string]$ProjectPath = "C:\UnrealProjects\ComfyUITestProject", 
    [string]$OutputPath = "C:\UnrealProjects\ComfyUITestProject\Releases",
    [string]$Version = "0.8.0-beta"
)

Write-Host "================================" -ForegroundColor Green
Write-Host "ComfyUI Integration Plugin ����ű�" -ForegroundColor Green  
Write-Host "�汾: v$Version" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""

# ���·��
Write-Host "��黷��..." -ForegroundColor Yellow
if (!(Test-Path $UEPath)) {
    Write-Host "����: UE5·�������� $UEPath" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $ProjectPath)) {
    Write-Host "����: ��Ŀ·�������� $ProjectPath" -ForegroundColor Red
    exit 1
}

$PluginPath = "$ProjectPath\Plugins\ComfyUIIntegration"
if (!(Test-Path $PluginPath)) {
    Write-Host "����: ���·�������� $PluginPath" -ForegroundColor Red
    exit 1
}

Write-Host "�������ͨ��" -ForegroundColor Green

# 1. ����������
Write-Host ""
Write-Host "���� 1: ����������..." -ForegroundColor Yellow
$BinariesPath = "$PluginPath\Binaries"
$IntermediatePath = "$PluginPath\Intermediate"

if (Test-Path $BinariesPath) {
    Remove-Item $BinariesPath -Recurse -Force
    Write-Host "  ���� Binaries Ŀ¼" -ForegroundColor Gray
}
if (Test-Path $IntermediatePath) {
    Remove-Item $IntermediatePath -Recurse -Force  
    Write-Host "  ���� Intermediate Ŀ¼" -ForegroundColor Gray
}
Write-Host "�������" -ForegroundColor Green

# 2. ������
Write-Host ""
Write-Host "���� 2: ������..." -ForegroundColor Yellow
Write-Host "  �������Ҫ������ʱ��..." -ForegroundColor Gray

$BuildCmd = "$UEPath\Engine\Build\BatchFiles\Build.bat"
$BuildArgs = @(
    "ComfyUITestProjectEditor",
    "Win64",
    "Development", 
    "`"$ProjectPath\ComfyUITestProject.uproject`"",
    "-waitmutex"
)

Write-Host "  ִ�б�������..." -ForegroundColor Gray
$BuildProcess = Start-Process -FilePath $BuildCmd -ArgumentList $BuildArgs -Wait -PassThru -NoNewWindow

if ($BuildProcess.ExitCode -ne 0) {
    Write-Host "����ʧ��! �˳�����: $($BuildProcess.ExitCode)" -ForegroundColor Red
    exit 1
}
Write-Host "����ɹ�" -ForegroundColor Green

# 3. �������Ŀ¼
Write-Host ""
Write-Host "���� 3: ׼�����Ŀ¼..." -ForegroundColor Yellow
if (!(Test-Path $OutputPath)) {
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
    Write-Host "  ���������Ŀ¼: $OutputPath" -ForegroundColor Gray
}

$ReleaseDir = "$OutputPath\ComfyUIIntegration-v$Version"
if (Test-Path $ReleaseDir) {
    Remove-Item $ReleaseDir -Recurse -Force
    Write-Host "  ����ɵķ���Ŀ¼" -ForegroundColor Gray
}
New-Item -ItemType Directory -Path $ReleaseDir -Force | Out-Null
Write-Host "���Ŀ¼׼�����" -ForegroundColor Green

# 4. ���Ʋ���ļ�
Write-Host ""
Write-Host "���� 4: ���Ʋ���ļ�..." -ForegroundColor Yellow
$DestPath = "$ReleaseDir\ComfyUIIntegration"

# ֱ�Ӹ����������Ŀ¼
Write-Host "  ���Ʋ��Ŀ¼..." -ForegroundColor Gray
Copy-Item -Path $PluginPath -Destination $DestPath -Recurse -Force

# ɾ������Ҫ���ļ�
$ItemsToRemove = @(
    "$DestPath\Intermediate",
    "$DestPath\.vs",
    "$DestPath\*.tmp",
    "$DestPath\*.log",
    "$DestPath\*.suo",
    "$DestPath\*.user"
)

foreach ($Item in $ItemsToRemove) {
    if (Test-Path $Item) {
        Remove-Item $Item -Recurse -Force
        Write-Host "  �Ƴ�: $(Split-Path $Item -Leaf)" -ForegroundColor Gray
    }
}

Write-Host "�ļ��������" -ForegroundColor Green

# 5. ��֤�ؼ��ļ�
Write-Host ""
Write-Host "���� 5: ��֤�ļ�������..." -ForegroundColor Yellow
$RequiredFiles = @(
    "$DestPath\ComfyUIIntegration.uplugin",
    "$DestPath\Source\ComfyUIIntegration\ComfyUIIntegration.Build.cs",
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

# 6. ������װ�ű�
Write-Host ""
Write-Host "���� 6: ������װ�ű�..." -ForegroundColor Yellow
$InstallScript = @"
@echo off
chcp 65001 >nul
echo ================================
echo ComfyUI Integration Plugin v$Version
echo �Զ���װ�ű�
echo ================================
echo.

set /p "ProjectPath=����������UE5��Ŀ·��: "

if not exist "%ProjectPath%" (
    echo.
    echo [����] ��Ŀ·�������ڣ�
    echo ����·���Ƿ���ȷ��
    echo.
    pause
    exit /b 1
)

if not exist "%ProjectPath%\*.uproject" (
    echo.
    echo [����] ָ��Ŀ¼��û���ҵ�.uproject�ļ�
    echo ��ȷ��������ȷ��UE5��ĿĿ¼��
    echo.
)

if not exist "%ProjectPath%\Plugins" (
    echo ���� Plugins Ŀ¼...
    mkdir "%ProjectPath%\Plugins"
)

echo.
echo ���ڰ�װ���...
xcopy /s /y /i "ComfyUIIntegration" "%ProjectPath%\Plugins\ComfyUIIntegration\"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [����] �ļ�����ʧ�ܣ�
    pause
    exit /b 1
)

echo.
echo ================================
echo ��װ��ɣ�
echo ================================
echo.
echo �������Ĳ��裺
echo 1. �Ҽ������Ŀ�� .uproject �ļ�
echo 2. ѡ�� "Generate Visual Studio project files"
echo 3. ���±�����Ŀ�������Ҫ��
echo 4. �����������༭��
echo 5. �� Edit ^> Plugins ������ "ComfyUI Integration"
echo.
echo ʹ�÷�����
echo Tools ^> ComfyUI Integration
echo.
pause
"@

$InstallScript | Out-File "$ReleaseDir\Install.bat" -Encoding UTF8

# 7. �������� README
Write-Host ""
Write-Host "���� 7: ���������ĵ�..." -ForegroundColor Yellow
$BuildDate = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$ReleaseReadme = @"
# ComfyUI Integration Plugin v$Version

**����ʱ��**: $BuildDate  
**֧�ְ汾**: Unreal Engine 5.3+  
**ƽ̨֧��**: Windows 10/11  

## ���ٰ�װ

### �Զ���װ���Ƽ���
1. ˫������ Install.bat
2. ��������UE5��Ŀ����·��
3. ������ʾ��ɰ�װ

### �ֶ���װ
1. �� ComfyUIIntegration �ļ��и��Ƶ���Ŀ�� Plugins Ŀ¼
2. �Ҽ���Ŀ�� .uproject �ļ���ѡ�� "Generate Visual Studio project files"  
3. ���±�����Ŀ������Ҫ��
4. �����������༭��

## ϵͳҪ��

- Unreal Engine 5.3+
- Windows 10/11 (64λ)
- ComfyUI ����

## ���ٿ�ʼ

1. ����ComfyUI���� (Ĭ��: http://127.0.0.1:8188)
2. ��UE5��: Tools > ComfyUI Integration
3. ���÷�������ַ����ʼʹ�ã�

---
�汾: v$Version | ����: $BuildDate
"@

$ReleaseReadme | Out-File "$ReleaseDir\README.md" -Encoding UTF8

# 8. ����ѹ����
Write-Host ""
Write-Host "���� 8: �����ַ���..." -ForegroundColor Yellow
$ZipPath = "$OutputPath\ComfyUIIntegration-v$Version-Win64.zip"
if (Test-Path $ZipPath) {
    Remove-Item $ZipPath -Force
    Write-Host "  �Ƴ��ɵ�ѹ����" -ForegroundColor Gray
}

Write-Host "  ����ѹ���ļ�..." -ForegroundColor Gray
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::CreateFromDirectory($ReleaseDir, $ZipPath)

$ZipSizeMB = [math]::Round((Get-Item $ZipPath).Length / 1MB, 2)
Write-Host "ѹ����������� ($($ZipSizeMB) MB)" -ForegroundColor Green

# 9. �����ļ�У���
Write-Host ""
Write-Host "���� 9: �����ļ�У��..." -ForegroundColor Yellow
$Hash = Get-FileHash $ZipPath -Algorithm SHA256
$ChecksumFile = "$OutputPath\ComfyUIIntegration-v$Version-Win64.sha256"
$Hash.Hash | Out-File $ChecksumFile -Encoding ASCII
Write-Host "У����������" -ForegroundColor Green

# 10. ������ʱ�ļ�
Write-Host ""
Write-Host "���� 10: ������ʱ�ļ�..." -ForegroundColor Yellow
Remove-Item $ReleaseDir -Recurse -Force
Write-Host "��ʱ�ļ��������" -ForegroundColor Green

# ��ɱ���
Write-Host ""
Write-Host "================================" -ForegroundColor Green
Write-Host "������!" -ForegroundColor Green
Write-Host "================================" -ForegroundColor Green
Write-Host ""
Write-Host "����ļ�:" -ForegroundColor Cyan
Write-Host "������: $ZipPath" -ForegroundColor White
Write-Host "У���: $ChecksumFile" -ForegroundColor White
Write-Host ""
Write-Host "�ļ���С: $($ZipSizeMB) MB" -ForegroundColor Cyan
Write-Host "SHA256: $($Hash.Hash.Substring(0,16))..." -ForegroundColor Cyan
Write-Host ""
Write-Host "��һ��:" -ForegroundColor Yellow
Write-Host "1. �ڲ��Ի�������֤��װ��" -ForegroundColor White
Write-Host "2. ִ�������Ĳ�������" -ForegroundColor White  
Write-Host "3. ׼������֪ͨ�ͷַ�" -ForegroundColor White
Write-Host ""
Write-Host "ComfyUI Integration Plugin v$Version ��׼������!" -ForegroundColor Green
