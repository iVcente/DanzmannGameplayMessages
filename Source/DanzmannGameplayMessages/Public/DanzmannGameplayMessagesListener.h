// Copyright (C) 2025 Vicente Danzmann. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"

#include "DanzmannGameplayMessagesListener.generated.h"

/**
 * Enum used to set matching rule for Gameplay Message listeners.
 */
UENUM(BlueprintType)
enum class EDanzmannGameplayMessagesMatchCriteria : uint8
{
    // An exact match will only receive Gameplay Messages with exactly the same channel
    // (e.g., registering for "A.B" will match a broadcast of "A.B" but not "A.B.C")
    ExactMatch,

    // A partial match will receive any Gameplay Messages rooted in the same channel
    // (e.g., registering for "A.B" will match a broadcast of "A.B" as well as "A.B.C")
    PartialMatch
};

/**
 * A handle that can be used to remove a previously registered Gameplay Messages listener.
 * @see UDanzmannGameplayMessagesSubsystem::RegisterListener() and UDanzmannGameplayMessagesSubsystem::UnregisterListener().
 */
USTRUCT(BlueprintType)
struct DANZMANNGAMEPLAYMESSAGES_API FDanzmannGameplayMessagesListenerHandle
{
    GENERATED_BODY()

    /**
     * Allow UDanzmannGameplayMessagesSubsystem access to protected/private members. 
     */
    friend class UDanzmannGameplayMessagesSubsystem;
	
    public:
        FDanzmannGameplayMessagesListenerHandle()
        {
        }

        /**
         * Check if listener is valid.
         * @return Whether handle is valid or not.
         */
        bool IsValid() const
        {
            return Id != 0;
        }

    private:
        FDanzmannGameplayMessagesListenerHandle(const FGameplayTag Channel, const int32 Id):
            Channel(Channel), Id(Id)
        {
        }

        /**
         * Channel this listener is registered to.
         */
        UPROPERTY(Transient)
        FGameplayTag Channel = FGameplayTag();

        /**
         * Listener handle ID.
         */
        UPROPERTY(Transient)
        int32 Id = 0;
};

/** 
 * Struct to store entry information for a single registered listener.
 */
USTRUCT()
struct FDanzmannGameplayMessagesListenerData
{
    GENERATED_BODY()
    
    /**
     * Listener handle ID.
     */
    int32 HandleId = 0;
    
    /**
     * Listener callback for when a Gameplay Message has been received.
     */
    TFunction<void(FGameplayTag, const UScriptStruct*, const void*)> Callback;
	
    /**
     * Listener Gameplay Message struct type.
     */
    TWeakObjectPtr<const UScriptStruct> GameplayMessageStructType = nullptr;

    /**
     * Whether listener Gameplay Message struct type is valid.
     */
    bool bHasValidType = false;

    /**
     * Listener Gameplay Message match criteria. 
     */
    EDanzmannGameplayMessagesMatchCriteria MatchCriteria = EDanzmannGameplayMessagesMatchCriteria::ExactMatch;
};
