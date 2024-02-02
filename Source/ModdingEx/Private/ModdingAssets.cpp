#include "ModdingAssets.h"

bool ModdingAssets::DoesModExist(const FString& ModName)
{
	return FPaths::FileExists(FPaths::ProjectDir() / "Content" / "Mods" / ModName / "ModActor.uasset");
}

void ModdingAssets::GetMods(TArray<FString>& Dirs)
{
	IFileManager::Get().IterateDirectory(*(FPaths::ProjectDir() / "Content" / "Mods"), [&Dirs](const TCHAR* FilenameOrDirectory, bool bIsDirectory)
	{
		if (bIsDirectory && FPaths::FileExists(FPaths::Combine(FilenameOrDirectory, "ModActor.uasset")))
		{
			Dirs.Add(FPaths::GetCleanFilename(FilenameOrDirectory));
		}
		return true;
	});
}