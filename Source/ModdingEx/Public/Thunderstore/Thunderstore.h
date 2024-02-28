#pragma once
#include "ModdingExSection.h"
#include "ModdingExStyle.h"

#include "Misc/ScopedSlowTask.h"
#include "HttpModule.h"

struct FThunderstorePackageVersion;

struct FModRequestInfo
{
	TSharedPtr<FScopedSlowTask> ProgressTask{};
	int32 ContentLength{0};
	int32 Received{0};

	FModRequestInfo(TSharedPtr<FScopedSlowTask> ProgressTask, int32 ContentLength = 0, int32 Received = 0) :
		ProgressTask(MoveTemp(ProgressTask)), ContentLength(ContentLength), Received(Received)
	{
	}
};

struct FSourceEntry
{
	FString Name;
	FString SearchPath;
	TArray<uint8> Data;

	FSourceEntry(FString Name, FString SearchPath, TArray<uint8> Data) : Name(MoveTemp(Name)),
	                                                                     SearchPath(MoveTemp(SearchPath)),
	                                                                     Data(MoveTemp(Data))
	{
	}
};

class FThunderstore
{
public:
	void RegisterSections(TArray<FModdingExSection>& Sections, TSharedPtr<FUICommandList>& PluginCommands);

private:
	void OnOpenDownloadDependency() const;
	static FReply DownloadDependency(FString DependencyString);

private:
	static FString GetCachePath();

private:
	static TOptional<FThunderstorePackageVersion> TryFindVersionInCache(const FString& DependencyString);
	
private:
	static void OnIndexFetchComplete(const FHttpResponsePtr& Response, bool ConnectedSuccessfully,
	                                 TSharedPtr<FScopedSlowTask> ScopedTask,
	                                 const FString& DependencyString);

	static void DownloadVersion(TSharedPtr<FScopedSlowTask> ScopedTask, const FThunderstorePackageVersion& PackageVersion);
	static void OnModDownloadComplete(const FHttpResponsePtr& Response, TSharedPtr<FModRequestInfo> RequestInfo);
};

class FThunderstoreCommands : public TCommands<FThunderstoreCommands>
{
public:
	FThunderstoreCommands() : TCommands(
		TEXT("ModdingEx_Thunderstore"),
		NSLOCTEXT("Contexts", "ModdingEx_Thunderstore", "ModdingEx Thunderstore Support"), NAME_None,
		FModdingExStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> OnOpenDownloadDependency;
};
