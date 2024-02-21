#include "ModBuilder.h"

#include "FileHelpers.h"
#include "ISettingsModule.h"
#include "ModdingEx.h"
#include "ModdingExSettings.h"
#include "Async/Async.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"
#include "Widgets/Notifications/SNotificationList.h"

// TODO: Make this async

bool UModBuilder::ExecGenericCommand(const TCHAR* Command, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr)
{
	void* OutputReadPipe = nullptr;
	void* OutputWritePipe = nullptr;
	FPlatformProcess::CreatePipe(OutputReadPipe, OutputWritePipe);
	FProcHandle Proc = FPlatformProcess::CreateProc(Command, Params, false, true, true, nullptr, -1, nullptr, OutputWritePipe, nullptr);

	if (!Proc.IsValid())
	{
		FPlatformProcess::ClosePipe(OutputReadPipe, OutputWritePipe);
		return false;
	}

	FString NewLine;
	FPlatformProcess::Sleep(0.01);

	while (FPlatformProcess::IsProcRunning(Proc))
	{
		NewLine = FPlatformProcess::ReadPipe(OutputReadPipe);
		if (NewLine.Len() > 0)
		{
			UE_LOG(LogModdingEx, Log, TEXT("COOK: %s"), *NewLine);
			NewLine.Empty();
		}

		FPlatformProcess::Sleep(0.25f);
	}

	int32 RC;
	FPlatformProcess::GetProcReturnCode(Proc, &RC);
	// if (OutStdOut)
	// {
	// 	*OutStdOut = FPlatformProcess::ReadPipe(OutputReadPipe);
	// }
	FPlatformProcess::ClosePipe(OutputReadPipe, OutputWritePipe);
	FPlatformProcess::CloseProc(Proc);
	if (OutReturnCode)
	{
		*OutReturnCode = RC;
	}

	return RC == 0;
}

FString UModBuilder::CreateFilesTxt(const FString& RootDir, const FString& TrackingDir)
{
	const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::Combine(RootDir, TrackingDir));

	FString FileContents;

	const FString& TmpFilePath = FPaths::CreateTempFilename(FPlatformProcess::UserTempDir(), TEXT("files"), TEXT(".txt"));

	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> Files;

	FileManager.FindFilesRecursive(Files, *Directory, TEXT("*.*"), true, false);

	for (const FString& FileName : Files)
	{
		FString RelPath = FileName;
		FPaths::MakePathRelativeTo(RelPath, *RootDir);

		auto str = FString::Printf(TEXT("\"%s\" \"../../../%s\" -compress\n"), *FileName, *RelPath);
		UE_LOG(LogModdingEx, Log, TEXT("Entry: %s"), *str);

		FileContents += str;
	}

	if (FFileHelper::SaveStringToFile(FileContents, *TmpFilePath))
	{
		UE_LOG(LogModdingEx, Log, TEXT("FilesTxt created successfully at: %s"), *TmpFilePath);
		return TmpFilePath;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Failed to create FilesTxt at: %s"), *TmpFilePath);
	return FString();
}

