#include "UpdateDialog.h"

#include "HttpModule.h"
#include "ModdingExSettings.h"
#include "ModdingExStyle.h"
#include "semver.hpp"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IPluginManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Input/SHyperlink.h"

void UpdateDialog::CheckForUpdate()
{
	if(Instance.IsValid())
	{
		if(Instance->Window.IsValid())
			Instance->Window->RequestDestroyWindow();
		return;
	}

	FString URL = "https://api.github.com/repos/ToniMacaroni/ModdingEx/releases/latest";
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetVerb("GET");
	Request->SetURL(URL);
	Request->OnProcessRequestComplete().BindLambda([&](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if(!bWasSuccessful)
		{
			UE_LOG(LogTemp, Error, TEXT("Github request not successful."))
			return;
		}

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
		if(!FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to deserialize github JSON."))
			return;
		}

		FString Tag = JsonObject->GetStringField("tag_name");
		if(Tag.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get tag name from github JSON."))
			return;
		}

		IPluginManager& PluginManager = IPluginManager::Get();
		TArray<TSharedRef<IPlugin>> Plugins = PluginManager.GetDiscoveredPlugins();
		for(const TSharedRef<IPlugin>& Plugin: Plugins) {
			if (Plugin->GetName() == "ModdingEx") {
				const FPluginDescriptor& Descriptor = Plugin->GetDescriptor();
				auto PluginVersion = semver::version::parse(TCHAR_TO_UTF8(*Descriptor.VersionName));
				auto LatestVersion = semver::version::parse(TCHAR_TO_UTF8(*Tag));
				if (LatestVersion > PluginVersion) {
					UE_LOG(LogTemp, Warning, TEXT("ModdingEx Update available! Showing dialog."))

					if(GetDefault<UModdingExSettings>()->bShowUpdateDialog)
					{
						Instance = MakeShareable(new UpdateDialog());
						Instance->CreateWindow();	
					}
					
					return;
				}

				if(LatestVersion < PluginVersion)
				{
					UE_LOG(LogTemp, Warning, TEXT(" === ModdingEx is ahead of the latest release ==="))
					return;
				}

				UE_LOG(LogTemp, Warning, TEXT("ModdingEx is up to date."))
				return;
			}
		}

		UE_LOG(LogTemp, Error, TEXT("Failed to find ModdingEx plugin."))
	});

	Request->ProcessRequest();
}

void UpdateDialog::CreateWindow()
{
	FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	FSlateFontInfo ContentFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	
	Window = SNew(SWindow)
						.Title(FText::FromString("ModdingEx"))
						.SupportsMaximize(false)
						.SupportsMinimize(false)
						.FocusWhenFirstShown(true)
						.SizingRule(ESizingRule::Autosized)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.25f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.Padding(10, 0, 10, 20)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("ModdingEx update available!"))
				.Font(HeaderFont)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.Padding(10, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Head over to the repo to get the latest version."))
				.Font(ContentFont)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.75f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(10.0f, 0.0f, 5.0f, 0.0f)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SImage).Image(FModdingExStyle::Get().GetBrush("ModdingEx.Github"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SHyperlink)
					.Text(FText::FromString("github.com/ToniMacaroni/ModdingEx"))
					.ToolTipText(FText::FromString("Go to the downloads in your default browser."))
					.OnNavigate(FSimpleDelegate::CreateLambda([&]
					{
						FPlatformProcess::LaunchURL(TEXT("https://github.com/ToniMacaroni/ModdingEx/releases"), nullptr, nullptr);
					}))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(20.0f, 0.0f, 7.0f, 7.0f)
				[
					SNew(SButton)
					.Text(FText::FromString("Got it"))
					.ContentPadding(7)
					.OnClicked_Lambda([&]
					{
						Window->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		];
	
	FSlateApplication::Get().AddModalWindow(Window.ToSharedRef(), nullptr);
}
