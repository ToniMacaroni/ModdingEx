#pragma once
#include "IDetailCustomization.h"
#include "PropertyHandle.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "ModdingExSettings.h"
#include "ModdingEx.h"

class FModdingExSettingsCustomization : public IDetailCustomization
{
private:
	static void CustomizeModManager(IDetailLayoutBuilder& DetailBuilder);
	static void CustomizeHashSettings(IDetailLayoutBuilder& DetailBuilder);
	
public:
	static TSharedRef<IDetailCustomization> MakeInstance()
	{
		return MakeShareable(new FModdingExSettingsCustomization());
	}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
