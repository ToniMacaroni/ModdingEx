#pragma once

namespace Notifications
{
	void ShowFailNotification(const FText& Text, bool bShowOutputLog = true);
	void ShowSuccessNotification(const FText& Text);
}