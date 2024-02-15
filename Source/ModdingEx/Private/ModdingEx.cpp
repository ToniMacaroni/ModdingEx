#include "ModdingEx.h"

#include "BlueprintCreator.h"
#include "ISettingsModule.h"
#include "ModdingExStyle.h"
#include "ModdingExCommands.h"
#include "ModBuilder.h"
#include "ModdingAssets.h"
#include "ModdingExSettings.h"
#include "SPositiveActionButton.h"
#include "StartupDialog.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenus.h"
#include "UpdateDialog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SHyperlink.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/STextComboBox.h"

DEFINE_LOG_CATEGORY(LogModdingEx);

#define LOCTEXT_NAMESPACE "FModdingExModule"

#define ABSPATH(x) FPaths::ConvertRelativePathToFull(x)

void FModdingExModule::StartupModule()
{
	FModdingExStyle::Initialize();
	FModdingExStyle::ReloadTextures();

	FModdingExCommands::Register();

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->RegisterSettings("Project", "Plugins", "ModdingEx",
		                                 LOCTEXT("RuntimeSettingsName", "ModdingEx"),
		                                 LOCTEXT("RuntimeSettingsDescription", "Configure the ModdingEx plugin"),
		                                 GetMutableDefault<UModdingExSettings>());
	}

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FModdingExCommands::Get().OpenBlueprintCreator,
		FExecuteAction::CreateRaw(this, &FModdingExModule::OnOpenBlueprintCreator),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FModdingExCommands::Get().OpenModCreator,
		FExecuteAction::CreateRaw(this, &FModdingExModule::OnOpenModCreator),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FModdingExCommands::Get().OpenPluginSettings,
		FExecuteAction::CreateRaw(this, &FModdingExModule::OnOpenPluginSettings),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FModdingExCommands::Get().OpenGameFolder,
		FExecuteAction::CreateRaw(this, &FModdingExModule::OnOpenGameFolder),
		FCanExecuteAction());

	PluginCommands->MapAction(
		FModdingExCommands::Get().OpenRepository,
		FExecuteAction::CreateRaw(this, &FModdingExModule::OnOpenRepository),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FModdingExModule::RegisterMenus));
	FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FModdingExModule::OnPostWorldInit);
}

void FModdingExModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FModdingExStyle::Shutdown();

	FModdingExCommands::Unregister();

	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->UnregisterSettings("Project", "Plugins", "ModdingEx");
	}
	
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
}

void FModdingExModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		// const TSharedRef<SPositiveActionButton> BuildModButton = SNew(SPositiveActionButton)
		// .ToolTipText(FText::FromString("Build Mod"))
		// .Icon(FAppStyle::Get().GetBrush("Icons.Adjust"))
		// .Text(FText::FromString("Build Mod"))
		// .OnClicked(FOnClicked::CreateLambda([this]
  //                                           {
  //                                               BuildMod("ToniMod");
  //                                               return FReply::Handled();
  //                                           }));

		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
		{
			auto BuildModsEntry = FToolMenuEntry::InitComboButton(
				"BuildMods",
				FToolUIActionChoice(),
				FNewToolMenuChoice(
					FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InNewToolMenu)
					{
						TArray<FString> Mods;
						ModdingAssets::GetMods(Mods);

						FMenuBuilder MenuBuilder(true, nullptr);
						MenuBuilder.BeginSection("ModdingEx_BuildModsEntry", LOCTEXT("ModdingEx_BuildMod", "Build Mod"));

						for (FString Mod : Mods)
						{
							MenuBuilder.AddMenuEntry(
								FText::FromString(Mod),
								FText::FromString(FString::Format(TEXT("Build {0}"), {Mod})),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, Mod]
								{
									UModBuilder::BuildMod(Mod);
								}))
							);
						}

						MenuBuilder.EndSection();

						MenuBuilder.BeginSection("ModdingEx_ZipModsEntry", LOCTEXT("ModdingEx_ZipMod", "Zip Mod"));

						for (FString Mod : Mods)
						{
							MenuBuilder.AddMenuEntry(
								FText::FromString(Mod),
								FText::FromString(FString::Format(TEXT("Zip (and build if configure in settings) {0}"), {Mod})),
								FSlateIcon(),
								FUIAction(FExecuteAction::CreateLambda([this, Mod]
								{
									UModBuilder::ZipMod(Mod);
								}))
							);
						}

						MenuBuilder.EndSection();

						InNewToolMenu->AddMenuEntry(
							"ModdingEx_ToolEntry",
							FToolMenuEntry::InitWidget(
								"ModdingEx_ToolMenu",
								MenuBuilder.MakeWidget(),
								FText::GetEmpty(),
								true, false, true
							)
						);
					})
				),
				LOCTEXT("ModdingEx_ButtonLabel", "Build Mod"),
				LOCTEXT("ModdingEx_Tooltip", "Build and Zip Mod."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust")
			);
			BuildModsEntry.StyleNameOverride = "CalloutToolbar";

			auto ModdingToolsEntry = FToolMenuEntry::InitComboButton(
				"ModdingTools",
				FToolUIActionChoice(),
				FNewToolMenuChoice(
					FNewToolMenuDelegate::CreateLambda([this](UToolMenu* InNewToolMenu)
					{
						FMenuBuilder MenuBuilder(true, PluginCommands);

						MenuBuilder.AddMenuEntry(FModdingExCommands::Get().OpenModCreator);
						MenuBuilder.AddMenuEntry(FModdingExCommands::Get().OpenBlueprintCreator);
						MenuBuilder.AddMenuEntry(FModdingExCommands::Get().OpenGameFolder);
						MenuBuilder.AddMenuEntry(FModdingExCommands::Get().OpenPluginSettings);

						MenuBuilder.BeginSection(FName("ModdingEx_GoTo"), LOCTEXT("ModdingEx_GoTo", "Go To"));
						MenuBuilder.AddMenuEntry(FModdingExCommands::Get().OpenRepository);
						MenuBuilder.EndSection();

						InNewToolMenu->AddMenuEntry(
							"ModdingEx_ToolEntry",
							FToolMenuEntry::InitWidget(
								"ModdingEx_ToolMenu",
								MenuBuilder.MakeWidget(),
								FText::GetEmpty(),
								true, false, true
							)
						);
					})
				),
				LOCTEXT("ModdingEx_ModdingTools", "Modding Tools"),
				LOCTEXT("ModdingEx_ModdingTools_Tooltip", "Modding Tools."),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings")
			);
			ModdingToolsEntry.StyleNameOverride = "CalloutToolbar";

			StartBuildMods.Empty();
			StartBuildMods.Add(MakeShared<FString>("None"));
			SelectedStartBuildMod = StartBuildMods[0];

			{
				TArray<FString> Mods;
				ModdingAssets::GetMods(Mods);

				for (FString Mod : Mods)
				{
					StartBuildMods.Add(MakeShared<FString>(Mod));
				}
			}

			const auto SelectBuildModDropdown = FToolMenuEntry::InitWidget("ME_Dropdown",
				SNew(SBox).WidthOverride(150).Padding(0,0,4,0)
				[
					SNew(STextComboBox)
					.OptionsSource(&StartBuildMods)
					.InitiallySelectedItem(StartBuildMods[0])
					.OnSelectionChanged_Lambda([&](const TSharedPtr<FString>& Item, ESelectInfo::Type)
					{
						if (Item.IsValid())
						{
							SelectedStartBuildMod = Item;
						}
					})
					.ToolTipText(FText::FromString("Here you can choose a mod to build before starting the game. Leave it as it is to just start the game."))
				]
				, FText::GetEmpty(), true, false, true);

			const auto StartGameEntry = FToolMenuEntry::InitWidget("ME_Button",
				SNew(SPositiveActionButton)
				.Text(LOCTEXT("ModdingEx_StartGame", "Start Game"))
				.Icon(FAppStyle::Get().GetBrush("Icons.Play"))
				.ToolTipText(FText::FromString("Start the game executable. Needs to be configured in the settings."))
				.OnClicked(FOnClicked::CreateRaw(this, &FModdingExModule::TryStartGame))
				, FText::GetEmpty(), true, false, true);

			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("User");
			Section.AddEntry(BuildModsEntry);
			Section.AddEntry(ModdingToolsEntry);
			Section.AddSeparator("ME_Seperator");
			Section.AddEntry(SelectBuildModDropdown);
			Section.AddEntry(StartGameEntry);
		}
	}
}

void FModdingExModule::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
	FWorldDelegates::OnPostWorldInitialization.RemoveAll(this);
	
	UModdingExSettings* Settings = GetMutableDefault<UModdingExSettings>();

	if(Settings->bIsFirstStart)
	{
		StartupDialog::ShowDialog(LOCTEXT("ModdingEx", "ModdingEx"), LOCTEXT("ModdingEx_FirstStartHeader", "Welcome to ModdingEx!"), LOCTEXT("ModdingEx_FirstStart", "This Plugin makes heavy use of tooltips. If you are not sure what an option does, hover over it to get a description."));
		
		Settings->bIsFirstStart = false;
		Settings->SaveConfig();
	}

	UpdateDialog::CheckForUpdate();
}

