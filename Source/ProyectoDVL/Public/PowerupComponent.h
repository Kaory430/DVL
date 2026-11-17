#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "PowerupComponent.generated.h"

class APersonajeBase;
class APersonajeLadron;
class UMaterialInterface;
class USkeletalMeshComponent;
enum class EVisibilityBasedAnimTickOption : uint8;

UENUM(BlueprintType)
enum class EPowerupDVL : uint8
{
	Ninguno,
	Pulso,
	Adrenalina
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROYECTODVL_API UPowerupComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPowerupComponent();

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Powerups")
	EPowerupDVL Carga = EPowerupDVL::Ninguno;

	UPROPERTY(ReplicatedUsing = OnRep_Efecto, VisibleAnywhere, BlueprintReadOnly, Category = "Powerups")
	EPowerupDVL Efecto = EPowerupDVL::Ninguno;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Powerups")
	double FinEfecto = 0.0;

	bool PuedeRecoger(EPowerupDVL Tipo) const;
	bool PuedeDetectar(const APersonajeLadron* Ladron) const;
	bool Recoger(EPowerupDVL Tipo);
	void Activar();
	void CancelarEfecto();
	float SegundosRestantes() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool PuedeUsar() const;
	void ComprobarEfecto();
	void ActualizarSiluetas();
	void LimpiarSiluetas();
	void RestaurarAnimacion(USkeletalMeshComponent* Origen);
	void AvisarPropietario(const FText& Mensaje) const;

	UFUNCTION()
	void OnRep_Efecto();

	UPROPERTY(Replicated)
	TArray<TObjectPtr<APersonajeLadron>> ObjetivosPulso;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> MaterialPulso;

	UPROPERTY(Transient)
	TMap<TObjectPtr<APersonajeLadron>, TObjectPtr<USkeletalMeshComponent>> Siluetas;

	TMap<TWeakObjectPtr<USkeletalMeshComponent>, EVisibilityBasedAnimTickOption> AnimacionesPrevias;
	FTimerHandle TemporizadorEfecto;
	float VelocidadAnterior = 0.0f;
	bool bVelocidadAplicada = false;
};
