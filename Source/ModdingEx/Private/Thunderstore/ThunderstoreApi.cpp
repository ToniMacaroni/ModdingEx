#include "Thunderstore/ThunderstoreApi.h"

#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "ModdingEx.h"

namespace ThunderstoreApi
{
	bool ParseResponseContent(const FString& Content, TArray<FThunderstorePackage>& OutPackages)
	{	
		const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(Content);

		TArray<TSharedPtr<FJsonValue>> PackagesArray{};
		if (!FJsonSerializer::Deserialize(JsonReader, PackagesArray))
		{
			return false;
		}

		TArray<FThunderstorePackage> Packages{};

		FThunderstorePackageVersion FoundVersion{};
		for (const auto& Package : PackagesArray)
		{
			FThunderstorePackage ParsedPackage{};
			FText FailureReason{};
			if (!FJsonObjectConverter::JsonObjectToUStruct(Package->AsObject().ToSharedRef(), &ParsedPackage, 0, 0, false,
														   &FailureReason))
			{
				UE_LOG(LogModdingEx, Error, TEXT("%s"), *FailureReason.ToString());
				return false;
			}

			Packages.Add(ParsedPackage);
		}

		OutPackages = Packages;
		return true;
	}

}
