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

UCLASS(Config=Editor)
class UModdingExSettings : public UObject
{
	GENERATED_BODY()
	
public:

	/** The directory of the game you are modding (the root dir)  */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FDirectoryPath GameDir;

	/** Path to the game exe (used for starting the game from the editor). Can be relative to GameDir or absolute */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FFilePath GameExePath = { "Pal/Binaries/Win64/Palworld-Win64-Shipping.exe" };

	/** Folder where content mods should be build to (relative to the GameDir) */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FString ContentModFolder = "{GameName}/Content/Paks/~mods";

	/** Folder where logic mods should be build to (relative to the GameDir) */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FString LogicModFolder = "{GameName}/Content/Paks/LogicMods";

	/** If specified always outputs pak files to this directory no matter what type of mod */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FDirectoryPath CustomPakDir;

	/** The name of the executables to kill before building the mod (stuff that blocks overwriting the pak file) */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	TArray<FString> ProcessesToKill = { "FModel.exe" };

	/** If true will kill the processes specified in ProcessesToKill before building the mod */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	bool bShouldKillProcesses = true;

	/** When chosen a mod to build before starting the game via the button, this will cancel the start if the mod building wasn't successul */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	bool bShouldStartGameAfterFailedBuild = false;

	/** If true checks the hash of the output file before and after building to check if stuff has changed */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx | Hash Check")
	bool bShouldCheckHash = true;

	/** If building before starting the game is enabled then this will disable the check to see if the content has changed */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx | Hash Check")
	bool bDontCheckHashOnGameStart = false;

	/** Also zip the mod even if the content stayed the same after building */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx | Hash Check")
	bool bZipWhenContentIsSame = false;

	/** If true will build the mod before zipping it (using Modding Tools -> Zip Mod) */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	bool bAlwaysBuildBeforeZipping = true;

	/** Path where the zipped mods should be saved to */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	FDirectoryPath ModZipDir = { "Saved/Zips" };

	/** If true will open the folder where the zipped mod was saved to after zipping */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx")
	bool bOpenZipFolderAfterZipping = true;

	/** A list of events to add when creating a new mod. If you have common events that you hook in lua, add them here */
	UPROPERTY(Config, EditAnywhere, Category= "ModdingEx | Advanced")
	TArray<FCustomModActorEvent> NewEventsToAdd = {
		{ FName("PreBeginPlay"), true },
		{ FName("PostBeginPlay"), true },
		{ FName("PrintToModLoader"), true },
		{ FName("ModMenuButtonPressed"), false }
	};

	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx | Misc")
	bool bIsFirstStart = true;

	/** Whether to notify once an update to the plugin is availble */
	UPROPERTY(Config, EditAnywhere, Category = "ModdingEx | Misc")
	bool bShowUpdateDialog = true;
};
