#pragma once
#include "ThunderstoreApi.generated.h"

USTRUCT()
struct FThunderstorePackageVersion
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString name;
	UPROPERTY()
	FString full_name;
	UPROPERTY()
	FString description;
	UPROPERTY()
	FString download_url;
};

USTRUCT()
struct FThunderstorePackage
{
	GENERATED_BODY()
	
	UPROPERTY()
	FString name;
	FString full_name;
	UPROPERTY()
	TArray<FThunderstorePackageVersion> versions;
};


namespace ThunderstoreApi
{
	bool ParseResponseContent(const FString& Content, TArray<FThunderstorePackage>& OutPackages);
}