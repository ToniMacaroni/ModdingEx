#include "ModdingExCommands.h"

#define LOCTEXT_NAMESPACE "FModdingExModule"

void FModdingExCommands::RegisterCommands()
{
	UI_COMMAND(OpenModCreator, "Mod Creator", "Create Mods", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenBlueprintCreator, "Blueprint Creator", "Create blueprints", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenPluginSettings, "Settings", "Open the ModdingEx plugin settings", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
