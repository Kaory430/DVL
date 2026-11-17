#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MenuDVL.generated.h"

class UButton;
class UEditableTextBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UBorder;

UCLASS()
class PROYECTODVL_API UEntradaPartidaDVL : public UUserWidget
{
	GENERATED_BODY()

public:
	void Configurar(int32 Indice, const FString& Nombre, int32 Jugadores);

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void Unirse();

	int32 IndicePartida = INDEX_NONE;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Texto;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoCupos;
};

UCLASS()
class PROYECTODVL_API UMenuDVL : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> NombreSala;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonCrear;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonBuscar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonSalir;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> ListaPartidas;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextoEstado;

private:
	UFUNCTION()
	void MostrarPrincipal();

	UFUNCTION()
	void MostrarCrear();

	UFUNCTION()
	void MostrarBuscar();

	UFUNCTION()
	void Crear();

	UFUNCTION()
	void Buscar();

	UFUNCTION()
	void Salir();

	UFUNCTION()
	void Actualizar();

	UPROPERTY(Transient)
	TObjectPtr<UWidgetSwitcher> Paginas;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoResultados;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoListaVacia;
};

UCLASS()
class PROYECTODVL_API USalaEsperaDVL : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonPolicia;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonLadron;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonListo;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonIniciar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BotonVolver;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextoJugadores;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TextoEstado;

private:
	UFUNCTION()
	void ElegirPolicia();

	UFUNCTION()
	void ElegirLadron();

	UFUNCTION()
	void AlternarListo();

	UFUNCTION()
	void Iniciar();

	UFUNCTION()
	void Volver();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ListaJugadoresNativa;

	FString UltimaListaJugadores;
	float TiempoActualizacion = 0.0f;
};

UCLASS()
class PROYECTODVL_API UPartidaDVL : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void Aceptar();
	UFUNCTION()
	void Rechazar();
	UFUNCTION()
	void Regresar();
	UFUNCTION()
	void Continuar();
	UFUNCTION()
	void Salir();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoReloj;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoArtefactos;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelAvisos;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoAvisos;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoPowerup;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> PanelOpciones;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoResultado;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoDetalle;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TextoVotos;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BotonAceptar;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BotonRechazar;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BotonRegresar;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BotonContinuar;
	UPROPERTY(Transient)
	TObjectPtr<UButton> BotonSalir;

	float TiempoActualizacion = 0.0f;
};
