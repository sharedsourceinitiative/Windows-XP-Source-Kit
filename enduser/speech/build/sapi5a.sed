[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=ExtractOnly
ShowInstallProgramWindow=0
HideExtractAnimation=0
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=I
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles
[Strings]
InstallPrompt=Install SAPI 5.0 runtime?
DisplayLicense=
FinishMessage=SAPI 5.0 runtime installation complete.
TargetName=D:\SAPI5\build\sapi5.EXE
FriendlyName=SAPI 5.0 runtime 
AppLaunched=
PostInstallCmd=
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="sapi5rt.inf"
FILE1="sapi.dll"
FILE2="spttseng.dll"
FILE3="mike.dll"
FILE4="voice.dll"
FILE5="ahd.gxc"
[SourceFiles]
SourceFiles0=D:\SAPI5\build\
SourceFiles1=D:\SAPI5\Src\SAPI\Release_Alpha\
SourceFiles2=D:\SAPI5\Src\TTS\msttsdrv\Release_Alpha\
SourceFiles3=D:\SAPI5\Src\TTS\mike\Release_Alpha\
SourceFiles4=D:\SAPI5\Src\TTS\mary\Release_Alpha\
SourceFiles5=D:\SAPI5\Bin\
[SourceFiles0]
%FILE0%=
[SourceFiles1]
%FILE1%=
[SourceFiles2]
%FILE2%=
[SourceFiles3]
%FILE3%=
[SourceFiles4]
%FILE4%=
[SourceFiles5]
%FILE5%=
