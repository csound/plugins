; Cabbage Installer Script, Copyright Rory Walsh 2017.

;swap this with plugin name
#define MyAppName "Csound Plugins"    
#define Manufacturer "Csound"
#define OPVERSION "1.0"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{0E50FA48-4C41-4D27-A742-C77D1A3A3905}
AppName={#MyAppName}
AppVersion={#OPVERSION}
DefaultDirName={localappdata}\csound\6.0\plugins64
DisableDirPage=no
DisableProgramGroupPage=yes
UsePreviousAppDir=no

;if you want to add a license..
;LicenseFile= "../LICENSE"

;swap this with plugin name                          
OutputBaseFilename=Csound6-Plugins-Windows_x86_64-{#OPVERSION}

;to add install icon
;SetupIconFile=icon.ico
Compression=lzma
SolidCompression=yes
ChangesEnvironment=yes


[Components]
Name: "core"; Description: "Install all plugins"; Types: full custom; Flags: fixed

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"

; add files - dlls go standard location, or one of the users choosing, csd and other reousrce go to program data
[Files]
Source: "../../build\AbletonLinkOpcodes\Release\ableton_link_opcodes.dll"; DestDir: "{app}\plugins64\"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "../../build\chua\Release\chua.dll"; DestDir: "{app}\plugins64\"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "../../build\faustcsound\Release\faustcsound.dll"; DestDir: "{app}\plugins64\"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "../../build\image\Release\image.dll"; DestDir: "{app}\plugins64\"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "../../build\py\Release\py.dll"; DestDir: "{app}\plugins64"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "../../build\widgets\Release\widgets.dll"; DestDir: "{app}\plugins64\"; Flags: ignoreversion; Components: core; Permissions: users-full;
Source: "C:\Program Files\Faust\lib\faust.dll"; DestDir: "{autopf64}\Csound6_x64\bin\"; Flags: ignoreversion; Components: core; Permissions: users-full;


[Tasks]
;Name: modifypath; Description: &Add application directory to your PATH environment variable (recommended); 

[Code]
var
  NewPage: TWizardPage;

const
  installerMessage =
'This installer will install the Python, Faust, Image, Chua, FLTK and Ableton Link ' + #13#10  +
'plugin opcodes for Csound.' + #13#10 + #13#10 +
'Opcodes will be installed to C:\Users\<name>\AppData\Local\csound\6.0\plugins64' + #13#10  +
'' + #13#10 + 
'Csound should detect and load these plugins whenever it runs. The Faust library will' + #13#10  +
'be installed to C:/Program Files/Csound_x64/bin. If you do not have Csound installed'  + #13#10  +
'at this location, please move the faust.dll file after installation';


procedure InitializeWizard();
var
  DescLabel: TNewStaticText ;
begin
  NewPage := CreateCustomPage(wpWelcome, 'Csound Plugins Installer',
    '') 
  DescLabel := TNewStaticText .Create(NewPage);
  DescLabel.AutoSize := True
  DescLabel.Width := NewPage.SurfaceWidth;
  DescLabel.WordWrap := True
  DescLabel.Parent := NewPage.Surface;
  DescLabel.Caption := installerMessage;
end;