bool UModBuilder::Cook()
{	
	FString Args = FString::Printf(
		TEXT("\"%s\" -run=Cook -TargetPlatform=Windows -unversioned -stdout -CrashForUAT -unattended -NoLogTimes -UTF8Output"),
		*(FPaths::ProjectDir() / FApp::GetProjectName() + TEXT(".uproject")));
	UE_LOG(LogModdingEx, Log, TEXT("Args: %s"), *Args);

	int32 OutReturnCode = 0;
	FString OutStdOut;
	FString OutStdErr;
	if (!FPlatformProcess::ExecProcess(
		*(FPaths::EngineDir() / "Binaries/Win64/UnrealEditor-Cmd.exe"),
		*Args, &OutReturnCode, &OutStdOut, &OutStdErr))
	{
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Returned with: %d"), OutReturnCode);

	if (OutReturnCode == 0)
	{
		UE_LOG(LogModdingEx, Log, TEXT("%s"), *OutStdOut);
		return true;
	}

	UE_LOG(LogModdingEx, Error, TEXT("Err: %s"), *OutStdErr);
	return false;
}

bool UModBuilder::Pack(const FString& FilesPath, const FString& OutputPath)
{
	FString Args = FString::Printf(
		TEXT(
			"\"%s\" -patchpaddingalign=2048 -compressionformats=Oodle -compressmethod=Kraken -compresslevel=5 -platform=Windows -create=\"%s\" --compress"),
		*OutputPath,
		*FilesPath);
	UE_LOG(LogModdingEx, Log, TEXT("Args: %s"), *Args);

	int32 OutReturnCode = 0;
	FString OutStdOut;
	FString OutStdErr;
	if (!FPlatformProcess::ExecProcess(
		*FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries/Win64/UnrealPak.exe")),
		*Args, &OutReturnCode, &OutStdOut, &OutStdErr))
	{
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Returned with: %d"), OutReturnCode);

	if (OutReturnCode == 0)
	{
		UE_LOG(LogModdingEx, Log, TEXT("%s"), *OutStdOut);
		return true;
	}

	UE_LOG(LogModdingEx, Error, TEXT("Err: %s"), *OutStdErr);
	return false;
}

bool UModBuilder::GetOutputFolder(bool bIsLogicMod, FString& OutFolder)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	if (!Settings->CustomPakDir.Path.IsEmpty() && FPaths::DirectoryExists(Settings->CustomPakDir.Path))
	{
		OutFolder = Settings->CustomPakDir.Path;
		return true;
	}

	if (Settings->GameDir.Path.IsEmpty() || !FPaths::DirectoryExists(Settings->GameDir.Path))
	{
		UE_LOG(LogModdingEx, Error, TEXT("GameDir is not set or does not exist"));
		return false;
	}

	FString RelFolder = bIsLogicMod ? Settings->LogicModFolder : Settings->ContentModFolder;
	if (RelFolder.IsEmpty())
	{
		UE_LOG(LogModdingEx, Error, TEXT("LogicModFolder or ContentModFolder is not set"));
		return false;
	}

	RelFolder.ReplaceInline(TEXT("{GameName}"), FApp::GetProjectName());

	OutFolder = FPaths::Combine(Settings->GameDir.Path, RelFolder);

	if (!FPaths::DirectoryExists(OutFolder))
	{
		if (!IFileManager::Get().MakeDirectory(*OutFolder, true))
		{
			UE_LOG(LogModdingEx, Error, TEXT("Output folder could not be created: %s"), *OutFolder);
			return false;
		}

		UE_LOG(LogModdingEx, Log, TEXT("Output folder created: %s"), *OutFolder);
		//UE_LOG(LogModdingEx, Error, TEXT("Output folder does not exist: %s"), *OutFolder);
		// return false;
	}

	return true;
}

bool UModBuilder::BuildMod(const FString& ModName, bool bIsSameContentError)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	FString OutputDir;
	if (!GetOutputFolder(true, OutputDir))
	{
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(
			                         "Game dir is not set or does not exist. Should I take you to the setting?.")) == EAppReturnType::Yes)
		{
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "ModdingEx");
		}

		UE_LOG(LogModdingEx, Error, TEXT("Output dir could not be found"));
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Output dir: %s"), *OutputDir);

	const FString OutFileName = OutputDir / (ModName + ".pak");

	if(Settings->bSaveAllBeforeBuilding)
	{
		const bool bPromptUserToSave = false;
		const bool bSaveMapPackages = true;
		const bool bSaveContentPackages = true;
		const bool bFastSave = false;
		const bool bNotifyNoPackagesSaved = false;
		const bool bCanBeDeclined = false;
		FEditorFileUtils::SaveDirtyPackages( bPromptUserToSave, bSaveMapPackages, bSaveContentPackages, bFastSave, bNotifyNoPackagesSaved, bCanBeDeclined );
		UE_LOG(LogModdingEx, Log, TEXT("Saved all packages"));
	}

	FMD5Hash InputHash = FMD5Hash();
	if (Settings->bShouldCheckHash && FPaths::FileExists(OutFileName))
	{
		InputHash = FMD5Hash::HashFile(*OutFileName);
	}

	FScopedSlowTask SlowTask(4, FText::FromString(FString::Format(TEXT("Building {0} (this can take a while)"), {ModName})));
	SlowTask.MakeDialog();

	SlowTask.EnterProgressFrame(1, FText::FromString("Killing processes"));

	const TArray<FString>& ProcessesToKill = Settings->ProcessesToKill;
	FPlatformProcess::FProcEnumerator ProcIter;
	while (ProcIter.MoveNext())
	{
		FPlatformProcess::FProcEnumInfo ProcInfo = ProcIter.GetCurrent();
		const FString& Name = ProcInfo.GetName();
		// UE_LOG(LogModdingEx, Log, TEXT("Found process: %s"), *Name);
		if (ProcessesToKill.Contains(Name))
		{
			UE_LOG(LogModdingEx, Log, TEXT("Killing process: %s"), *Name);
			FProcHandle ProcHandle = FPlatformProcess::OpenProcess(ProcInfo.GetPID());
			FPlatformProcess::TerminateProc(ProcHandle);
			FPlatformProcess::CloseProc(ProcHandle);
		}
	}

	UE_LOG(LogModdingEx, Log, TEXT("Building mod"));

	const FString ModPath = FString("/Game") / "Mods" / ModName;
	EditDirectoriesToAlwaysCook(ModPath, false);	

	SlowTask.EnterProgressFrame(1, FText::FromString("Cooking mod"));
	const bool bCooked = Cook();

	// Remove the mod from the list of directories to always cook even if fail
	EditDirectoriesToAlwaysCook(ModPath, true);
	
	if (!bCooked)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Cooking failed"));
		return false;
	}

	SlowTask.EnterProgressFrame(1, FText::FromString("Tracking files to pack"));
	const FString& FilePath = CreateFilesTxt(
		FPaths::ProjectDir() / "Saved" / "Cooked" / "Windows" / FApp::GetProjectName(), FString("Content") / "Mods" / ModName);

	if (FilePath.IsEmpty())
	{
		return false;
	}

	SlowTask.EnterProgressFrame(1, FText::FromString("Packing mod"));
	if (!Pack(FilePath, OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Packing failed"));
		return false;
	}

	if (!FPaths::FileExists(OutFileName))
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Packing failed. Output file not present. Check logs for more info."));
		return false;
	}

	const FMD5Hash OutputHash = FMD5Hash::HashFile(*OutFileName);

	if (Settings->bShouldCheckHash && bIsSameContentError && InputHash == OutputHash)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Output file is the same as the input file. Either packing failed or you didn't change the content."));
		FMessageDialog::Open(EAppMsgType::Ok,
		                     FText::FromString(
			                     "Output file is the same as the input file. Either packing failed or you didn't change the content. Check logs for more info."));
		return false;
	}

	FNotificationInfo Info(FText::FromString("Mod built successfully!"));
	Info.Image = FAppStyle::GetBrush(TEXT("LevelEditor.RecompileGameCode"));
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 3.5f;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = true;
	Info.bUseLargeFont = true;
	Info.bFireAndForget = false;
	Info.bAllowThrottleWhenFrameRateIsLow = false;
	const auto NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
	NotificationItem->ExpireAndFadeout();

	GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));

	return true;
}

