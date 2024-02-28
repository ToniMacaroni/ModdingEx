#include "Thunderstore/Thunderstore.h"

#include "FileHelpers.h"
#include "Json.h"

#include "HttpModule.h"
#include "ModdingEx.h"
#include "ModdingExSettings.h"
#include "PackageTools.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/ScopedSlowTask.h"

#include "SPositiveActionButton.h"

#include "Zip/ZipFile.h"

#include "Notifications.h"

#include "Thunderstore/ThunderstoreApi.h"
#include "Thunderstore/ThunderstoreLoctext.h"

#define LOCTEXT_NAMESPACE "ModdingEx_Thunderstore"

void FThunderstore::RegisterSections(TArray<FModdingExSection>& Sections, TSharedPtr<FUICommandList>& PluginCommands)
{
	FThunderstoreCommands::Register();

	FModdingExSection Section("ModdingEx_Thunderstore", ThunderstoreLoctext::Thunderstore);
	Section.Entries.Add(FThunderstoreCommands::Get().OnOpenDownloadDependency);

	Sections.Add(Section);

	PluginCommands->MapAction(
		FThunderstoreCommands::Get().OnOpenDownloadDependency,
		FExecuteAction::CreateRaw(this, &FThunderstore::OnOpenDownloadDependency),
		FCanExecuteAction());
}

void FThunderstore::OnOpenDownloadDependency() const
{
	const TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(ThunderstoreLoctext::DownloadDependency)
		.ClientSize({400, 150})
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	const auto DependencyString = SNew(SEditableTextBox)
		.HintText(FText::FromString("localcc-HelloWorld-1.0.1"))
		.ToolTipText(ThunderstoreLoctext::DependencyStringHint)
		.SelectAllTextWhenFocused(true);

	Window->SetContent(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(7)
		[
			SNew(STextBlock)
				.Text(ThunderstoreLoctext::DownloadDependencyDescription)
				.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		  .AutoHeight()
		  .Padding(7)
		[
			DependencyString
		]
		+ SVerticalBox::Slot()
		  .FillHeight(1)
		  .Padding(7)
		[
			SNew(SPositiveActionButton)
				.Text(ThunderstoreLoctext::Download)
				.OnClicked(FOnClicked::CreateLambda([this, DependencyString]
			                           {
				                           return this->DownloadDependency(DependencyString->GetText().ToString());
			                           }))
		]);

	const TSharedPtr<SWindow> RootWindow = FGlobalTabmanager::Get()->GetRootWindow();
	FSlateApplication::Get().AddWindowAsNativeChild(Window, RootWindow.ToSharedRef());
}

bool TryFindPackage(const TArray<FThunderstorePackage>& Packages, const FString& DependencyString,
                    FThunderstorePackageVersion& OutVersion)
{
	for (const auto& Package : Packages)
	{
		for (const auto& Version : Package.versions)
		{
			if (Version.full_name == DependencyString)
			{
				OutVersion = Version;
				return true;
			}
		}
	}

	return false;
}

FReply FThunderstore::DownloadDependency(FString DependencyString)
{
	TSharedPtr<FScopedSlowTask> ScopedTask = MakeShared<FScopedSlowTask>(3, ThunderstoreLoctext::FetchingDependency);

	ScopedTask->MakeDialog();
	ScopedTask->EnterProgressFrame();

	TOptional<FThunderstorePackageVersion> FoundVersion = TryFindVersionInCache(DependencyString);
	if (FoundVersion)
	{
		ScopedTask->EnterProgressFrame();
		DownloadVersion(ScopedTask, *FoundVersion);
		return FReply::Handled();
	}
	
	FHttpModule& Module = FHttpModule::Get();

	const FString ApiUrl = FString::Format(
		TEXT("https://thunderstore.io/c/{0}/api/v1"), FStringFormatOrderedArguments({
			GetDefault<UModdingExSettings>()->ThunderstoreCommunityName
		}));

	const TSharedPtr<IHttpRequest> Request = Module.CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Accept-Encoding"), TEXT("gzip"));
	Request->SetURL(ApiUrl + "/package");
	Request->SetTimeout(5);

	Request->OnProcessRequestComplete().BindLambda(
		[DependencyString](FHttpRequestPtr, const FHttpResponsePtr& Response,
		                   bool ConnectedSuccessfully, TSharedPtr<FScopedSlowTask> ScopedTask)
		{
			OnIndexFetchComplete(Response, ConnectedSuccessfully, ScopedTask, DependencyString);
		}, ScopedTask);

	Request->ProcessRequest();

	return FReply::Handled();
}

FString FThunderstore::GetCachePath()
{
	return FPaths::Combine(FPaths::ProjectIntermediateDir(), ".thunderstore_cache.json");
}

TOptional<FThunderstorePackageVersion> FThunderstore::TryFindVersionInCache(const FString& DependencyString)
{
	FString CachedResponse{};
	if (!FFileHelper::LoadFileToString(CachedResponse, *GetCachePath()))
	{
		return NullOpt;
	}
	
	TArray<FThunderstorePackage> Packages{};
	if (!ThunderstoreApi::ParseResponseContent(CachedResponse, Packages))
	{
		return NullOpt;
	}

	FThunderstorePackageVersion FoundVersion{};
	if (!TryFindPackage(Packages, DependencyString, FoundVersion))
	{
		return NullOpt;
	}

	return FoundVersion;
}

void FThunderstore::OnIndexFetchComplete(
	const FHttpResponsePtr& Response, bool ConnectedSuccessfully, TSharedPtr<FScopedSlowTask> ScopedTask,
	const FString& DependencyString)
{
	ScopedTask->EnterProgressFrame();
	if (!ConnectedSuccessfully || Response->GetResponseCode() != 200)
	{
		Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToFindMod);
		return;
	}

	const FString Content = Response->GetContentAsString();
	FFileHelper::SaveStringToFile(Content, *GetCachePath());
	
	TArray<FThunderstorePackage> Packages{};
	if (!ThunderstoreApi::ParseResponseContent(Content, Packages))
	{
		Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToParseResponse);
		return;
	}

	FThunderstorePackageVersion FoundVersion{};
	if (!TryFindPackage(Packages, DependencyString, FoundVersion))
	{
		Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToFindMod);
		return;
	}

	DownloadVersion(ScopedTask, FoundVersion);
}

