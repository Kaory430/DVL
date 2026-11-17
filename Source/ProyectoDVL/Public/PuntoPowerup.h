#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerupComponent.h"
#include "PuntoPowerup.generated.h"

class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class PROYECTODVL_API APuntoPowerup : public AActor
{
	GENERATED_BODY()

public:
	APuntoPowerup();

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Disponibilidad, BlueprintReadOnly, Category = "Powerups")
	EPowerupDVL Tipo = EPowerupDVL::Pulso;

	UPROPERTY(ReplicatedUsing = OnRep_Disponibilidad, VisibleAnywhere, BlueprintReadOnly, Category = "Powerups")
	bool bDisponible = true;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Powerups")
	double ProximaReaparicion = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<USphereComponent> ZonaRecogida;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UTextRenderComponent> Texto;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Componentes")
	TObjectPtr<UPointLightComponent> Luz;

	bool IntentarRecoger(APersonajeBase* Personaje);
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void AlEntrar(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnRep_Disponibilidad();

	void Reaparecer();
	FTimerHandle TemporizadorReaparicion;
};
