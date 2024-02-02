#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

class UBlueprintCreator : UBlueprintFunctionLibrary
{
public:
	static bool SplitPathToName(const FString& Path, FString& OutName);
	static void CleanBlueprintPath(const FString& Path, FString& OutPath, FString& OutName);
	static bool CreateGameBlueprint(const FString& Path, const FString& ClassName, UBlueprint*& OutBlueprint);
	static bool CreateModBlueprint(const FString& ModName, UClass* Class, UBlueprint*& OutBlueprint);
	static UBlueprint* CreateBlueprint(const FString& Path, const FString& BlueprintName, UClass* Class);
};
