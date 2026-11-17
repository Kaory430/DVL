#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ModoMenuDVL.generated.h"

UCLASS()
class PROYECTODVL_API AModoMenuDVL : public AGameModeBase
{
	GENERATED_BODY()

public:
	AModoMenuDVL();

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