// TArray<TSharedPtr<FString>> PropList;
void FModdingExModule::OnOpenBlueprintCreator() const
{
	// PropList.Empty();
	// const auto W2 = SNew(SWindow).Title(FText::FromString("Cr2")).ClientSize({600,400});
	// const auto TB = SNew(SMultiLineEditableTextBox);
	// const auto LST = SNew(SListView<TSharedPtr<FString>>)
	// 	.ListItemsSource(&PropList)
	// 	.OnGenerateRow_Lambda([](TSharedPtr<FString> Item, const TSharedRef<STableViewBase>& OwnerTable)
	// 	{
	// 		return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
	// 		[
	// 			SNew(STextBlock).Text(FText::FromString(*Item))
	// 		];
	// 	});
	//
	// W2->SetContent(SNew(SVerticalBox)
	// 	+ SVerticalBox::Slot()
	// 	.FillHeight(1)
	// 	[
	// 		TB
	// 	]
	// 	+ SVerticalBox::Slot()
	// 	.FillHeight(1)
	// 	.Padding(10)
	// 	[
	// 		LST
	// 	]
	// 	+ SVerticalBox::Slot()
	// 	.AutoHeight()
	// 	[
	// 		SNew(SButton)
	// 		.Text(FText::FromString("Create"))
	// 		.OnClicked(FOnClicked::CreateLambda([&]
	// 		{
	// 			TArray<TSharedPtr<FJsonValue>> Arr;
	// 			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(TB->GetText().ToString());
	// 			if(FJsonSerializer::Deserialize(Reader, Arr))
	// 			{
	// 				for (TSharedPtr<FJsonValue> JsonValue : Arr)
	// 				{
	// 					auto JsonObject = JsonValue->AsObject();
	//
	// 					FString Name;
	// 					const TArray<TSharedPtr<FJsonValue>>* ChildProps = nullptr;
	// 					
	// 					if(!JsonObject->TryGetStringField("Name", Name))
	// 						continue;
	//
	// 					if(!JsonObject->TryGetArrayField("ChildProperties", ChildProps))
	// 						continue;
	//
	// 					for (auto ChildProp : *ChildProps)
	// 					{
	// 						auto PropObject = ChildProp->AsObject();
	// 						
	// 						FString PropName;
	// 						FString PropType;
	//
	// 						if(!PropObject->TryGetStringField("Name", PropName))
	// 							continue;
	//
	// 						if(!PropObject->TryGetStringField("Type", PropType))
	// 							continue;
	//
	// 						if(PropType != "ObjectProperty")
	// 							continue;
	//
	// 						PropList.Add(MakeShared<FString>(PropName + " (" + PropType + ")"));
	// 					}
	// 				}
	// 			}
	//
	// 			LST->RequestListRefresh();
	// 			
	// 			return FReply::Handled();
	// 		}))
	// 	]
	// 	);
	//
	// FSlateApplication::Get().AddModalWindow(W2, FSlateApplication::Get().GetActiveTopLevelWindow());
	//
	// return;
	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("ModdingEx_BlueprintCreatorTitle", "Blueprint Creator"))
		.ClientSize({400, 300})
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const auto PackagePath = SNew(SMultiLineEditableTextBox)
		.Text(FText::FromString("/Game/Pal/Blueprints/BP_GameBlueprint"))
		.ToolTipText(FText::FromString("The path to the blueprint (you can copy it from FModel)"))
		.SelectAllTextWhenFocused(true);

	const auto ParentClass = SNew(SMultiLineEditableTextBox)
		.Text(FText::FromString("/Script/CoreUObject.Object"))
		.ToolTipText(FText::FromString("The parent class of the blueprint (UObject by default). Also accepts a json string with {\"ObjectName\":... , \"ObjectPath\":...}"))
		.SelectAllTextWhenFocused(true);

	const auto BlueprintCheck = SNew(SCheckBox)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString("Open Blueprint after creation"))
		]
		.IsChecked(true);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7)
		[
			SNew(STextBlock)
			.Text(FText::FromString(
				"Put in the path of a blueprint you want to create (like /Game/Pal/Blueprints/MyBp.uasset) and the parent class (like /Script/CoreUObject.Object)."
				))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7)
		[
			PackagePath
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(7)
		[
			ParentClass
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7, 0, 7, 7)
		[
			SNew(SPositiveActionButton).Icon(FAppStyle::Get().GetBrush("Icons.Find"))
			.Text(FText::FromString("Pick Class"))
			.OnClicked(FOnClicked::CreateLambda([ParentClass]
			{
				FClassViewerInitializationOptions Options;
				UClass* OutClass = nullptr;
				SClassPickerDialog::PickClass(FText::FromString("Pick Class"), Options, OutClass, UObject::StaticClass());
				ParentClass->SetText(FText::FromString(OutClass->GetPathName()));
				return FReply::Handled();
			}))
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(7)
		[
			BlueprintCheck
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(7)
		[
			SNew(SPositiveActionButton)
			.ToolTipText(FText::FromString("Create Blueprint at specified path with specified parent class"))
			.Text(FText::FromString("Create Blueprint"))
			.OnClicked(FOnClicked::CreateLambda([PackagePath, ParentClass, BlueprintCheck, Window]
            {
				UBlueprint* Blueprint = nullptr;
                UBlueprintCreator::CreateGameBlueprint(PackagePath->GetText().ToString(), ParentClass->GetText().ToString(), Blueprint);

				if(BlueprintCheck->IsChecked())
				{
					FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(Blueprint->GetLastEditedUberGraph());
				}

				Window->RequestDestroyWindow();

                return FReply::Handled();
            }))
		]
	);

	const TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild(Window, RootWindow.ToSharedRef());
}

void FModdingExModule::OnOpenModCreator() const
{
	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("ModdingEx_ModCreatorTitle", "Mod Creator"))
		.ClientSize({400, 200})
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const TSharedRef<SMultiLineEditableTextBox> ModNameEdit = SNew(SMultiLineEditableTextBox)
		.Text(FText::FromString("MyMod"))
		.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12))
		.ToolTipText(FText::FromString("The name of the mod you want to create. This will be the name of the folder in the Mods folder"))
		.SelectAllTextWhenFocused(true);

	const auto BlueprintCheck = SNew(SCheckBox)
		.Content()
		[
			SNew(STextBlock)
			.Text(FText::FromString("Open Blueprint after creation"))
		]
		.IsChecked(true);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7)
		[
			SNew(STextBlock)
			.Text(FText::FromString(
				"Put in the name of the mod you want to create and the correct folder structure as well as a 'ModActor' Blueprint will be created for you."
				))
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7)
		[
			ModNameEdit
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(7)
		[
			BlueprintCheck
		]
		+ SVerticalBox::Slot()
		.FillHeight(1)
		.Padding(7)
		[
			SNew(SPositiveActionButton)
			.ToolTipText(FText::FromString("Create Blueprint at specified path with specified parent class"))
			.Text(FText::FromString("Create Mod"))
			.OnClicked(FOnClicked::CreateLambda([ModNameEdit, Window, BlueprintCheck]
					{
						UBlueprint* ModBlueprint = nullptr;
						UBlueprintCreator::CreateModBlueprint(ModNameEdit->GetText().ToString(), AActor::StaticClass(), ModBlueprint);

						if(BlueprintCheck->IsChecked())
						{
							FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ModBlueprint->GetLastEditedUberGraph());
						}
				
						Window->RequestDestroyWindow();
						return FReply::Handled();
					}))
		]
	);

	const TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild(Window, RootWindow.ToSharedRef());
}

