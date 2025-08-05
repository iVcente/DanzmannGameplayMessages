# Gameplay Messages
A plugin that contains a subsystem that allows listening for and broadcasting/sending messages between unconnected gameplay objects. The system relies on Gameplay Tags as channels and structs to transmit data.

---

### Usage Example

Make sure you have added the `GameplayTags` and `DanzmannGameplayMessages` modules to your project's `Build.cs` file. Then, create some Gameplay Tags:
```cpp
// MyProjectGameplayMessagesGameplayTags.h

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Gameplay Tags used as channels for Gameplay Messages.
 */
namespace MyProject::GameplayTags
{
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayMessages_LocalPlayerDeath);
    UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayMessages_LocalPlayerKilledNpc);
}

// ---------------------------------------------------------------------- //

// MyProjectGameplayMessagesGameplayTags.cpp

#include "GameplayMessages/MyProjectGameplayMessagesGameplayTags.h"

namespace MyProject::GameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayMessages_LocalPlayerDeath, "GameplayMessages.LocalPlayerDeath", "Channel used to broadcast and receive Gameplay Message that local player has died.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayMessages_LocalPlayerKilledNpc, "GameplayMessages.LocalPlayerKilledEnemy", "Channel used to broadcast and receive Gameplay Message that local player has killed an enemy.");
}
```

Once the channels are set up, you can broadcast and listen for Gameplay Messages as the following:
```cpp
// MyActor.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "MyActor.generated.h"

struct FGameplayTag;

/**
 * Struct to store PlayerDeath Gameplay Message.
 * You can add anything to your custom struct.
 */
USTRUCT(BlueprintType)
struct FMyProjectGameplayMessage_PlayerDeath
{
    GENERATED_BODY()
	
    UPROPERTY(BlueprintReadWrite)
    AActor* Killer = nullptr;
};

/**
 * Struct to store PlayerKilledEnemy Gameplay Message.
 * You can add anything to your custom struct.
 */
USTRUCT(BlueprintType)
struct FMyProjectGameplayMessage_PlayerKilledEnemy
{
    GENERATED_BODY()
	
    UPROPERTY(BlueprintReadWrite)
    AActor* Victim = nullptr;

    UPROPERTY(BlueprintReadWrite)
    AActor* WeaponUsed = nullptr;
};

UCLASS()
class DANCINGMANPLUGINS_API AMyActor : public AActor
{
    GENERATED_BODY()

    protected:
        virtual void BeginPlay() override;
	
    private:
        void OnPlayerKilledEnemy(const FGameplayTag Channel, const FMyProjectGameplayMessage_PlayerKilledEnemy& GameplayMessage);
};

// ---------------------------------------------------------------------- //

// MyActor.cpp

#include "MyActor.h"

#include "DanzmannGameplayMessagesSubsystem.h"
#include "GameplayMessages/MyProjectGameplayMessagesGameplayTags.h"

void AMyActor::BeginPlay()
{
    Super::BeginPlay();

    UDanzmannGameplayMessagesSubsystem* GameplayMessagesSubsystem = UDanzmannGameplayMessagesSubsystem::Get(GetWorld());
	
    // Listen for Gameplay Message -- lambda version
    GameplayMessagesSubsystem->RegisterListener<FMyProjectGameplayMessage_PlayerDeath>(
        MyProject::GameplayTags::GameplayMessages_PlayerDeath,
        []
        (const FGameplayTag Channel, const FMyProjectGameplayMessage_PlayerDeath& GameplayMessage)
        {
            // Do something...
        }
    );

    // Listen for Gameplay Message -- bind version
    GameplayMessagesSubsystem->RegisterListener(MyProject::GameplayTags::GameplayMessages_PlayerKilledEnemy, this, &ThisClass::OnPlayerKilledEnemy);

    // Broadcast Gameplay Message: PlayerDeath
    const FMyProjectGameplayMessage_PlayerDeath PlayerDeathPayload { this }; // Assign some appropriate value
    GameplayMessagesSubsystem->BroadcastGameplayMessage(MyProject::GameplayTags::GameplayMessages_PlayerDeath, PlayerDeathPayload);

    // Broadcast Gameplay Message: PlayerKilledEnemy
    FMyProjectGameplayMessage_PlayerKilledEnemy PlayerKilledEnemyPayload;
    PlayerKilledEnemyPayload.Victim = nullptr; // Assign some appropriate value
    PlayerKilledEnemyPayload.WeaponUsed = nullptr; // Assign some appropriate value
    GameplayMessagesSubsystem->BroadcastGameplayMessage(MyProject::GameplayTags::GameplayMessages_PlayerKilledEnemy, PlayerKilledEnemyPayload);
}
```

---

Based on the plugin named `GameplayMessageRouter`, which was developed by Epic Games and can be found in the Lyra Starter Game project.
