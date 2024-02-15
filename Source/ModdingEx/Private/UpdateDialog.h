#pragma once

#include "CoreMinimal.h"

class UpdateDialog
{
private:
	TSharedPtr<SWindow> Window;
	inline static TSharedPtr<UpdateDialog> Instance;
	
public:

	static void CheckForUpdate();
	void CreateWindow();
};
