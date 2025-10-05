// Copyright (C) 2025 Vicente Danzmann. All Rights Reserved.

#include "DanzmannGameplayMessagesGameInstanceSubsystem.h"
#include "DanzmannLogGameplayMessages.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/ScriptMacros.h"
#include "UObject/Stack.h"

void UDanzmannGameplayMessagesGameInstanceSubsystem::Deinitialize()
{
	ListenerMap.Reset();

	Super::Deinitialize();
}

UDanzmannGameplayMessagesGameInstanceSubsystem* UDanzmannGameplayMessagesGameInstanceSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	checkf(IsValid(World), TEXT("[%hs] World is not valid."), __FUNCTION__);
	
	UDanzmannGameplayMessagesGameInstanceSubsystem* GameplayMessagesSubsystem = World->GetGameInstance()->GetSubsystem<UDanzmannGameplayMessagesGameInstanceSubsystem>();
	checkf(IsValid(GameplayMessagesSubsystem), TEXT("[%hs] Gameplay Message Subsystem is not valid."), __FUNCTION__);
	
	return GameplayMessagesSubsystem;
}

bool UDanzmannGameplayMessagesGameInstanceSubsystem::HasInstance(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	const UDanzmannGameplayMessagesGameInstanceSubsystem* GameplayMessagesSubsystem = IsValid(World) ? World->GetGameInstance()->GetSubsystem<UDanzmannGameplayMessagesGameInstanceSubsystem>() : nullptr;
	return IsValid(GameplayMessagesSubsystem);
}

void UDanzmannGameplayMessagesGameInstanceSubsystem::BroadcastGameplayMessage_Internal(const FGameplayTag Channel, const UScriptStruct* GameplayMessageStructType, const void* GameplayMessagePayload)
{
	// Log the broadcast details if we have increased LogDanzmannGameplayMessages verbosity
	if (UE_LOG_ACTIVE(LogDanzmannGameplayMessages, Verbose))
	{
		const FString* ContextString = nullptr;
		#if WITH_EDITOR
			if (GIsEditor)
			{
				extern ENGINE_API FString GPlayInEditorContextString;
				ContextString = &GPlayInEditorContextString;
			}
		#endif

		FString HumanReadableMessage;
		GameplayMessageStructType->ExportText(
			HumanReadableMessage,
			GameplayMessagePayload,
			nullptr,
			nullptr,
			PPF_None,
			nullptr
		);
		UE_LOG(LogDanzmannGameplayMessages, Verbose, TEXT("Broadcasting Gameplay Message (%s, %s, %s)..."), ContextString != nullptr ? **ContextString : *GetPathNameSafe(this), *Channel.ToString(), *HumanReadableMessage);
	}

	// Broadcast the Gameplay Message
	bool bOnInitialTag = true;
	for (FGameplayTag Tag = Channel; Tag.IsValid(); Tag = Tag.RequestDirectParent())
	{
		if (const FDanzmannChannelListenerList* ListenersList = ListenerMap.Find(Tag))
		{
			// Copy in case there are removals while handling callbacks
			TArray<FDanzmannGameplayMessagesListenerData> Listeners = ListenersList->Listeners;

			for (const FDanzmannGameplayMessagesListenerData& Listener : Listeners)
			{
				if (bOnInitialTag || (Listener.MatchCriteria == EDanzmannGameplayMessagesMatchCriteria::PartialMatch))
				{
					if (Listener.bHasValidType && !Listener.GameplayMessageStructType.IsValid())
					{
						UE_LOG(LogDanzmannGameplayMessages, Warning, TEXT("[%hs] Listener Gameplay Message struct type has gone invalid on channel %s. Removing listener from list."), __FUNCTION__, *Channel.ToString());
						UnregisterListener_Internal(Channel, Listener.HandleId);
						continue;
					}

					// The receiving type must be either a parent of the sending type or completely ambiguous (for internal use)
					if (!Listener.bHasValidType || GameplayMessageStructType->IsChildOf(Listener.GameplayMessageStructType.Get()))
					{
						Listener.Callback(Channel, GameplayMessageStructType, GameplayMessagePayload);
					}
					else
					{
						UE_LOG(LogDanzmannGameplayMessages, Error, TEXT("[%hs] Gameplay Message struct type mismatch on channel %s. Broadcast type %s, listener at %s was expecting type %s."), __FUNCTION__, *Channel.ToString(), *GameplayMessageStructType->GetPathName(), *Tag.ToString(), *Listener.GameplayMessageStructType->GetPathName());
					}
				}
			}
		}
		
		bOnInitialTag = false;
	}
}

