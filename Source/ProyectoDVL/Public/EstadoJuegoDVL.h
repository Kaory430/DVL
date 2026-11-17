// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "EstadoJugadorDVL.h"
#include "EstadoJuegoDVL.generated.h"

UENUM(BlueprintType)
enum class EFasePartidaDVL : uint8
{
	Esperando,
	EnCurso,
	Finalizada,
	Viajando
};

/**
 * 
 */
UCLASS()
class PROYECTODVL_API AEstadoJuegoDVL : public AGameStateBase
{
	GENERATED_BODY()

public:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	bool bEsPartida = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	EFasePartidaDVL Fase = EFasePartidaDVL::Esperando;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	double FinPartida = 0.0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	int32 ArtefactosRobados = 0;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	int32 CapacidadInventario = 3;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	EBandoDVL BandoGanador = EBandoDVL::SinElegir;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Partida")
	FString MotivoResultado;

	UFUNCTION(BlueprintPure, Category = "Partida")
	int32 SegundosRestantes() const;

	bool PermiteAcciones() const;
	bool ComenzarRonda(FString& Error);
	EBandoDVL CalcularGanador(int32 ObjetosEnMochilas, int32 LadronesVivos) const;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sala")
	bool bEsSala = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sala")
	bool bIniciando = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Sala")
	AEstadoJugadorDVL* Anfitrion = nullptr;

	UFUNCTION(BlueprintPure, Category = "Sala")
	int32 ContarBando(EBandoDVL Bando) const;

	bool PuedeElegirBando(const AEstadoJugadorDVL* Jugador, EBandoDVL Bando, FString& Error) const;
	bool PuedeIniciar(FString& Error) const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
