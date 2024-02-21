#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

class UBlueprintCreator : UBlueprintFunctionLibrary
{
public:
	static bool SplitPathToName(const FString& Path, FString& OutName);
	static void CleanBlueprintPath(const FString& Path, FString& OutPath, FString& OutName);
	static bool CreateGameBlueprint(const FString& Path, const FString& ClassName, UBlueprint*& OutBlueprint);
	static bool CreateModBlueprint(UClass* Class, UBlueprint*& OutBlueprint,
		const FString& ModName = TEXT("MyMod"),
		const FString& ModAuthor = TEXT("Unknown"),
		const FString& ModDesc = TEXT("A new mod"),
		const FString& ModVersion = TEXT("1.0.0"));
	static UBlueprint* CreateBlueprint(const FString& Path, const FString& BlueprintName, UClass* Class);
};
