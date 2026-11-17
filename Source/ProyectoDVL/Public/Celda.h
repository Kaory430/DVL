#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Celda.generated.h"

class APersonajeLadron;
class APersonajePolicia;
class UArrowComponent;
class UBoxComponent;

UCLASS()
class PROYECTODVL_API ACelda : public AActor
{
	GENERATED_BODY()

public:
	ACelda();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	UBoxComponent* ZonaInteraccion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	UArrowComponent* PuntoPrisionero;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	UArrowComponent* PuntoSalida;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Celda")
	APersonajeLadron* Prisionero = nullptr;

	bool EstaEnZona(const AActor* Interactor) const;
	bool PuedeRescatar(const APersonajeLadron* Rescatador) const;
	bool Encarcelar(APersonajePolicia* Policia, FString& Error);
	bool LiberarPrisionero(FString& Error);
	static ACelda* BuscarEnZona(const AActor* Interactor, bool bSoloOcupadas = false);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool BuscarPosicionSegura(const UArrowComponent* Punto, const APersonajeLadron* Ladron,
		const AActor* ActorIgnorado, FVector& Posicion) const;

	UFUNCTION()
	void AlDestruirPrisionero(AActor* ActorDestruido);
};
