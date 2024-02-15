#include "StartupDialog.h"

#include "ModdingExStyle.h"
#include "Widgets/Input/SHyperlink.h"

#define LOCTEXT_NAMESPACE "StartupDialog"

void StartupDialog::ShowDialog(const FText& Title, const FText& HeaderText, const FText& Message)
{
	if(Instance.IsValid())
	{
		if(Instance->Window.IsValid())
			Instance->Window->RequestDestroyWindow();
		return;
	}
	
	Instance = MakeShareable(new StartupDialog());
	Instance->CreateWindow(Title, HeaderText, Message);
}

void StartupDialog::CreateWindow(const FText& Title, const FText& HeaderText, const FText& Message)
{
	FSlateFontInfo HeaderFont = FCoreStyle::GetDefaultFontStyle("Regular", 16);
	FSlateFontInfo ContentFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);
	
	const TSharedPtr<SWindow> Dialog = SNew(SWindow)
						.Title(Title)
						.SupportsMaximize(false)
						.SupportsMinimize(false)
						.FocusWhenFirstShown(true)
						.SizingRule(ESizingRule::Autosized)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(0.25f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.Padding(10, 0, 10, 20)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(HeaderText)
				.Font(HeaderFont)
				.AutoWrapText(true)
				.Justification(ETextJustify::Center)
			]
			+ SVerticalBox::Slot()
			.Padding(10, 0)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(Message)
				.Font(ContentFont)
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.75f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.Padding(10.0f, 0.0f, 5.0f, 0.0f)
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SImage).Image(FModdingExStyle::Get().GetBrush("ModdingEx.Github"))
				]
				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.AutoWidth()
				[
					SNew(SHyperlink)
					.Text(FText::FromString("ToniMacaroni/ModdingEx"))
					.ToolTipText(FText::FromString("Go to the website in your default browser."))
					.OnNavigate(FSimpleDelegate::CreateLambda([&]
					{
						FPlatformProcess::LaunchURL(TEXT("https://github.com/ToniMacaroni/ModdingEx"), nullptr, nullptr);
					}))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(SSpacer)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f, 3.0f, 7.0f, 7.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("GotIt", "Got it"))
					.ContentPadding(7)
					.OnClicked_Lambda([&]
					{
						Dialog->RequestDestroyWindow();
						return FReply::Handled();
					})
				]
			]
		];

	FSlateApplication::Get().AddModalWindow(Dialog.ToSharedRef(), nullptr);
}

#undef LOCTEXT_NAMESPACE