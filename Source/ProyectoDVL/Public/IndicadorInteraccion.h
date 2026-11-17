#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IndicadorInteraccion.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;

UCLASS()
class PROYECTODVL_API UIndicadorInteraccion : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UBorder> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Texto;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> Barra;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelInventario;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoInventario;

	float TiempoActualizacion = 0.0f;
};
