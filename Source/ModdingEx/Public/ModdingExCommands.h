#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "ModdingExStyle.h"

class FModdingExCommands : public TCommands<FModdingExCommands>
{
public:
	FModdingExCommands()
		: TCommands(TEXT("ModdingEx"), NSLOCTEXT("Contexts", "ModdingEx", "ModdingEx Plugin"), NAME_None,
		            FModdingExStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OpenBlueprintModCreator;
	TSharedPtr<FUICommandInfo> OpenBlueprintCreator;
	TSharedPtr<FUICommandInfo> OpenPluginSettings;
	TSharedPtr<FUICommandInfo> OpenGameFolder;
	TSharedPtr<FUICommandInfo> OpenRepository;
};
