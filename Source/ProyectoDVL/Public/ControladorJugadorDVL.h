// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EstadoJugadorDVL.h"
#include "ControladorJugadorDVL.generated.h"

class UUserWidget;

struct FAvisoPartidaDVL
{
	FText Mensaje;
	double Caduca = 0.0;
};

UCLASS()
class PROYECTODVL_API AControladorJugadorDVL : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Sala")
	void ElegirBando(EBandoDVL Bando);

	UFUNCTION(BlueprintCallable, Category = "Sala")
	void MarcarListo(bool bListo);

	UFUNCTION(BlueprintCallable, Category = "Sala")
	void IniciarPartida();

	UFUNCTION(BlueprintCallable, Category = "Sala")
	void SalirAlMenu();

	UFUNCTION(BlueprintCallable, Category = "Partida")
	void VotarRevancha(bool bAceptar);

	UFUNCTION(BlueprintCallable, Category = "Partida")
	void VolverASala();

	UFUNCTION(BlueprintCallable, Category = "Partida")
	void AlternarMenuPartida();

	UPROPERTY(BlueprintReadOnly, Category = "Botin")
	bool bInventarioAbierto = false;

	UPROPERTY(BlueprintReadOnly, Category = "Partida")
	bool bMenuPartidaAbierto = false;

	UFUNCTION(Client, Reliable)
	void Client_MensajeSala(const FString& Mensaje);

	UFUNCTION(Client, Reliable)
	void Client_AvisoPartida(const FText& Mensaje);

	FText ObtenerAvisosPartida() const;

protected:
	virtual void PlayerTick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION(Server, Reliable)
	void Server_ElegirBando(EBandoDVL Bando);

	UFUNCTION(Server, Reliable)
	void Server_MarcarListo(bool bListo);

	UFUNCTION(Server, Reliable)
	void Server_IniciarPartida();

	UFUNCTION(Server, Reliable)
	void Server_VotarRevancha(bool bAceptar);

	UFUNCTION(Server, Reliable)
	void Server_VolverASala();

	UFUNCTION(Server, Reliable)
	void Server_ActivarPowerup();

	void ActivarPowerup();

	void AlternarInventario();
	void LimpiarInterfaz();
	void ActualizarEntradaPartida();

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> Interfaz;

	int32 TipoInterfaz = -1;
	TWeakObjectPtr<UWorld> MundoInterfaz;
	bool bEntradaInterfaz = false;
	bool bMovimientoBloqueado = false;
	TArray<FAvisoPartidaDVL> AvisosPartida;
};
