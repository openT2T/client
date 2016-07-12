$saveProgressPreference = $ProgressPreference
$ProgressPreference = "SilentlyContinue"

$jxcoreLibUri = "https://github.com/jasongin/jxcore/releases/download/v0.3.1.2/libjxcore.zip"
$jxcoreDir = (Split-Path $MyInvocation.MyCommand.Path)
$jxcoreLibZip = "$jxcoreDir\libjxcore.zip"
$jxcoreLibDir = "$jxcoreDir\lib"

Write-Host "Downloading JXCore lib files from $jxcoreLibUri..."
Invoke-WebRequest -Uri $jxcoreLibUri -OutFile $jxcoreLibZip

Write-Host "Extracting JXCore lib files to $jxcoreLibDir..."
New-Item -ItemType "directory" -Path $jxcoreLibDir -Force > $null
Expand-Archive -Path $jxcoreLibZip -DestinationPath $jxcoreLibDir -Force

# Workaround missing Android mips lib file in the zip
New-Item -ItemType "directory" -Path "$jxcoreLibDir\Android\mips" -Force > $null
New-Item -ItemType "file" -Path "$jxcoreLibDir\Android\mips\libjxcore.so" -Force > $null

Write-Host "Done."

$ProgressPreference = $saveProgressPreference