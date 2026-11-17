// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Botin.generated.h"

class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class APersonajeLadron;

UENUM(BlueprintType)
enum class EEstadoBotin : uint8
{
	EnOrigen,
	Transportado,
	Caido
};

UCLASS()
class PROYECTODVL_API ABotin : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABotin();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Componentes")
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Componentes")
	USphereComponent* EsferaRecoleccion;

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Botin")
	FText NombreObjeto = NSLOCTEXT("Botin", "NombrePredeterminado", "Objeto robado");

	UPROPERTY(EditAnywhere, Replicated, BlueprintReadOnly, Category = "Botin", meta = (ClampMin = "50.0", ClampMax = "400.0", Units = "cm"))
	float AlcanceRobo = 200.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Estado, VisibleAnywhere, BlueprintReadOnly, Category = "Botin")
	EEstadoBotin Estado = EEstadoBotin::EnOrigen;

	bool EstaAlAlcance(const APersonajeLadron* Ladron) const;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnRecoleccionOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	friend class APersonajeLadron;

	bool Recoger(APersonajeLadron* Ladron);
	void Soltar(APersonajeLadron* Ladron, int32 Indice);
	void DevolverAlOrigen(bool bRecuperadoPorPolicia = false);
	void ComprobarPoliciaCercano();

	UFUNCTION()
	void OnRep_Estado();

	FTransform TransformacionOriginal;
	uint32 RevisionEstado = 0;

};
