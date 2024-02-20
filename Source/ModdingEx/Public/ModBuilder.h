#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ModBuilder.generated.h"

UCLASS(Blueprintable)
class UModBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
private:
	static void EditDirectoriesToAlwaysCook(const FString& DirectoryToCook, const bool bShouldRemove = false);
	
	static bool ZipModInternal(const FString& ModName);

	static void CreateModManifest(FString& OutModManifest, const FString& ModName, const FString& WebsiteUrl,
	                              const FString& Dependencies, const FString& ModDesc, const FString& ModVersion);

	static void CreateModReadme(FString& OutModReadme, const FString& ModName, const FString& ModDesc,
								  const FString& ModVersion, const FString& ModAuthor);

	static bool GetModProperties(const FString& ModName, FString& OutModVersion, FString& OutModAuthor, FString& OutModDescription);

	static FString* FindFStringPropertyValue(UObject* Object, const FName& PropertyName);

	TPromise<bool> BuildModAsync(const FString& ModName, bool bForceRebuild);

public:
	static bool ExecGenericCommand(const TCHAR* Command, const TCHAR* Params, int32* OutReturnCode, FString* OutStdOut, FString* OutStdErr);

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool BuildMod(const FString& ModName, bool bIsSameContentError = true);

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool PrepareModForRelease(const FString& ModName, const FString& WebsiteUrl, const FString& Dependencies);

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool ZipMod(const FString& ModName);

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool GetOutputFolder(bool bIsLogicMod, FString& OutFolder);

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool Pack(const FString& FilesPath, const FString& OutputPath);
	
	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static bool Cook();

	UFUNCTION(BlueprintCallable, Category = "Mod Building")
	static FString CreateFilesTxt(const FString& RootDir, const FString& TrackingDir);
};
