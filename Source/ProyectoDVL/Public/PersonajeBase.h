// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PersonajeBase.generated.h"

class UHealthComponent;
class UIndicadorInteraccion;
class UPowerupComponent;

UCLASS()
class PROYECTODVL_API APersonajeBase : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	APersonajeBase();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	UHealthComponent* ComponenteSalud;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UPowerupComponent> ComponentePowerup;

	bool PuedeActuarEnPartida() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void UnPossessed() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UIndicadorInteraccion> IndicadorInteraccion;
};
