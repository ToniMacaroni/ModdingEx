#include "FModdingExSettingsCustomization.h"

void FModdingExSettingsCustomization::CustomizeModManager(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory("Mod Manager", FText::GetEmpty(), ECategoryPriority::Important);
	
	// Manage how the settings are displayed based on the selected mod managers
    const TSharedPtr<IPropertyHandle> bUsingCurseforgeProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bUsingCurseforge));
    const TSharedPtr<IPropertyHandle> bUsingThunderstoreProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bUsingThunderstore));
    const TSharedPtr<IPropertyHandle> stagingDirProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, PrepStagingDir));
    const TSharedPtr<IPropertyHandle> bOpenReadmeAfterPrepProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bOpenReadmeAfterPrep));
    const TSharedPtr<IPropertyHandle> bAlwaysBuildBeforePrepProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bAlwaysBuildBeforePrep));
    const TSharedPtr<IPropertyHandle> bPrepModWhenContentIsSameProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bPrepModWhenContentIsSame));
    const TSharedPtr<IPropertyHandle> bAlwaysBuildBeforeZippingProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bAlwaysBuildBeforeZipping));

    bUsingCurseforgeProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&DetailBuilder]() { DetailBuilder.ForceRefreshDetails(); }));
    bUsingThunderstoreProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&DetailBuilder]() { DetailBuilder.ForceRefreshDetails(); }));

	bool bUsingCurseforgeValue;
	bool bUsingThunderstoreValue;
	bUsingCurseforgeProperty->GetValue(bUsingCurseforgeValue);
	bUsingThunderstoreProperty->GetValue(bUsingThunderstoreValue);

	if ((bUsingCurseforgeValue && !bUsingThunderstoreValue) || (!bUsingCurseforgeValue && !bUsingThunderstoreValue))
	{
		stagingDirProperty->MarkHiddenByCustomization();
		bOpenReadmeAfterPrepProperty->MarkHiddenByCustomization();
		bAlwaysBuildBeforePrepProperty->MarkHiddenByCustomization();
		bPrepModWhenContentIsSameProperty->MarkHiddenByCustomization();
	}

	if (bUsingThunderstoreValue && !bUsingCurseforgeValue)
	{
		bAlwaysBuildBeforeZippingProperty->MarkHiddenByCustomization();
		bAlwaysBuildBeforeZippingProperty->SetValue(false);
	}

	if (bUsingThunderstoreValue)
	{
		bUsingThunderstoreProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&]()
		{
			// Refresh the tool menus
			const FModdingExModule& ModdingExModule = FModuleManager::LoadModuleChecked<FModdingExModule>("ModdingEx");
			ModdingExModule.OnModManagerChanged.ExecuteIfBound();
		}));
	}
}

void FModdingExSettingsCustomization::CustomizeHashSettings(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory("Hash Check", FText::GetEmpty(), ECategoryPriority::Default);

	const TSharedPtr<IPropertyHandle> bShouldCheckHashProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bShouldCheckHash));
	const TSharedPtr<IPropertyHandle> bZipWhenContentIsSameProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bZipWhenContentIsSame));
	const TSharedPtr<IPropertyHandle> bPrepModWhenContentIsSameProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bPrepModWhenContentIsSame));
	const TSharedPtr<IPropertyHandle> bDontCheckHashOnGameStartProperty = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UModdingExSettings, bDontCheckHashOnGameStart));

	bShouldCheckHashProperty->SetOnPropertyValueChanged(FSimpleDelegate::CreateLambda([&DetailBuilder]() { DetailBuilder.ForceRefreshDetails(); }));
	
	bool bShouldCheckHashValue;
	bShouldCheckHashProperty->GetValue(bShouldCheckHashValue);

	if (!bShouldCheckHashValue)
	{
		bZipWhenContentIsSameProperty->MarkHiddenByCustomization();
		bDontCheckHashOnGameStartProperty->MarkHiddenByCustomization();
		bPrepModWhenContentIsSameProperty->MarkHiddenByCustomization();
	}
}

void FModdingExSettingsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	// Idiotic lazy setting category priorities, I'm so sorry
	DetailBuilder.EditCategory("General", FText::GetEmpty(), ECategoryPriority::Variable);
	DetailBuilder.EditCategory("Building", FText::GetEmpty(), ECategoryPriority::Transform);
	// Mod managers is Important
	DetailBuilder.EditCategory("Zipping", FText::GetEmpty(), ECategoryPriority::TypeSpecific);
	// Hash check is Default
	DetailBuilder.EditCategory("Advanced", FText::GetEmpty(), ECategoryPriority::Uncommon);
	DetailBuilder.EditCategory("Misc", FText::GetEmpty(), ECategoryPriority::Uncommon);
	
	CustomizeModManager(DetailBuilder);
	CustomizeHashSettings(DetailBuilder);	
}