bool UModBuilder::PrepareModForRelease(const FString& ModName, const FString& WebsiteUrl, const FString& Dependencies)
{
	const auto Settings = GetDefault<UModdingExSettings>();
	
	if (Settings->bAlwaysBuildBeforePrep && !BuildMod(ModName, !Settings->bPrepModWhenContentIsSame))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to prepare mod for release because building failed"));
		return false;
	}
	
	// Fetch the mod author, description and version from the /Game/Mods/ModName/ModActor blueprint
	FString ModAuthor = "Unknown";
	FString ModDesc = "Unknown";
	FString ModVersion = "1.0.0";

	if (!GetModProperties(ModName, ModVersion, ModAuthor, ModDesc))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to get mod properties"));
		return false;
	}

	// Create the manifest.json and README.md using the mod properties, but do not overwrite if they already exist
	const FString StagingDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->PrepStagingDir.Path) / ModName;

	const FString ManifestPath = StagingDir / "manifest.json";
	FString ModManifest;
	CreateModManifest(ModManifest, ModName, WebsiteUrl, Dependencies, ModDesc, ModVersion);
	if (!FPaths::FileExists(ManifestPath))
	{
		if (!FFileHelper::SaveStringToFile(ModManifest, *ManifestPath))
		{
			UE_LOG(LogModdingEx, Error, TEXT("Failed to save manifest.json"));
			return false;
		}
	}

	const FString ReadmePath = StagingDir / "README.md";
	FString Readme;
	CreateModReadme(Readme, ModName, ModDesc, ModVersion, ModAuthor);
	
	if (!FPaths::FileExists(ReadmePath))
	{
		if (!FFileHelper::SaveStringToFile(Readme, *ReadmePath))
		{
			UE_LOG(LogModdingEx, Error, TEXT("Failed to save README.md"));
			return false;
		}
	}
	
	if (Settings->bOpenReadmeAfterPrep)
	{
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*ReadmePath, nullptr, ELaunchVerb::Edit);
	}

	// Copy TempModIcon.png from Resources folder to the staging dir
	const FString IconPath = StagingDir / "icon.png";
	const FString PluginIconPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), TEXT("Plugins/ModdingEx/Resources/TempModIcon.png"));

	if (!FPaths::FileExists(PluginIconPath))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Icon file does not exist: %s"), *PluginIconPath);
		return false;
	}
	
	if (!FPaths::FileExists(IconPath))
	{
		if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*IconPath, *PluginIconPath))
		{
			UE_LOG(LogModdingEx, Error, TEXT("Failed to copy icon file"));
			return false;
		}
	}

	FString OutFileName;
	if (!GetOutputPakDirectory(OutFileName, ModName))
	{
		return false;
	}

	if (!FPaths::FileExists(OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Output file does not exist: %s"), *OutFileName);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Didn't find any pak file to copy. Make sure you built the mod successfully."));
		return false;
	}

	// Copy pak file from OutFileName to the staging dir/pak/
	// There currently isn't any support for non-logic mods with Thunderstore, so this will have to be updated in the future when there is
	const FString PakDir = StagingDir / "pak";
	if (!FPaths::DirectoryExists(PakDir) && !IFileManager::Get().MakeDirectory(*PakDir, true))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Pak dir does not exist: %s"), *PakDir);
		return false;
	}

	if (!FPlatformFileManager::Get().GetPlatformFile().CopyFile(*FString(PakDir / ModName + ".pak"), *OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to copy pak file"));
		return false;
	}

	return true;
}

