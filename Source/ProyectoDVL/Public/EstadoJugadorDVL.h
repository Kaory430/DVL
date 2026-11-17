// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "EstadoJugadorDVL.generated.h"

UENUM(BlueprintType)
enum class EBandoDVL : uint8
{
	SinElegir,
	Policia,
	Ladron
};

UCLASS()
class PROYECTODVL_API AEstadoJugadorDVL : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sala")
	EBandoDVL Bando = EBandoDVL::SinElegir;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sala")
	bool bListo = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Botin")
	TArray<FText> ObjetosRobados;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	bool bEliminado = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	bool bQuiereRevancha = false;

	virtual void CopyProperties(APlayerState* NuevoEstado) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
