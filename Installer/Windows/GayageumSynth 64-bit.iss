#define ApplicationVersion GetVersionNumbersString('..\..\Builds\VisualStudio2022\x64\Release\Standalone Plugin\GayageumSynth.exe')

[Setup]
AppName=GayageumSynth
AppVersion={#ApplicationVersion}
DefaultDirName={commoncf}
DefaultGroupName=GayageumSynth
OutputBaseFilename=GayageumSynth-{#ApplicationVersion}-Windows-64bit
DisableDirPage=yes
ArchitecturesInstallIn64BitMode=x64
ArchitecturesAllowed=x64

[Files]
Source: "..\..\Builds\VisualStudio2022\x64\Release\VST3\GayageumSynth.vst3\*"; DestDir: "{code:GetDir|0}\GayageumSynth.vst3\"; Flags: recursesubdirs

Source: "..\..\Builds\VisualStudio2022\x64\Release\Standalone Plugin\GayageumSynth.exe"; DestDir: "{code:GetDir|1}"

[Icons]
Name: "{group}\GayageumSynth"; Filename: "{code:GetDir|1}\GayageumSynth.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall GayageumSynth"; Filename: "{uninstallexe}"

[Code]
var
  DirPage: TInputDirWizardPage;

function GetDir(Param: String): String;
begin
  Result := DirPage.Values[StrToInt(Param)];
end;

procedure InitializeWizard;
begin
  { create a directory input page }
  DirPage := CreateInputDirPage(
    wpSelectDir, 'Install GayageumSynth', '', 'Choose installation locations', False, '');
  { add directory input page items }
  DirPage.Add('VST3 plugin');
  DirPage.Add('Stand-alone app');
  { assign default directories for the items from the previously stored data; if }
  { there are no data stored from the previous installation, use default folders }
  { of your choice }
  DirPage.Values[0] := GetPreviousData('Directory1', ExpandConstant('{commonpf}\Common Files\VST3'));
  DirPage.Values[1] := GetPreviousData('Directory2', ExpandConstant('{commonpf}\GayageumSynth'));
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  { store chosen directories for the next run of the setup }
  SetPreviousData(PreviousDataKey, 'Directory1', DirPage.Values[0]);
  SetPreviousData(PreviousDataKey, 'Directory2', DirPage.Values[1]);
end;
