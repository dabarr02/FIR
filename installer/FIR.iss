#define MyAppName "FIR"

#ifndef MyAppVersion
  #define MyAppVersion "0.1.0"
#endif

#ifndef SourceDir
  #define SourceDir "dist\FIR-v0.1.0"
#endif

#ifndef OutputDir
  #define OutputDir "dist"
#endif

[Setup]
AppId={{B3E4F68D-8B9B-4EE2-8D8E-1E8B1A1B9A30}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={localappdata}\FIR
DefaultGroupName={#MyAppName}
OutputDir={#OutputDir}
OutputBaseFilename=FIR-{#MyAppVersion}-setup
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
Compression=lzma
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\FIR.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\FIR.exe"; WorkingDir: "{app}"

[Run]
Filename: "{app}\FIR.exe"; Description: "Launch {#MyAppName}"; WorkingDir: "{app}"; Flags: postinstall nowait skipifsilent