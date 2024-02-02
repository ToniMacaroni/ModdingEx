#pragma once

class ModdingAssets
{
public:
	static bool DoesModExist(const FString& ModName);
	static void GetMods(TArray<FString>& Dirs);
};
