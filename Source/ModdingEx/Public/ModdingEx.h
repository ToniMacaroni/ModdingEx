#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ModdingExSection.h"
#include "Thunderstore/Thunderstore.h"

class FToolBarBuilder;
class FMenuBuilder;

DECLARE_LOG_CATEGORY_EXTERN(LogModdingEx, Log, All);

DECLARE_DELEGATE(FOnModManagerChanged);

class FModdingExModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);
	void OnOpenPluginSettings() const;
	void OnOpenBlueprintCreator() const;
	void OnOpenModCreator() const;
	void OnOpenPrepareModForRelease(const FString& Mod) const;
	FReply TryStartGame() const;
	void OnOpenGameFolder() const;
	void OnOpenRepository() const;

	FOnModManagerChanged OnModManagerChanged;

private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> PluginCommands;

	TArray<FModdingExSection> Sections;

	FThunderstore Thunderstore;
	
	// List of mods that can be build before starting the game + "None"
	TArray<TSharedPtr<FString>> StartBuildMods;
	TSharedPtr<FString> SelectedStartBuildMod;
};
