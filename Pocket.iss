[Setup]
AppId={{D3B3E5C1-7A9B-4D8E-B7C2-1F8E8E8E8E8E}
AppName=Pocket
AppVersion=1.0.0.1
DefaultDirName={autopf}\Pocket
DisableProgramGroupPage=yes
OutputBaseFilename=PocketSetup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
SetupIconFile=Pocket.ico
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "x64\Release\Pocket.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Pocket.ico"; DestDir: "{app}"

[Icons]
Name: "{autoprograms}\Pocket"; Filename: "{app}\Pocket.exe"; IconFilename: "{app}\Pocket.ico"
Name: "{autodesktop}\Pocket"; Filename: "{app}\Pocket.exe"; Tasks: desktopicon; IconFilename: "{app}\Pocket.ico"

[Code]
function NeedsNpcap(): Boolean;
begin
  Result := not (RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\Npcap') or
                RegKeyExists(HKEY_LOCAL_MACHINE, 'SOFTWARE\WOW6432Node\Npcap'));
end;