void FThunderstore::DownloadVersion(TSharedPtr<FScopedSlowTask> ScopedTask, const FThunderstorePackageVersion& PackageVersion)
{
	const TSharedPtr<FModRequestInfo> RequestInfo = MakeShared<FModRequestInfo>(ScopedTask);

	FHttpModule& Module = FHttpModule::Get();
	const TSharedPtr<IHttpRequest> Request = Module.CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetURL(PackageVersion.download_url);
	Request->SetTimeout(5);

	Request->OnHeaderReceived().BindLambda(
		[](FHttpRequestPtr, const FString& HeaderName, const FString& NewHeaderValue,
		   const TSharedPtr<FModRequestInfo>& RequestInfo)
		{
			if (HeaderName == TEXT("Content-Length"))
			{
				RequestInfo->ContentLength = FCString::Atoi(*NewHeaderValue);
				RequestInfo->ProgressTask->TotalAmountOfWork += RequestInfo->ContentLength;
			}
		}, RequestInfo);

	Request->OnProcessRequestComplete().BindLambda(
		[](FHttpRequestPtr, const FHttpResponsePtr& Response, bool, TSharedPtr<FModRequestInfo> RequestInfo)
		{
			OnModDownloadComplete(Response, RequestInfo);
		}, RequestInfo);

	Request->OnRequestProgress().BindLambda(
		[](FHttpRequestPtr, int32, int32 BytesReceived, const TSharedPtr<FModRequestInfo>& RequestInfo)
		{
			if (RequestInfo->ContentLength == 0) return;

			const int32 Received = BytesReceived - RequestInfo->Received;
			RequestInfo->Received = BytesReceived;

			RequestInfo->ProgressTask->MakeDialog();
			RequestInfo->ProgressTask->EnterProgressFrame(Received);
		}, RequestInfo);

	Request->ProcessRequest();
}

