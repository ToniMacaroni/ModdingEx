#pragma once

#include "CoreMinimal.h"

class StartupDialog
{
private:
	TSharedPtr<SWindow> Window;
	inline static TSharedPtr<StartupDialog> Instance;
	
public:

	static void ShowDialog(const FText& Title, const FText& HeaderText, const FText& Message);
	void CreateWindow(const FText& Title, const FText& HeaderText, const FText& Message);
};
