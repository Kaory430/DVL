#pragma once

#include "CoreMinimal.h"
#include "ModoMenuDVL.h"
#include "EstadoJugadorDVL.h"
#include "ModoSalaDVL.generated.h"

class AControladorJugadorDVL;

UCLASS()
class PROYECTODVL_API AModoSalaDVL : public AModoMenuDVL
{
	GENERATED_BODY()

public:
	AModoSalaDVL();
	virtual void InitGameState() override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void ElegirBando(AControladorJugadorDVL* Jugador, EBandoDVL Bando);
	void CambiarListo(AControladorJugadorDVL* Jugador, bool bListo);
	void SolicitarInicio(AControladorJugadorDVL* Jugador);
	bool IniciarViaje(FString& Error);
};
