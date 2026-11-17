// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

class AActor;
class AController;
class UDamageType;
class FLifetimeProperty;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSaludAgotada);


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROYECTODVL_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Salud")
	float SaludMaxima = 100.0f;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Salud")
	float SaludActual = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Salud")
	FSaludAgotada OnSaludAgotada;

	TWeakObjectPtr<AActor> UltimoCausanteDano;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	UFUNCTION()
	void ManejarDano(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
