#include "ModdingExCommands.h"

#define LOCTEXT_NAMESPACE "FModdingExModule"

void FModdingExCommands::RegisterCommands()
{
	UI_COMMAND(OpenModCreator, "Mod Creator", "Create Mods", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenBlueprintCreator, "Blueprint Creator", "Create blueprints", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenPluginSettings, "Settings", "Open the ModdingEx plugin settings", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenGameFolder, "Open Game Folder", "Open the game folder", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(OpenRepository, "Open Repository", "Open the ModdingEx repository", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