void UModBuilder::EditDirectoriesToAlwaysCook(const FString& DirectoryToCook, const bool bShouldRemove)
{
	const FString IniFilePath = FPaths::ProjectConfigDir() / "DefaultGame.ini";
	
	FString FileContents;
	FFileHelper::LoadFileToString(FileContents, *IniFilePath);
	
	TArray<FString> Lines;
	FileContents.ParseIntoArrayLines(Lines);

	const FString LineToModify = FString::Printf(TEXT("+DirectoriesToAlwaysCook=(Path=\"%s\")"), *DirectoryToCook);
	
	FString ModifiedContents;
	bool bSectionFound = false;

	for (FString& Line : Lines)
	{
		if (Line.Contains(TEXT("[/Script/UnrealEd.ProjectPackagingSettings]")))
		{
			bSectionFound = true;
		}
		else if (bSectionFound && Line.StartsWith(TEXT("[")))
		{
			if (!bShouldRemove)
			{
				ModifiedContents += LineToModify + TEXT("\n");
			}
			bSectionFound = false;
		}
		
		if (bShouldRemove && bSectionFound && Line.TrimStart().Equals(LineToModify))
		{
			continue;
		}
		
		ModifiedContents += Line + TEXT("\n");
	}

	if (bSectionFound && !bShouldRemove)
	{
		ModifiedContents += LineToModify + TEXT("\n");
	}

	const bool bSuccess = FFileHelper::SaveStringToFile(ModifiedContents, *IniFilePath);

	if (!bSuccess)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to save file: %s"), *IniFilePath);
	}
	else
	{
		UE_LOG(LogModdingEx, Log, TEXT("File saved successfully: %s"), *IniFilePath);
	}
}

bool UModBuilder::GetOutputPakDirectory(FString& OutDirectory, const FString& ModName)
{
	FString OutputDir;
	// TODO: Support non-logic mods and remove this hardcoded terribleness
	if (!GetOutputFolder(true, OutputDir))
	{
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(
									 "Game dir is not set or does not exist. Should I take you to the setting?.")) == EAppReturnType::Yes)
		{
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "ModdingEx");
		}

		UE_LOG(LogModdingEx, Error, TEXT("Output dir could not be found"));
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Output dir: %s"), *OutputDir);

	OutDirectory = OutputDir / (ModName + ".pak");
	return true;
}

bool UModBuilder::ZipModBasic(const FString& ModName, const FString& ModManager)
{
	FString OutFileName;
	if (!GetOutputPakDirectory(OutFileName, ModName))
	{
		return false;
	}

	if (!FPaths::FileExists(OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Output file does not exist: %s"), *OutFileName);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Didn't find anything to zip. Make sure you built the mod."));
		return false;
	}

	TArray<FString> FilesToZip;
	FilesToZip.Add(OutFileName);

	if (!ZipModInternal(ModName, FilesToZip, ModManager, FPaths::GetPath(OutFileName)))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to zip mod"));
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Zipped mod %s"), *ModName);

	return true;
}

