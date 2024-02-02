#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// TODO: Expose functions to Blueprint
class UModBuilder : UBlueprintFunctionLibrary
{
private:
	static bool ZipModInternal(const FString& ModName);
	TPromise<bool> BuildModAsync(const FString& ModName, bool bForceRebuild);

public:
	static bool BuildMod(const FString& ModName, bool bIsSameContentError = true);
	static bool ZipMod(const FString& ModName);
	static bool GetOutputFolder(bool bIsLogicMod, FString& OutFolder);
	static bool Pack(const FString& FilesPath, const FString& OutputPath);
	static bool Cook();
	static bool ExecGenericCommand(const TCHAR* Command, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr);
	static FString CreateFilesTxt(const FString& RootDir, const FString& TrackingDir);
};
