[Setup]
AppName=Menace Audio
AppVersion=1.0.0
AppPublisher=Menace Audio
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=MenaceAudio-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma
SolidCompression=yes

[Files]
Source: "build\MenaceAudio_artefacts\Release\VST3\Menace Audio.vst3\*"; DestDir: "{commoncf64}\VST3\Menace Audio.vst3"; Flags: recursesubdirs ignoreversion