void FThunderstore::OnModDownloadComplete(const FHttpResponsePtr& Response, TSharedPtr<FModRequestInfo> RequestInfo)
{
	RequestInfo->ProgressTask->EnterProgressFrame(0.5f);

	FZipFile File{};
	FZipError Error{};

	if (!FZipFile::TryCreateZipFile(Response->GetContent(), File, Error))
	{
		UE_LOG(LogModdingEx, Error, TEXT("Zip file open error: %d, description: %s"), Error.ErrorCode,
		       Error.Description ? **Error.Description : TEXT(""));

		Notifications::ShowFailNotification(ThunderstoreLoctext::ModDecompressionError_ZipOpen);
		return;
	}

	const TArray<FZipEntry> Entries = File.GetEntries(Error);

	TArray<FSourceEntry> SourceEntries{};
	for (const FZipEntry& Entry : Entries)
	{
		if (!Entry.Name) continue;

		const FString& Name = *Entry.Name;
		if (!Name.StartsWith("Sources")) continue;

		if (Name.Contains(".."))
		{
			UE_LOG(LogModdingEx, Warning, TEXT("Skipping potentially malicious entry: %s"), *Name);
			continue;
		}

		TArray<uint8> Data = File.ReadEntry(Entry);
		if (Data.Num() == 0)
		{
			continue;
		}

		const FString DstName = Name.Replace(TEXT("Sources/"), TEXT(""));
		const FString SearchPath = FPaths::SetExtension(FPaths::Combine(TEXT("/Game"), DstName), "");

		SourceEntries.Add(FSourceEntry{DstName, SearchPath, Data});
	}

	if (SourceEntries.IsEmpty())
	{
		UE_LOG(LogModdingEx, Error,
		       TEXT("Sources not found in the mod file, error: %d, description: %s"), Error.ErrorCode,
		       Error.Description ? **Error.Description : TEXT(""));
		Notifications::ShowFailNotification(ThunderstoreLoctext::ModDecompressionError_MissingSources);
		return;
	}

	for (const auto& Entry : SourceEntries)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Entry.SearchPath);
		if (UPackage* Package = FindPackage(nullptr, *Entry.SearchPath))
		{
			if (!UPackageTools::UnloadPackages({Package}))
			{
				Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToUnloadPackages);
				return;
			}
		}
	}

	bool bFailedToSave = false;
	for (const auto& Entry : SourceEntries)
	{
		FString DstPath = FPaths::Combine(FPaths::ProjectContentDir(), Entry.Name);
		if (!FFileHelper::SaveArrayToFile(Entry.Data, *DstPath))
		{
			UE_LOG(LogModdingEx, Error, TEXT("Failed to save %s"), *DstPath);
			bFailedToSave = true;
		}
	}

	if (bFailedToSave)
	{
		Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToSave);
	}


	TArray<UPackage*> PackagesToReload{};
	for (const auto& Entry : SourceEntries)
	{
		if (UPackage* Package = UPackageTools::LoadPackage(Entry.SearchPath))
		{
			PackagesToReload.Add(Package);
		}
	}

	if (!UPackageTools::ReloadPackages(PackagesToReload))
	{
		Notifications::ShowFailNotification(ThunderstoreLoctext::FailedToReload);
	}

	for (const auto& Entry : SourceEntries)
	{
		if (UPackage* Package = UPackageTools::LoadPackage(Entry.SearchPath))
		{
			Package->SetChunkIDs({0});
			Package->SetDirtyFlag(true);
		}
	}

	FEditorFileUtils::SaveDirtyPackages(false, true, true);

	RequestInfo->ProgressTask->EnterProgressFrame(0.5f);

	Notifications::ShowSuccessNotification(ThunderstoreLoctext::SuccessfulDownload);
}

void FThunderstoreCommands::RegisterCommands()
{
	UI_COMMAND(OnOpenDownloadDependency, "Dependency Downloader", "Download mod dependency from Thunderstore",
	           EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
