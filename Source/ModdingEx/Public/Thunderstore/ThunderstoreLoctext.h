#pragma once

#define LOCTEXT_NAMESPACE "ModdingEx_Thunderstore"

namespace ThunderstoreLoctext
{
	const inline FText Thunderstore = LOCTEXT("Thunderstore", "Thunderstore");

	const inline FText Download = LOCTEXT("Download", "Download");

	const inline FText DownloadDependency = LOCTEXT("DownloadDependency", "Download dependency");
	const inline FText DownloadDependencyDescription = LOCTEXT("DownloadDependencyDescription",
	                                                        "Download a mod as a dependency from Thunderstore. To get the dependency string, open the mod page on thunderstore and find it in the details tab of the mod.");

	const inline FText DependencyStringHint = LOCTEXT("DependencyStringHint",
	                                               "Dependency string of the mod you want to download");

	const inline FText FailedToDeserializeDependencyJson = LOCTEXT("FailedToDeserializeJson",
	                                                            "Failed to deserialize dependency json");

	const inline FText FetchingDependency = LOCTEXT("FetchingDependency",
	                                             "Fetching dependency");

	const inline FText FailedToParseResponse = LOCTEXT("FailedToParseResponse", "Failed to parse server response");
	const inline FText FailedToFindMod = LOCTEXT("FailedToFindMod", "Failed to find mod");

	const inline FText ModDecompressionError_ZipOpen = LOCTEXT("ZipOpenError",
	                                                        "Mod decompression error (Zip Open)");
	const inline FText ModDecompressionError_MissingSources = LOCTEXT("ZipMissingSources",
	                                                               "Mod decompression error (Missing sources)");

	const inline FText FailedToUnloadPackages = LOCTEXT("UnloadFailed",
	                                                 "Failed to unload all packages");

	const inline FText FailedToSave = LOCTEXT("FailedToSave", "Failed to save some files");
	const inline FText FailedToReload = LOCTEXT("FailedToReload", "Failed to reload some assets");

	const inline FText SuccessfulDownload = LOCTEXT("SuccessfulDependencyFetch",
	                                             "Successfully downloaded mod dependency");
}

#undef LOCTEXT_NAMESPACE