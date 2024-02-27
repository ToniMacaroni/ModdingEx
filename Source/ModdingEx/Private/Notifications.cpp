#include "Notifications.h"

#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"

#define LOCTEXT_NAMESPACE "Notifications"

namespace Notifications
{
	void ShowFailNotification(const FText& Text, bool bShowOutputLog)
	{
		FNotificationInfo Info(Text);
		Info.ExpireDuration = 5.0f;
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;

		if (bShowOutputLog)
		{
			Info.Hyperlink = FSimpleDelegate::CreateLambda([]
			{
				FGlobalTabmanager::Get()->TryInvokeTab(FName("OutputLog"));
			});
			Info.HyperlinkText = LOCTEXT("ShowOutputLogHyperlink", "Show Output Log");
		}

		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Fail);
	}

	void ShowSuccessNotification(const FText& Text)
	{
		FNotificationInfo Info(Text);
		Info.ExpireDuration = 2.5f;
		Info.bFireAndForget = true;
		Info.bUseSuccessFailIcons = true;

		FSlateNotificationManager::Get().AddNotification(Info)->SetCompletionState(SNotificationItem::CS_Success);
	}
}

#undef LOCTEXT_NAMESPACE