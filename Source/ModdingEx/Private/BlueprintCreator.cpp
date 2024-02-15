#include "BlueprintCreator.h"

#include "EdGraphNode_Comment.h"
#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_Event.h"
#include "ModdingExSettings.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ScopedSlowTask.h"
#include "Serialization/JsonSerializer.h"
#include "Widgets/Notifications/SNotificationList.h"

bool UBlueprintCreator::SplitPathToName(const FString& Path, FString& OutName)
{
	int32 LastSlash = INDEX_NONE;
	Path.FindLastChar('/', LastSlash);
	if(LastSlash != INDEX_NONE)
	{
		OutName = Path.RightChop(LastSlash + 1);
		return true;
	}
	
	OutName = Path;
	return false;
}

void UBlueprintCreator::CleanBlueprintPath(const FString& Path, FString& OutPath, FString& OutName)
{
	OutPath = FPaths::GetBaseFilename(Path, false);
	OutName = FPaths::GetCleanFilename(OutPath);
	OutPath.ReplaceInline(*FString::Format(TEXT("{0}/Content"), {FApp::GetProjectName()}), TEXT("/Game"));
}

bool UBlueprintCreator::CreateGameBlueprint(const FString& Path, const FString& ClassName, UBlueprint*& OutBlueprint)
{
	FString PathWithoutExt = FPaths::GetBaseFilename(Path, false);
	const FString BPName = FPaths::GetCleanFilename(PathWithoutExt);

	PathWithoutExt.ReplaceInline(*FString::Format(TEXT("{0}/Content"), {FApp::GetProjectName()}), TEXT("/Game"));
	
	UE_LOG(LogTemp, Log, TEXT("Creating blueprint at %s"), *Path);
	UE_LOG(LogTemp, Log, TEXT(" - Without Ext: %s"), *PathWithoutExt);
	UE_LOG(LogTemp, Log, TEXT(" - Name: %s"), *BPName);

	UClass* Class = nullptr;

	if(ClassName.IsEmpty())
		Class = UObject::StaticClass();
	
	else if(ClassName.StartsWith("{"))
	{
		TSharedPtr<FJsonObject> JsonObj;
		const FString JsonString = ClassName.Replace(TEXT("\"SuperStruct\": "), TEXT(""));
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if(FJsonSerializer::Deserialize(Reader, JsonObj))
		{
			FString ObjectName;
			FString ObjectPath;
			if(!JsonObj->TryGetStringField("ObjectName", ObjectName))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get ObjectName from JSON"));
				return nullptr;
			}

			ObjectName.ReplaceInline(TEXT("Class'"), TEXT(""));
			ObjectName.ReplaceInline(TEXT("'"), TEXT(""));

			if(!JsonObj->TryGetStringField("ObjectPath", ObjectPath))
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get ObjectPath from JSON"));
				return nullptr;
			}

			Class = FindObject<UClass>(ANY_PACKAGE, *(ObjectPath + "." + ObjectName));
		}
	}
	else
	{
		Class = FindObject<UClass>(ANY_PACKAGE, *ClassName);
	}

	if(!Class)
	{
		UE_LOG(LogTemp, Error, TEXT("Class %s not found"), *ClassName);
		return nullptr;
	}

	OutBlueprint = CreateBlueprint(PathWithoutExt, BPName, Class);

	if(!OutBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint %s"), *BPName);
		return false;
	}

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(OutBlueprint, EBlueprintCompileOptions::None, &Results);

	if(Results.NumErrors > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Blueprint %s had errors during compilation"), *BPName);
		return false;
	}

	return true;
}

static UK2Node_CustomEvent* CreateEventNode(UEdGraph* Graph, const FName& EventName)
{
	const auto NewNode = FEdGraphSchemaAction_K2NewNode::SpawnNode<UK2Node_CustomEvent>(Graph, Graph->GetGoodPlaceForNewNode(), EK2NewNodeFlags::SelectNewNode, [](UK2Node_Event* UK2Node_Event){});
	NewNode->CustomFunctionName = EventName;
	return NewNode;
}

static UEdGraphNode_Comment* CreateCommentNode(UEdGraph* Graph, const FString& Comment, const FVector4& Transform)
{
	const auto Template = NewObject<UEdGraphNode_Comment>(Graph);
	
	Template->NodeComment = Comment;
	Template->NodePosX = Transform.X;
	Template->NodePosY = Transform.Y;
	Template->NodeWidth = Transform.Z;
	Template->NodeHeight = Transform.W;

	Graph->AddNode(Template, false, false);
	
	return Template;
}

static UEdGraphNode_Comment* CreateCommentNode(UEdGraph* Graph, const FString& Comment, const UEdGraphNode* NodeToWrap)
{
	const auto NewNode = CreateCommentNode(Graph, Comment, FVector4(NodeToWrap->NodePosX - 100, NodeToWrap->NodePosY - 100, 400, 250));
	return NewNode;
}

