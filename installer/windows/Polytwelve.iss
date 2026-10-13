; Inno Setup script for the Windows installer.
; Build:  ISCC /DAppVersion=0.4.0 installer\windows\Polytwelve.iss
; (paths are relative to this file; expects a Release build in ..\..\build)

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif

#define Artefacts "..\..\build\Polytwelve_artefacts\Release"

[Setup]
AppId={{6E0F2C1A-7B3D-4C8E-9A51-2F4D8B6C1E70}
AppName=Polytwelve
AppVersion={#AppVersion}
AppVerName=Polytwelve {#AppVersion}
AppPublisher=Ticco Ross
DefaultDirName={autopf}\Ticco Ross\Polytwelve
DefaultGroupName=Ticco Ross
DisableDirPage=yes
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=Polytwelve-{#AppVersion}-Windows-Setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Polytwelve

[Types]
Name: "full";   Description: "VST3 plug-in and standalone app"
Name: "custom"; Description: "Choose components"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in (Ableton Live, Bitwig, Reaper, Cubase...)"; Types: full custom
Name: "app";  Description: "Standalone app";                                         Types: full custom

[Files]
Source: "{#Artefacts}\VST3\Polytwelve.vst3\*"; DestDir: "{commoncf64}\VST3\Polytwelve.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\Polytwelve.exe"; DestDir: "{app}"; Components: app; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Polytwelve"; Filename: "{app}\Polytwelve.exe"; Components: app

[Messages]
FinishedLabel=Polytwelve is installed.%n%nRescan plug-ins in your DAW (Ableton Live: Options > Preferences > Plug-Ins > Rescan).
