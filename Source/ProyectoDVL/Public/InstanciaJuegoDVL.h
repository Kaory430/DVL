#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "InstanciaJuegoDVL.generated.h"

class UMenuDVL;
class USalaEsperaDVL;
class UInputMappingContext;
class APersonajePolicia;
class APersonajeLadron;

USTRUCT(BlueprintType)
struct FPartidaLANDVL
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Nombre;

	UPROPERTY(BlueprintReadOnly)
	int32 Jugadores = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMenuDVLActualizado);

UCLASS(Config = Game)
class PROYECTODVL_API UInstanciaJuegoDVL : public UGameInstance
{
	GENERATED_BODY()

public:
	UInstanciaJuegoDVL();
	virtual void Init() override;
	virtual void Shutdown() override;

	UPROPERTY(EditDefaultsOnly, Config, Category = "DVL|Mapas")
	TSoftObjectPtr<UWorld> MapaMenu;

	UPROPERTY(EditDefaultsOnly, Config, Category = "DVL|Mapas")
	TSoftObjectPtr<UWorld> MapaJuego;

	UPROPERTY(EditDefaultsOnly, Config, Category = "DVL|Personajes")
	TSoftClassPtr<APersonajePolicia> ClasePolicia;

	UPROPERTY(EditDefaultsOnly, Config, Category = "DVL|Personajes")
	TSoftClassPtr<APersonajeLadron> ClaseLadron;

	UPROPERTY(EditDefaultsOnly, Config, Category = "DVL|Personajes")
	TSoftObjectPtr<UInputMappingContext> ContextoJuego;

	UPROPERTY(EditDefaultsOnly, Category = "DVL|Interfaz")
	TSubclassOf<UMenuDVL> ClaseMenu;

	UPROPERTY(EditDefaultsOnly, Category = "DVL|Interfaz")
	TSubclassOf<USalaEsperaDVL> ClaseSala;

	UPROPERTY(BlueprintReadOnly, Category = "DVL|Sesiones")
	TArray<FPartidaLANDVL> Partidas;

	UPROPERTY(BlueprintReadOnly, Category = "DVL|Sesiones")
	bool bOcupado = false;

	UPROPERTY(BlueprintReadOnly, Category = "DVL|Sesiones")
	FString Mensaje;

	UPROPERTY(BlueprintAssignable, Category = "DVL|Sesiones")
	FMenuDVLActualizado OnMenuActualizado;

	UFUNCTION(BlueprintCallable, Category = "DVL|Sesiones")
	void CrearPartida(const FString& Nombre);

	UFUNCTION(BlueprintCallable, Category = "DVL|Sesiones")
	void BuscarPartidas();

	UFUNCTION(BlueprintCallable, Category = "DVL|Sesiones")
	void UnirseAPartida(int32 Indice);

	UFUNCTION(BlueprintCallable, Category = "DVL|Sesiones")
	void VolverAlMenu();

	void MostrarMensaje(const FString& Texto);
	void CerrarSalaEIniciar();
	bool ValidarViaje(bool bASala, FString& Error) const;
	bool ViajarConSesion(bool bASala, FString& Error);
	bool ReabrirSala();
	int32 JugadoresEsperados = 0;

private:
	IOnlineSessionPtr Sesiones;
	TSharedPtr<FOnlineSessionSearch> Busqueda;
	TArray<FOnlineSessionSearchResult> Resultados;
	FDelegateHandle CreacionHandle, BusquedaHandle, UnionHandle, DestruccionHandle, ActualizacionHandle, InicioHandle;
	FDelegateHandle FinHandle;
	FDelegateHandle RedHandle, ViajeHandle, MapaHandle;
	bool bVolviendoAlMenu = false;

	bool PrepararSesiones();
	void LimpiarDelegados();
	void AbrirMenu();
	void RegresarTrasFallo(const FString& Error);
	void AlCrear(FName Nombre, bool bExito);
	void AlBuscar(bool bExito);
	void AlUnirse(FName Nombre, EOnJoinSessionCompleteResult::Type Resultado);
	void AlDestruir(FName Nombre, bool bExito);
	void AlActualizar(FName Nombre, bool bExito);
	void AlIniciar(FName Nombre, bool bExito);
	void AlTerminarSesion(FName Nombre, bool bExito);
	void AlReabrir(FName Nombre, bool bExito);
	void AlCargarMapa(UWorld* Mundo);
	void AlFallarRed(UWorld* Mundo, UNetDriver* Driver, ENetworkFailure::Type Tipo, const FString& Error);
	void AlFallarViaje(UWorld* Mundo, ETravelFailure::Type Tipo, const FString& Error);
};