bool UModBuilder::ZipModStaging(const FString& ModName, const FString& ModManager)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	const FString StagingDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->PrepStagingDir.Path) / ModName;
	if (!FPaths::DirectoryExists(StagingDir))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Staging dir does not exist: %s"), *StagingDir);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Staging dir does not exist. Make sure you prepared the mod for release first!"));
		return false;
	}
	
	const FString PakPath = StagingDir / "pak" / (ModName + ".pak");
	if (!FPaths::FileExists(PakPath))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Pak file does not exist: %s"), *PakPath);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Pak file does not exist. Make sure you prepared the mod for release first!"));
		return false;
	}
	
	TArray<FString> FilesToZip;
	IFileManager::Get().FindFilesRecursive(FilesToZip, *StagingDir, TEXT("*.*"), true, false);
	
	if (!ZipModInternal(ModName, FilesToZip, ModManager, *StagingDir))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to zip mod"));
		return false;
	}

	UE_LOG(LogModdingEx, Log, TEXT("Zipped mod %s"), *ModName);
	
	return true;
}

bool UModBuilder::ZipModInternal(const FString& ModName, const TArray<FString>& FilesToZip, const FString& ModManager,
                                 const FString& CommonDirectory)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	FString ZipsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->ModZipDir.Path + "/" + ModManager);
	if (ModManager == "None")
	{
		ZipsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->ModZipDir.Path);
	}
	
	if (!FPaths::DirectoryExists(ZipsPath) && !IFileManager::Get().MakeDirectory(*ZipsPath, true))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Zips dir does not exist: %s"), *ZipsPath);
		if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(
			                         "Zip dir does not exist. Should I take you to the setting?.")) == EAppReturnType::Yes)
		{
			FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "ModdingEx");
		}
		return false;
	}

	const FString ZipFilePath = ZipsPath / (ModName + ".zip");

	UE_LOG(LogModdingEx, Log, TEXT("Zipping mod to %s"), *ZipFilePath);

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* ZipFile = PlatformFile.OpenWrite(*ZipFilePath);

	if (!ZipFile)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to open zip file for writing: %s"), *ZipFilePath);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Failed to open zip file for writing."));
		return false;
	}

	FZipArchiveWriter ZipWriter(ZipFile);
	for (const FString& FileName : FilesToZip)
	{
		IFileHandle* FileHandle = PlatformFile.OpenRead(*FileName);
		if (!FileHandle)
		{
			UE_LOG(LogModdingEx, Error, TEXT("Failed to open file for reading: %s"), *FileName);
			continue;
		}
		
		FString ZipPath = FileName;
		FPaths::MakePathRelativeTo(ZipPath, *CommonDirectory);
		int32 Index;
		if (ZipPath.FindChar(TEXT('/'), Index))
		{
			ZipPath = ZipPath.Mid(Index + 1);
		}
		
		const int64 FileSize = FileHandle->Size();
		TArray<uint8> FileData;
		FileData.SetNumUninitialized(FileSize);
		FileHandle->Read(FileData.GetData(), FileSize);
		
		ZipWriter.AddFile(ZipPath, FileData, FDateTime::Now());
		
		delete FileHandle;
	}

	FNotificationInfo Info(FText::FromString("Mod zipped successfully!"));
	Info.Image = FAppStyle::GetBrush(TEXT("LevelEditor.RecompileGameCode"));
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 3.5f;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = true;
	Info.bUseLargeFont = true;
	Info.bFireAndForget = false;
	Info.bAllowThrottleWhenFrameRateIsLow = false;
	const auto NotificationItem = FSlateNotificationManager::Get().AddNotification(Info);
	NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
	NotificationItem->ExpireAndFadeout();

	GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));

	if(Settings->bOpenZipFolderAfterZipping)
		FPlatformProcess::ExploreFolder(*FPaths::ConvertRelativePathToFull(ZipsPath));

	return true;
}

void UModBuilder::CreateModManifest(FString& OutModManifest, const FString& ModName, const FString& WebsiteUrl,
                                    const FString& Dependencies, const FString& ModDesc, const FString& ModVersion)
{
	/*
	 * Format:
	*	{
			"name": "HelloWorld",
			"version_number": "1.0.0",
			"description": "Hello palworld!",
			"website_url": "https://github.com/thunderstore-io",
			"dependencies": [
				"Thunderstore-unreal_shimloader-1.0.2"
			]
		}
	*/

	// Turn Dependencies (which is a comma separated list of mod names) into a json array
	TArray<TSharedPtr<FJsonValue>> Deps;
	TArray<FString> DepsArray;
	Dependencies.ParseIntoArray(DepsArray, TEXT(","), true);
	for (const FString& Dep : DepsArray)
	{
		Deps.Add(MakeShareable(new FJsonValueString(Dep)));
	}

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField("name", ModName);
	JsonObject->SetStringField("version_number", ModVersion);
	JsonObject->SetStringField("description", ModDesc);
	JsonObject->SetStringField("website_url", WebsiteUrl);
	JsonObject->SetArrayField("dependencies", Deps);
	
	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	OutModManifest = JsonString;
}

