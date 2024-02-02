#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;

DECLARE_LOG_CATEGORY_EXTERN(LogModdingEx, Log, All);

class FModdingExModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS) const;
	void StartingDialog(const FText& Title, const FText& HeaderText, const FText& Message) const;
	void OnOpenPluginSettings() const;
	void OnOpenBlueprintCreator() const;
	void OnOpenModCreator() const;
	FReply TryStartGame() const;

private:
	void RegisterMenus();

	TSharedPtr<FUICommandList> PluginCommands;

	// List of mods that can be build before starting the game + "None"
	TArray<TSharedPtr<FString>> StartBuildMods;
	TSharedPtr<FString> SelectedStartBuildMod;
};