FReply FModdingExModule::TryStartGame() const
{
	const auto Settings = GetDefault<UModdingExSettings>();
	FString GamePath = Settings->GameExePath.FilePath;

	if (GamePath.IsEmpty())
	{
		UE_LOG(LogModdingEx, Error, TEXT("GameExePath is empty!"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("GameExePath is empty!"));
		return FReply::Handled();
	}

	GamePath = FPaths::ConvertRelativePathToFull(Settings->GameDir.Path, GamePath);

	if(!FPaths::FileExists(GamePath))
	{
		UE_LOG(LogModdingEx, Error, TEXT("GameExePath does not exist!"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("GameExePath does not exist!"));
		return FReply::Handled();
	}


	if(SelectedStartBuildMod.IsValid())
	{
		const FString Mod = *SelectedStartBuildMod;
		if(!Mod.IsEmpty() && Mod != "None")
		{
			UE_LOG(LogModdingEx, Log, TEXT("Starting game after building %s"), *Mod);
			if(!UModBuilder::BuildMod(Mod, !Settings->bDontCheckHashOnGameStart) && !Settings->bShouldStartGameAfterFailedBuild)
			{
				UE_LOG(LogModdingEx, Error, TEXT("Failed to build mod %s"), *Mod);
				return FReply::Handled();
			}
		}
	}

	FPlatformProcess::CreateProc(*GamePath, nullptr, true, false, false, nullptr, 0, nullptr, nullptr);

	return FReply::Handled();
}

void FModdingExModule::OnOpenGameFolder() const
{
	const auto Settings = GetDefault<UModdingExSettings>();
	FString GamePath = Settings->GameDir.Path;

	if (GamePath.IsEmpty())
	{
		UE_LOG(LogModdingEx, Error, TEXT("GameDir is empty!"));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("GameDir is empty!"));
		return;
	}

	GamePath = FPaths::ConvertRelativePathToFull(GamePath);
	FPlatformProcess::ExploreFolder(*GamePath);
}

void FModdingExModule::OnOpenPluginSettings() const
{
	FModuleManager::LoadModuleChecked<ISettingsModule>("Settings").ShowViewer("Project", "Plugins", "ModdingEx");
}

void FModdingExModule::OnOpenRepository() const
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/ToniMacaroni/ModdingEx"), nullptr, nullptr);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FModdingExModule, ModdingEx)
