// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EstadoJugadorDVL.h"
#include "TimerManager.h"
#include "ModoJuegoDVL.generated.h"

class ABotin;
class APersonajeLadron;
/**
 * 
 */
UCLASS()
class PROYECTODVL_API AModoJuegoDVL : public AGameModeBase
{
	GENERATED_BODY()

public:
	AModoJuegoDVL();
	virtual void InitGameState() override;
	virtual void Logout(AController* Exiting) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	void EvaluarPartida();
	void VotarRevancha(APlayerController* Jugador, bool bAceptar);
	void SolicitarVolverASala(APlayerController* Jugador);
	void AvisarRobo(const ABotin* Objeto);
	void AvisarDevolucion(const ABotin* Objeto);
	void AvisarDerribo(const APersonajeLadron* Ladron, bool bPorPolicia);
	void AvisarPulso();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Finalizar(EBandoDVL Ganador, const FString& Motivo);
	void VolverASala();
	void ReiniciarRonda();
	void PrepararJugadoresParaViaje(bool bBorrarBandos);
	int32 ContarArtefactosEnMochilas() const;
	void EnviarAviso(const FText& Mensaje, EBandoDVL Destinatarios);
	FTimerHandle TemporizadorPartida;
	double LimiteCarga = 0.0;
	bool bCerrando = false;
};