void UDanzmannGameplayMessagesGameInstanceSubsystem::BP_BroadcastGameplayMessage(const FGameplayTag Channel, const int32& GameplayMessage)
{
	// This will never be called, the exec version below will be hit instead
	checkNoEntry();
}

DEFINE_FUNCTION(UDanzmannGameplayMessagesGameInstanceSubsystem::execBP_BroadcastGameplayMessage)
{
	// Read the first parameter: a regular FGameplayTag passed from Blueprint
	P_GET_STRUCT(FGameplayTag, Channel);
	
	// Reset the pointer before assigning it again
	Stack.MostRecentPropertyAddress = nullptr;
	
	// Tell the Blueprint VM to advance the parameter stack and evaluate the next input -- in this case, a wildcard struct as a custom payload.
	// Stack.MostRecentPropertyAddress and Stack.MostRecentProperty will be updated and will point to the payload data and its type.
	// This allows any user-defined struct to be passed from Blueprint
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	
	// Extract the raw pointer to the data (void*) and its type information (FStructProperty) so we can interpret the payload correctly
	const void* GameplayMessagePtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);

	// Required macro that completes the parameter-parsing phase
	P_FINISH;

	// Check if value passed to wildcard pin is the expected one -- USTRUCT()
	if (ensureMsgf((StructProperty != nullptr) && (StructProperty->Struct != nullptr) && (GameplayMessagePtr != nullptr), TEXT("Dancing Man Gameplay Messages | %s class calls %s function with an invalid type into \"Gameplay Message\" wildcard parameter. Its type must be of UScriptStruct (USTRUCT())."), *Stack.Object->GetClass()->GetName(), *Stack.CurrentNativeFunction->GetName()))
	{
		// Call native function with parsed parameters
		P_THIS->BroadcastGameplayMessage_Internal(Channel, StructProperty->Struct, GameplayMessagePtr);
	}
}

FDanzmannGameplayMessagesListenerHandle UDanzmannGameplayMessagesGameInstanceSubsystem::RegisterListener_Internal(const FGameplayTag Channel, TFunction<void(FGameplayTag, const UScriptStruct*, const void*)>&& Callback, const UScriptStruct* GameplayMessageStructType, const EDanzmannGameplayMessagesMatchCriteria ChannelMatchCriteria)
{
	FDanzmannChannelListenerList& ListenersList = ListenerMap.FindOrAdd(Channel);

	FDanzmannGameplayMessagesListenerData& Entry = ListenerMap.FindOrAdd(Channel).Listeners.AddDefaulted_GetRef();
	Entry.Callback = MoveTemp(Callback);
	Entry.GameplayMessageStructType = GameplayMessageStructType;
	Entry.bHasValidType = GameplayMessageStructType != nullptr;
	Entry.HandleId = ++ListenersList.AvailableHandleId;
	Entry.MatchCriteria = ChannelMatchCriteria;

	return FDanzmannGameplayMessagesListenerHandle(Channel, Entry.HandleId);
}

void UDanzmannGameplayMessagesGameInstanceSubsystem::UnregisterListener(const FDanzmannGameplayMessagesListenerHandle Handle)
{
	if (Handle.IsValid())
	{
		UnregisterListener_Internal(Handle.Channel, Handle.Id);
	}
	else
	{
		UE_LOG(LogDanzmannGameplayMessages, Warning, TEXT("[%hs] Trying to unregister an invalid handle."), __FUNCTION__);
	}
}

void UDanzmannGameplayMessagesGameInstanceSubsystem::UnregisterListener_Internal(const FGameplayTag Channel, int32 HandleId)
{
	if (FDanzmannChannelListenerList* ListenersList = ListenerMap.Find(Channel))
	{
		const int32 MatchIndex = ListenersList->Listeners.IndexOfByPredicate(
			[Id = HandleId]
			(const FDanzmannGameplayMessagesListenerData& Other)
			{
				return Other.HandleId == Id;
			}
		);
		
		if (MatchIndex != INDEX_NONE)
		{
			ListenersList->Listeners.RemoveAtSwap(MatchIndex);
		}

		if (ListenersList->Listeners.Num() == 0)
		{
			ListenerMap.Remove(Channel);
		}
	}
}
