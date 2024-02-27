#pragma once

struct FModdingExSection
{
	FModdingExSection(FName ExtensionHook, TAttribute<FText> HeadingText,
	                  TArray<TSharedPtr<FUICommandInfo>> Commands = TArray<TSharedPtr<FUICommandInfo>>()) :
		ExtensionHook(std::move(ExtensionHook)),
		HeadingText(std::move(HeadingText)),
		Entries(std::move(Commands))
	{
	}

	FName ExtensionHook;
	TAttribute<FText> HeadingText;

	TArray<TSharedPtr<FUICommandInfo>> Entries;
};
