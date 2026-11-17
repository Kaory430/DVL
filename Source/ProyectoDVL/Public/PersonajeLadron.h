// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonajeBase.h"
#include "TimerManager.h"
#include "PersonajeLadron.generated.h"

class ACelda;
class ABotin;

UCLASS()
class PROYECTODVL_API APersonajeLadron : public APersonajeBase
{
	GENERATED_BODY()

public:
	APersonajeLadron();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	int32 BotinRecolectado = 0;

	void AñadirBotin();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	TArray<TObjectPtr<ABotin>> Inventario;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	bool bRobando = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	float ProgresoRobo = 0.0f;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	TObjectPtr<ABotin> ObjetivoRobo;

	UFUNCTION(BlueprintPure, Category = "Botin")
	bool PuedeRobar() const;

	UFUNCTION(BlueprintPure, Category = "Botin")
	int32 ObtenerCapacidadInventario() const;

	UFUNCTION(BlueprintPure, Category = "Botin")
	ABotin* BuscarBotinCercano() const;

	UFUNCTION(BlueprintCallable, Category = "Interaccion")
	void IniciarInteraccion();

	UFUNCTION(BlueprintCallable, Category = "Interaccion")
	void CancelarInteraccion();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Vidas")
	int32 VidasRestantes = 3;

	UPROPERTY(ReplicatedUsing = OnRep_CeldaActual, VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	ACelda* CeldaActual = nullptr;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	bool bRescatando = false;

	UFUNCTION(BlueprintPure, Category = "Celda")
	ACelda* BuscarCeldaParaRescatar() const;

	UPROPERTY(ReplicatedUsing = OnRep_Derribado, VisibleAnywhere, BlueprintReadOnly, Category = "Reanimacion")
	bool bDerribado = false;

	UPROPERTY(ReplicatedUsing = OnRep_SiendoCargado, VisibleAnywhere, BlueprintReadOnly, Category = "Reanimacion")
	bool bSiendoCargado = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Reanimacion")
	bool bReanimando = false;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Reanimacion")
	float ProgresoReanimacion = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Reanimacion")
	APersonajeLadron* BuscarCompaneroReanimable() const;

	UFUNCTION(BlueprintCallable, Category = "Reanimacion")
	void IniciarReanimacion();

	UFUNCTION(BlueprintCallable, Category = "Reanimacion")
	void CancelarReanimacion();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Reanimacion")
	void EstablecerSiendoCargado(bool bCargado);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.0f) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UnPossessed() override;

private:
	friend class ACelda;
	friend class ABotin;

	void ComprobarRobo();
	void CancelarRoboServidor();
	void SoltarInventario();
	void ActualizarResumenInventario();
	void RecuperarSaludYMovimiento();

	UFUNCTION()
	void OnRep_CeldaActual();

	UFUNCTION(Client, Reliable)
	void Client_NotificarErrorInteraccion(const FString& Mensaje);

	UFUNCTION()
	void AlAgotarseSalud();

	UFUNCTION()
	void OnRep_Derribado();

	UFUNCTION()
	void OnRep_SiendoCargado();

	UFUNCTION(Server, Reliable)
	void Server_IniciarReanimacion();

	UFUNCTION(Server, Reliable)
	void Server_CancelarReanimacion();

	void ComprobarReanimacion();
	void CancelarReanimacionServidor();
	bool PuedeReanimar(const APersonajeLadron* Objetivo) const;
	void Morir();

	FTimerHandle TemporizadorReanimacion;
	FTimerHandle TemporizadorRobo;
	double InicioRobo = 0.0;
	uint32 RevisionRobo = 0;
	TWeakObjectPtr<APersonajeLadron> ObjetivoReanimacion;
	TWeakObjectPtr<ACelda> ObjetivoCelda;
	double InicioReanimacion = 0.0;
};
