// Copyright (C) 2025 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "DanzmannGameplayMessagesListener.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "DanzmannGameplayMessagesSubsystem.generated.h"

class UDanzmannAsyncAction_ListenForGameplayMessages;
struct FFrame;

/**
 * This subsystem implements a decoupled messaging framework that allows senders (event raisers) and
 * listeners to communicate without having to know about each other directly by broadcasting and
 * receiving structured messages (Gameplay Messages) on named channels -- though they must agree on the format of the
 * message (as a USTRUCT() type).
 * Listeners can register to specific Gameplay Message types and Gameplay Tag-based channels without needing
 * direct references to the senders.
 * You can get the subsystem as the following:
 *  - UGameInstance::GetSubsystem<UDanzmannGameplayMessagesSubsystem>(WorldContextObject);
 *  - GetGameInstance()->GetSubsystem<UDanzmannGameplayMessagesSubsystem>();
 *  - UDanzmannGameplayMessagesSubsystem::Get(WorldContextObject);
 *
 * Note that call order when there are multiple listeners for the same channel is
 * not guaranteed and can change over time!
 */
UCLASS()
class DANZMANNGAMEPLAYMESSAGES_API UDanzmannGameplayMessagesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

	public:
		/**
		 * @see more info in USubsystem.
		 */
		virtual void Deinitialize() override;
	
		/**
		 * Get a reference to Gameplay Messages Subsystem according to the Game Instance associated with the world of the specified object.
		 * @return The Gameplay Messages Subsystem.
		 */
		static UDanzmannGameplayMessagesSubsystem& Get(const UObject* WorldContextObject);

		/**
		 * Check if there is a valid instance of the Gameplay Messages Subsystem associated with the world of the specified object.
		 * @return Whether Gameplay Messages Subsystem is valid or not.
		 */
		static bool HasInstance(const UObject* WorldContextObject);

		/**
		 * Broadcast a Gameplay Message on the specified channel.
		 * @tparam TGameplayMessage Gameplay Message of UScriptStrict type (USTRUCT()).
		 * @param Channel The Gameplay Message channel to broadcast on.
		 * @param GameplayMessage The Gameplay Message to send.
		 * @note GameplayMessage must be the same type of UScriptStruct expected by the listeners for this channel, otherwise an error will be logged.
		 */
		template<typename TGameplayMessage>
		void BroadcastGameplayMessage(const FGameplayTag Channel, const TGameplayMessage& GameplayMessage)
		{
			const UScriptStruct* MessageStruct = TBaseStructure<TGameplayMessage>::Get();
			BroadcastGameplayMessage_Internal(Channel, MessageStruct, &GameplayMessage);
		}

		/**
		 * Broadcast a Gameplay Message on the specified channel (BP version).
		 * @param Channel The Gameplay Message channel to broadcast on.
		 * @param GameplayMessage The Gameplay Message to send/broadcast.
		 * @note Function must be the same type of UScriptStruct provided by broadcasters for this channel, otherwise an error will be logged.
		 */
		UFUNCTION(BlueprintCallable, CustomThunk, Category = "Dancing Man|Gameplay Messages", DisplayName = "Broadcast Gameplay Message", Meta = (CustomStructureParam = "GameplayMessage", AllowAbstract = false))
		void BP_BroadcastGameplayMessage(const FGameplayTag Channel, const int32& GameplayMessage);

		/**
		 * By exposing a function to BP, Unreal will auto-generate a new version -- DECLARE_FUNCTION (exec...) -- of the native function. This is the actual function used by BP virtual machine (VM).
		 * You can find these auto-generated functions within .gen.cpp files.
		 * What we're doing here is adding the CustomThunk specifier to the native function we'd like to expose and providing a custom DECLARE_FUNCTION (exec...) for it.
		 * This way we can do our own implementation of the BP VM function, allowing us to parse the function parameters and enable a wildcard node for it.
		 */
		DECLARE_FUNCTION(execBP_BroadcastGameplayMessage);
	
		/**
	     * Register to receive Gameplay Messages on a specified channel and use a lambda function as callback.
		 * @tparam TGameplayMessage Gameplay Message of UScriptStrict type (USTRUCT()).
		 * @param Channel The Gameplay Message channel to listen to.
		 * @param Callback Function to call when Gameplay Message is received.
		 * @param ChannelMatchCriteria Callback will be triggered if any Gameplay Message is broadcast to Channel and Channel match given criteria.
		 * @return Handle that can be used to unregister this listener -- by calling UnregisterListener() on the subsystem.
		 * @note The provided Callback must match the exact UScriptStruct used by message broadcasters on this channel. Type mismatches will result in logged runtime warnings and Gameplay Message drops.
		 * @note Usage example:
		 *       RegisterListener<FGameplayMessageStructForChannel>(
		 *           FGameplayTag(),
		 *           []
		 *           (const FGameplayTag Channel, const FGameplayMessageStructForChannel& GameplayMessage)
		 *           {
		 *  	         // Do something...
		 *           }
		 *       );
		 */
		template<typename TGameplayMessage>
		FDanzmannGameplayMessagesListenerHandle RegisterListener(const FGameplayTag Channel, TFunction<void(const FGameplayTag, const TGameplayMessage&)>&& Callback, const EDanzmannGameplayMessagesMatchCriteria ChannelMatchCriteria = EDanzmannGameplayMessagesMatchCriteria::ExactMatch)
		{
			auto GenericCallback =
				[RegisteredCallback = MoveTemp(Callback)]
				(const FGameplayTag ChannelToRegister, const UScriptStruct* GameplayMessageStructType, const void* GameplayMessagePayload)
				{
					RegisteredCallback(ChannelToRegister, *static_cast<const TGameplayMessage*>(GameplayMessagePayload));
				};

			const UScriptStruct* GameplayMessageStructType = TBaseStructure<TGameplayMessage>::Get();
			return RegisterListener_Internal(Channel, GenericCallback, GameplayMessageStructType, ChannelMatchCriteria);
		}

		/**
		 * Register to receive Gameplay Messages on a specified channel and use a specified member function as callback.
		 * @tparam TListener Listener of UObject type.
		 * @tparam TGameplayMessage Gameplay Message of UScriptStrict type (USTRUCT()).
		 * @param Channel The Gameplay Message channel to listen to.
		 * @param Listener The object instance to call the function on.
		 * @param Callback Member function to call when Gameplay Message is received.
		 * @param ChannelMatchCriteria Callback will be triggered if any Gameplay Message is broadcast to Channel and Channel match given criteria.
		 * @return Handle that can be used to unregister this listener -- by calling UnregisterListener() on the subsystem.
		 * @note The provided Callback must match the exact UScriptStruct used by message broadcasters on this channel. Type mismatches will result in logged runtime warnings and message drops.
		 * @note The object registering the callback function will be checked if it still exists before triggering the callback.
	     * @note Usage example:
	     *       RegisterListener(
	     *           FGameplayTag(),
	     *           this,
	     *           &ThisClass::CallbackFunction
	     *       );
		 */
		template<typename TListener = UObject, typename TGameplayMessage>
		FDanzmannGameplayMessagesListenerHandle RegisterListener(const FGameplayTag Channel, TListener* Listener, void(TListener::* Callback)(const FGameplayTag, const TGameplayMessage&), const EDanzmannGameplayMessagesMatchCriteria ChannelMatchCriteria = EDanzmannGameplayMessagesMatchCriteria::ExactMatch)
		{
			TWeakObjectPtr<TListener> WeakListener = Listener;
			
			auto GenericCallback =
				[WeakListener, Callback]
				(const FGameplayTag ChannelToRegister, const TGameplayMessage& GameplayMessage)
				{
					if (WeakListener.IsValid())
					{
						TListener* StrongListener = WeakListener.Get();
						(StrongListener->*Callback)(ChannelToRegister, GameplayMessage);
					}
				};
			
			return RegisterListener<TGameplayMessage>(Channel, GenericCallback, ChannelMatchCriteria);
		}

		/**
		 * Remove a Gameplay Message listener previously registered by RegisterListener().
		 * @param Handle The handle returned by RegisterListener().
		 */
		void UnregisterListener(FDanzmannGameplayMessagesListenerHandle Handle);

	private:
		/**
	     * Internal helper for broadcasting a Gameplay Message. 
		 * @param Channel The Gameplay Message channel to broadcast on.
		 * @param GameplayMessageStructType The Gameplay Message struct type.
		 * @param GameplayMessagePayload The Gameplay Message content.
		 */
		void BroadcastGameplayMessage_Internal(const FGameplayTag Channel, const UScriptStruct* GameplayMessageStructType, const void* GameplayMessagePayload);

		/**
		 * Internal helper for registering a Gameplay Message listener.
		 * @param Channel Gameplay Message channel to listen.
		 * @param Callback Function to be triggered when Gameplay Message is broadcast to.
		 * @param GameplayMessageStructType Gameplay Message struct type.
		 * @param ChannelMatchCriteria Criteria to match Channel.
		 * @return Listener handle.
		 */
		FDanzmannGameplayMessagesListenerHandle RegisterListener_Internal(const FGameplayTag Channel, TFunction<void(FGameplayTag, const UScriptStruct*, const void*)>&& Callback, const UScriptStruct* GameplayMessageStructType, const EDanzmannGameplayMessagesMatchCriteria ChannelMatchCriteria);

		/**
		 * Internal helper for unregistering a Gameplay Message listener.
		 * @param Channel Channel to unregister listener.
		 * @param HandleId Listener's handle ID.
		 */
		void UnregisterListener_Internal(const FGameplayTag Channel, const int32 HandleId);
		
		/**
		 * Struct to store a list of all entries for a given channel.
		 */
		struct FDanzmannChannelListenerList
		{
			/**
			 * Channel's listeners.
			 */
			TArray<FDanzmannGameplayMessagesListenerData> Listeners;

			/**
			 * Available handle ID for listener. This value is incremented each time a new listener is registered.
			 */
			int32 AvailableHandleId = 0;
		};

		/**
		 * Map of channels to their respective listeners. 
		 */
		TMap<FGameplayTag, FDanzmannChannelListenerList> ListenerMap;
};