static void AddVar(UBlueprint* Blueprint, const FName& VarName, const FName& VarType, const FString& DefaultValue = TEXT(""), bool bIsInstanceEditable = false)
{
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, VarName, FEdGraphPinType(VarType, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType()), DefaultValue);

	if(bIsInstanceEditable)
		FBlueprintEditorUtils::SetBlueprintOnlyEditableFlag(Blueprint, VarName, false);
}

static void InitCustomEvents(const UBlueprint* Blueprint, UEdGraph* Graph)
{
	if(!Graph)
	{
		Graph = Blueprint->GetLastEditedUberGraph();
	}

	const auto Settings = GetDefault<UModdingExSettings>();

	for (const FCustomModActorEvent& Event : Settings->NewEventsToAdd)
	{
		if(!Event.bIsEnabled)
			continue;

		const auto EventName = Event.EventName.ToString();

		UK2Node_CustomEvent* NewEvent =	CreateEventNode(Graph, Event.EventName);

		if(EventName == "PrintToModLoader")
		{
			NewEvent->CreateUserDefinedPin(FName("Message"), FEdGraphPinType(UEdGraphSchema_K2::PC_String, NAME_None, nullptr, EPinContainerType::None, false, FEdGraphTerminalType()), EGPD_Output);
			NewEvent->NodeComment = "Don't touch. Call this event to log something in the mod loader.";

			UEdGraphNode_Comment* Comment = CreateCommentNode(Graph, "Don't touch. Call this event to log something in the mod loader.", NewEvent);
			Comment->CommentColor = FLinearColor::Gray;
			continue;
		}

		if(EventName == "PreBeginPlay")
		{
			NewEvent->NodeComment = "This event is too early to use to initialize your mod";
			continue;
		}

		if(EventName == "PostBeginPlay")
		{
			NewEvent->NodeComment = " This event is fired when the Player Controller Begin Play is called. Suggested for initialization.";
			continue;
		}
	}
}

bool UBlueprintCreator::CreateModBlueprint(const FString& ModName, UClass* Class, UBlueprint*& OutBlueprint)
{
	FScopedSlowTask SlowTask(1, FText::FromString("Creating Mod Blueprint"));
	SlowTask.MakeDialog();
	SlowTask.EnterProgressFrame(1);
	
	const FString& FullPath = "/Game/Mods/" + ModName + "/ModActor";
	OutBlueprint = CreateBlueprint(FullPath, "ModActor", Class);

	if(!OutBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create blueprint for mod %s"), *ModName);
		return false;
	}
	
	UEdGraph* UberGraph = OutBlueprint->GetLastEditedUberGraph();
	InitCustomEvents(OutBlueprint, UberGraph);

	AddVar(OutBlueprint, FName("ModAuthor"), UEdGraphSchema_K2::PC_String, "You", true);
	AddVar(OutBlueprint, FName("ModDescription"), UEdGraphSchema_K2::PC_String, "A mod that does something", true);
	AddVar(OutBlueprint, FName("ModVersion"), UEdGraphSchema_K2::PC_String, "1.0.0", true);

	FCompilerResultsLog Results;
	FKismetEditorUtilities::CompileBlueprint(OutBlueprint, EBlueprintCompileOptions::None, &Results);

	if(Results.NumErrors > 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Blueprint %s had errors during compilation"), *ModName);
		return false;
	}
	
	FNotificationInfo Info( FText::FromString("Mod Created, Have Fun!") );
	Info.Image = FAppStyle::GetBrush(TEXT("LevelEditor.RecompileGameCode"));
	Info.FadeInDuration = 0.1f;
	Info.FadeOutDuration = 0.5f;
	Info.ExpireDuration = 3.5f;
	Info.bUseThrobber = false;
	Info.bUseSuccessFailIcons = true;
	Info.bUseLargeFont = true;
	Info.bFireAndForget = false;
	Info.bAllowThrottleWhenFrameRateIsLow = false;
	const auto NotificationItem = FSlateNotificationManager::Get().AddNotification( Info );
	NotificationItem->SetCompletionState(SNotificationItem::CS_Success);
	NotificationItem->ExpireAndFadeout();
	
	GEditor->PlayEditorSound(TEXT("/Engine/EditorSounds/Notifications/CompileSuccess_Cue.CompileSuccess_Cue"));

	return true;
}

UBlueprint* UBlueprintCreator::CreateBlueprint(const FString& Path, const FString& BlueprintName, UClass* Class)
{
	UPackage* Package = CreatePackage(*Path);
	UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
		Class,
		Package,
		*BlueprintName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass());
	
	if(Blueprint)
	{
		FAssetRegistryModule::AssetCreated(Blueprint);
		bool WasMarkedDirty = Package->MarkPackageDirty();
	}
	
	return Blueprint;
}