void UModBuilder::CreateModReadme(FString& OutModReadme, const FString& ModName, const FString& ModDesc,
	const FString& ModVersion, const FString& ModAuthor)
{
	OutModReadme = FString::Printf(TEXT("# %s\n\n%s\n\n## Version: %s\n\n## Author: %s"), *ModName, *ModDesc, *ModVersion, *ModAuthor);
}

bool UModBuilder::GetModProperties(const FString& ModName, FString& OutModVersion, FString& OutModAuthor,
                                   FString& OutModDescription)
{
	const FString ModActorPath = FString("/Game") / "Mods" / ModName / "ModActor";
	const FString ModActorClassPath = ModActorPath + ".ModActor";

	const UBlueprint* ModActorBlueprint = LoadObject<UBlueprint>(nullptr, *ModActorClassPath);
	if (!ModActorBlueprint)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to load ModActor blueprint: %s"), *ModActorClassPath);
		return false;
	}
	
	const UClass* ModActorClass = ModActorBlueprint->GeneratedClass;
	if (!ModActorClass)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Invalid generated class for blueprint: %s"), *ModActorClassPath);
		return false;
	}

	// Temporary instance of the blueprint class to access variable values
	UObject* ModActorInstance = NewObject<UObject>(GetTransientPackage(), ModActorClass);
	if (!ModActorInstance)
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to create an instance of the ModActor class."));
		return false;
	}
	
	if (FString* FoundModAuthor = FindFStringPropertyValue(ModActorInstance, "ModAuthor"))
	{
		OutModAuthor = *FoundModAuthor;
	}

	if (FString* FoundModDesc = FindFStringPropertyValue(ModActorInstance, "ModDescription"))
	{
		OutModDescription = *FoundModDesc;
	}

	if (FString* FoundModVersion = FindFStringPropertyValue(ModActorInstance, "ModVersion"))
	{
		OutModVersion = *FoundModVersion;
	}
	
	UE_LOG(LogModdingEx, Log, TEXT("Mod Author: %s, Description: %s, Version: %s"),
		*OutModAuthor, *OutModDescription, *OutModVersion);
	
	ModActorInstance->ConditionalBeginDestroy();

	return true;
}

FString* UModBuilder::FindFStringPropertyValue(UObject* Object, const FName& PropertyName)
{
	if (!Object) return nullptr;
	
	const FStrProperty* StringProp = FindFProperty<FStrProperty>(Object->GetClass(), PropertyName);
	if (!StringProp) return nullptr;
	
	return StringProp->ContainerPtrToValuePtr<FString>(Object);
}

bool UModBuilder::ZipMod(const FString& ModName)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	if (Settings->bAlwaysBuildBeforeZipping && !BuildMod(ModName, !Settings->bZipWhenContentIsSame))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to zip mod because building failed"));
		return false;
	}

	// Yes, this is nasty boolean logic, but only because more mod managers may be supported and we want an easy way to add them
	bool bIsZipped = false;
	if (Settings->bUsingThunderstore)
	{
		bIsZipped = ZipModStaging(ModName, "Thunderstore");
	}

	if (Settings->bUsingCurseforge)
	{
		bIsZipped = ZipModBasic(ModName, "Curseforge");
	}

	// Default to basic zipping if no mod manager is selected (for now)
	if (!Settings->bUsingThunderstore && !Settings->bUsingCurseforge)
	{
		bIsZipped = ZipModBasic(ModName, "None");
	}

	return bIsZipped;
}

bool UModBuilder::UninstallMod(const FString& ModName)
{
	FString OutFileName;
	if (!GetOutputPakDirectory(OutFileName, ModName))
	{
		return false;
	}

	if (!FPaths::FileExists(OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Output file does not exist: %s, nothing to uninstall"), *OutFileName);
		return false;
	}

	if (!FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to delete file: %s"), *OutFileName);
		return false;
	}
	
	return true;
}
