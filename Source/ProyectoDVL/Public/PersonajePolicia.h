// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonajeBase.h"
#include "TimerManager.h"
#include "PersonajePolicia.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class APersonajeLadron;

UCLASS()
class PROYECTODVL_API APersonajePolicia : public APersonajeBase
{
	GENERATED_BODY()

public:
	APersonajePolicia();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	UBoxComponent* ColisionPorra;

	UFUNCTION(BlueprintCallable, Category = "Combate")
	void Atacar();

	UFUNCTION(BlueprintCallable, Category = "Captura")
	void CargarOSoltarLadron();

	UFUNCTION(BlueprintPure, Category = "Captura")
	APersonajeLadron* BuscarLadronParaCargar() const;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Captura")
	APersonajeLadron* LadronCargado = nullptr;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(Server, Reliable)
	void Server_CargarOSoltarLadron();

	UFUNCTION(Client, Reliable)
	void Client_NotificarErrorCarga(const FString& Mensaje);

	UFUNCTION(Server, Reliable)
	void Server_RealizarAtaque();

	UFUNCTION()
	void OnPorraOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	bool BuscarPosicionParaSoltar(FVector& Posicion) const;
	void SoltarLadron(bool bFinDeJuego = false);

	UFUNCTION()
	void AlDestruirLadronCargado(AActor* ActorDestruido);

	void DesactivarColisionPorra();

	FTimerHandle TemporizadorAtaque;
};
