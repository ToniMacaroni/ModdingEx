#include "ModBuilder.h"

#include "ISettingsModule.h"
#include "ModdingEx.h"
#include "ModdingExSettings.h"
#include "Async/Async.h"
#include "FileUtilities/ZipArchiveWriter.h"
#include "FileUtilities/ZipArchiveWriter.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
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
		UE_LOG(LogModdingEx, Error, TEXT("Output folder does not exist: %s"), *OutFolder);
		return false;
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

	SlowTask.EnterProgressFrame(1, FText::FromString("Cooking mod"));
	if (!Cook())
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

bool UModBuilder::ZipModInternal(const FString& ModName)
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

	if (!FPaths::FileExists(OutFileName))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Output file does not exist: %s"), *OutFileName);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Didn't find anything to zip. Make sure you built the mod."));
		return false;
	}

	const FString ZipsPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), Settings->ModZipDir.Path);

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

	FZipArchiveWriter* ZipWriter = new FZipArchiveWriter(ZipFile);

	TArray<FString> FilesToArchive{OutFileName};

	for (FString FileName : FilesToArchive)
	{
		TArray<uint8> FileData;
		FFileHelper::LoadFileToArray(FileData, *FileName);
		FPaths::MakePathRelativeTo(FileName, *OutputDir);

		ZipWriter->AddFile(FileName, FileData, FDateTime::Now());
	}

	delete ZipWriter;
	ZipWriter = nullptr;

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

bool UModBuilder::ZipMod(const FString& ModName)
{
	const auto Settings = GetDefault<UModdingExSettings>();

	if (Settings->bAlwaysBuildBeforeZipping && !BuildMod(ModName, !Settings->bZipWhenContentIsSame))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Failed to zip mod because building failed"));
		return false;
	}

	return ZipModInternal(ModName);
}
