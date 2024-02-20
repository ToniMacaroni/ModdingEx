#pragma once

#include "CoreMinimal.h"
#include "ModdingExSettings.generated.h"

USTRUCT()
struct FCustomModActorEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName EventName;

	UPROPERTY(EditAnywhere)
	bool bIsEnabled;
};

UENUM(BlueprintType)
enum class EModManager : uint8
{
	None UMETA(DisplayName = "None"),
	Curseforge UMETA(DisplayName = "Curseforge"),
	Thunderstore UMETA(DisplayName = "Thunderstore")
};

USTRUCT()
struct FModManagers
{
	GENERATED_BODY()

	EModManager ModManager;
};

USTRUCT()
struct FThunderstoreManifest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Version;

	UPROPERTY(EditAnywhere)
	FString Website;

	UPROPERTY(EditAnywhere)
	TArray<FString> Dependencies;
};

USTRUCT()
struct FThunderstoreManifestDetailed
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	FString Version;

	UPROPERTY(EditAnywhere)
	FString Author;

	UPROPERTY(EditAnywhere)
	FString Website;

	UPROPERTY(EditAnywhere)
	FString Description;

	UPROPERTY(EditAnywhere)
	TArray<FString> Dependencies;
};

UCLASS(Config=Editor)
class UModdingExSettings : public UObject
{
	GENERATED_BODY()
	
public:

	/** The directory of the game you are modding (the root dir)  */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	FDirectoryPath GameDir;

	/** Path to the game exe (used for starting the game from the editor). Can be relative to GameDir or absolute */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	FFilePath GameExePath = { "Pal/Binaries/Win64/Palworld-Win64-Shipping.exe" };

	/** Folder where content mods should be build to (relative to the GameDir) */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	FString ContentModFolder = "{GameName}/Content/Paks/~mods";

	/** Folder where logic mods should be build to (relative to the GameDir) */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	FString LogicModFolder = "{GameName}/Content/Paks/LogicMods";

	/** If specified always outputs pak files to this directory no matter what type of mod */
	UPROPERTY(Config, EditAnywhere, Category = "General")
	FDirectoryPath CustomPakDir;

	/** The name of the executables to kill before building the mod (stuff that blocks overwriting the pak file) */
	UPROPERTY(Config, EditAnywhere, Category = "Building")
	TArray<FString> ProcessesToKill = { "FModel.exe" };

	/** If true will kill the processes specified in ProcessesToKill before building the mod */
	UPROPERTY(Config, EditAnywhere, Category = "Building")
	bool bShouldKillProcesses = true;

	/** When chosen a mod to build before starting the game via the button, this will cancel the start if the mod building wasn't successul */
	UPROPERTY(Config, EditAnywhere, Category = "Building")
	bool bShouldStartGameAfterFailedBuild = false;

	/** If true will save all assets before building a mod */
	UPROPERTY(Config, EditAnywhere, Category = "Building")
	bool bSaveAllBeforeBuilding = true;

	/** If you are uploading your mod on Curseforge */
	UPROPERTY(Config, EditAnywhere, Category = "Mod Manager")
	bool bUsingCurseforge = true;

	/** If you are uploading your mod on Thunderstore */
	UPROPERTY(Config, EditAnywhere, Category = "Mod Manager")
	bool bUsingThunderstore = false;

	/** Staging directory for prepping Thunderstore mods for release */
	UPROPERTY(Config, EditAnywhere, Category = "Mod Manager")
	FDirectoryPath PrepStagingDir = { "Saved/Staging" };

	/** If you want to open README.md in your default text editor when Prepare For Mod Release runs */
	UPROPERTY(Config, EditAnywhere, Category = "Mod Manager")
	bool bOpenReadmeAfterPrep = true;

	/** If true will build the mod before zipping it (using Modding Tools -> Zip Mod) */
	UPROPERTY(Config, EditAnywhere, Category = "Zipping")
	bool bAlwaysBuildBeforeZipping = true;

	/** Path where the zipped mods should be saved to */
	UPROPERTY(Config, EditAnywhere, Category = "Zipping")
	FDirectoryPath ModZipDir = { "Saved/Zips" };

	/** If true will open the folder where the zipped mod was saved to after zipping */
	UPROPERTY(Config, EditAnywhere, Category = "Zipping")
	bool bOpenZipFolderAfterZipping = true;

	/** If true checks the hash of the output file before and after building to check if stuff has changed */
	UPROPERTY(Config, EditAnywhere, Category = "Hash Check")
	bool bShouldCheckHash = true;

	/** Also zip the mod even if the content stayed the same after building */
	UPROPERTY(Config, EditAnywhere, Category = "Hash Check")
	bool bZipWhenContentIsSame = false;

	/** If building before starting the game is enabled then this will disable the check to see if the content has changed */
	UPROPERTY(Config, EditAnywhere, Category = "Hash Check")
	bool bDontCheckHashOnGameStart = false;
	
	/** A list of events to add when creating a new mod. If you have common events that you hook in lua, add them here */
	UPROPERTY(Config, EditAnywhere, Category= "Advanced")
	TArray<FCustomModActorEvent> NewEventsToAdd = {
		{ FName("PreBeginPlay"), true },
		{ FName("PostBeginPlay"), true },
		{ FName("PrintToModLoader"), true },
		{ FName("ModMenuButtonPressed"), false }
	};

	/** Shows the welcome window on startup again like on first startup */
	UPROPERTY(Config, EditAnywhere, Category = "Misc")
	bool bIsFirstStart = true;

	/** Whether to notify once an update to the plugin is available */
	UPROPERTY(Config, EditAnywhere, Category = "Misc")
	bool bShowUpdateDialog = true;
};
