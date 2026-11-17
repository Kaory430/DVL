#include "InstanciaJuegoDVL.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "MenuDVL.h"
#include "ModoSalaDVL.h"
#include "OnlineSubsystemUtils.h"
#include "Misc/PackageName.h"
#include "HAL/IConsoleManager.h"
#include "PersonajePolicia.h"
#include "PersonajeLadron.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	const FName ClaveProyecto(TEXT("DVL_PROYECTO"));
	const FName ClaveNombre(TEXT("DVL_NOMBRE"));
	const FName ClaveAbierta(TEXT("DVL_ABIERTA"));
	const FString IdentificadorProyecto(TEXT("DVL_LAN_V1"));
}

UInstanciaJuegoDVL::UInstanciaJuegoDVL()
{
	MapaMenu = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Maps/MenuMap.MenuMap")));
	MapaJuego = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Maps/ThirdPersonMap.ThirdPersonMap")));
	ClasePolicia = TSoftClassPtr<APersonajePolicia>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Blueprints/BP_Policia.BP_Policia_C")));
	ClaseLadron = TSoftClassPtr<APersonajeLadron>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Blueprints/BP_Ladron.BP_Ladron_C")));
	ContextoJuego = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/ThirdPerson/Input/IMC_Default.IMC_Default")));
	ClaseMenu = UMenuDVL::StaticClass();
	ClaseSala = USalaEsperaDVL::StaticClass();
}

void UInstanciaJuegoDVL::Init()
{
	Super::Init();
	RedHandle = GEngine->OnNetworkFailure().AddUObject(this, &UInstanciaJuegoDVL::AlFallarRed);
	ViajeHandle = GEngine->OnTravelFailure().AddUObject(this, &UInstanciaJuegoDVL::AlFallarViaje);
	MapaHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UInstanciaJuegoDVL::AlCargarMapa);
}

void UInstanciaJuegoDVL::Shutdown()
{
	LimpiarDelegados();
	GEngine->OnNetworkFailure().Remove(RedHandle);
	GEngine->OnTravelFailure().Remove(ViajeHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(MapaHandle);
	Super::Shutdown();
}

bool UInstanciaJuegoDVL::PrepararSesiones()
{
	if (!Sesiones.IsValid())
	{
		Sesiones = Online::GetSessionInterface(GetWorld());
	}
	if (!Sesiones.IsValid())
	{
		MostrarMensaje(TEXT("No esta disponible OnlineSubsystemNull. Revisa los plugins del proyecto."));
		return false;
	}
	return true;
}

void UInstanciaJuegoDVL::MostrarMensaje(const FString& Texto)
{
	Mensaje = Texto;
	UE_LOG(LogTemp, Display, TEXT("DVL: %s"), *Texto);
	OnMenuActualizado.Broadcast();
}

void UInstanciaJuegoDVL::LimpiarDelegados()
{
	if (Sesiones.IsValid())
	{
		Sesiones->ClearOnCreateSessionCompleteDelegate_Handle(CreacionHandle);
		Sesiones->ClearOnFindSessionsCompleteDelegate_Handle(BusquedaHandle);
		Sesiones->ClearOnJoinSessionCompleteDelegate_Handle(UnionHandle);
		Sesiones->ClearOnDestroySessionCompleteDelegate_Handle(DestruccionHandle);
		Sesiones->ClearOnUpdateSessionCompleteDelegate_Handle(ActualizacionHandle);
		Sesiones->ClearOnStartSessionCompleteDelegate_Handle(InicioHandle);
		Sesiones->ClearOnEndSessionCompleteDelegate_Handle(FinHandle);
	}
}

void UInstanciaJuegoDVL::CrearPartida(const FString& Nombre)
{
	if (bOcupado || !PrepararSesiones())
	{
		return;
	}
	if (Nombre.TrimStartAndEnd().IsEmpty() || Nombre.Len() > 40)
	{
		MostrarMensaje(TEXT("Escribe un nombre de sala de entre 1 y 40 caracteres."));
		return;
	}
	if (Sesiones->GetNamedSession(NAME_GameSession))
	{
		MostrarMensaje(TEXT("Todavia hay una sesion abierta. Vuelve al menu antes de crear otra."));
		return;
	}
	if (!FPackageName::DoesPackageExist(MapaMenu.ToSoftObjectPath().GetLongPackageName()))
	{
		MostrarMensaje(TEXT("MapaMenu no existe. Corrige el mapa en InstanciaJuegoDVL antes de crear la sala."));
		return;
	}
	FOnlineSessionSettings Ajustes;
	Ajustes.bIsLANMatch = true;
	Ajustes.NumPublicConnections = 5;
	Ajustes.bShouldAdvertise = true;
	Ajustes.bAllowJoinInProgress = true;
	Ajustes.Set(ClaveProyecto, IdentificadorProyecto, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Ajustes.Set(ClaveNombre, Nombre.TrimStartAndEnd(), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	Ajustes.Set(ClaveAbierta, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	bOcupado = true;
	MostrarMensaje(TEXT("Creando sala LAN..."));
	CreacionHandle = Sesiones->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlCrear));
	if (!Sesiones->CreateSession(0, NAME_GameSession, Ajustes) && CreacionHandle.IsValid())
	{
		AlCrear(NAME_GameSession, false);
	}
}

void UInstanciaJuegoDVL::AlCrear(FName Nombre, bool bExito)
{
	Sesiones->ClearOnCreateSessionCompleteDelegate_Handle(CreacionHandle);
	if (!bExito)
	{
		bOcupado = false;
		MostrarMensaje(TEXT("No se pudo crear la sala LAN."));
		return;
	}
	MostrarMensaje(TEXT("Sala creada."));
	UGameplayStatics::OpenLevel(this, FName(*MapaMenu.ToSoftObjectPath().GetLongPackageName()), true,
		TEXT("listen?game=/Script/ProyectoDVL.ModoSalaDVL"));
}

void UInstanciaJuegoDVL::BuscarPartidas()
{
	if (bOcupado || !PrepararSesiones())
	{
		return;
	}
	Partidas.Reset();
	Resultados.Reset();
	Busqueda = MakeShared<FOnlineSessionSearch>();
	Busqueda->bIsLanQuery = true;
	Busqueda->MaxSearchResults = 100;
	bOcupado = true;
	MostrarMensaje(TEXT("Buscando salas en la red local..."));
	BusquedaHandle = Sesiones->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlBuscar));
	if (!Sesiones->FindSessions(0, Busqueda.ToSharedRef()) && BusquedaHandle.IsValid())
	{
		AlBuscar(false);
	}
}

void UInstanciaJuegoDVL::AlBuscar(bool bExito)
{
	Sesiones->ClearOnFindSessionsCompleteDelegate_Handle(BusquedaHandle);
	bOcupado = false;
	if (bExito && Busqueda.IsValid())
	{
		for (const FOnlineSessionSearchResult& Resultado : Busqueda->SearchResults)
		{
			FString Proyecto;
			bool bAbierta = false;
			Resultado.Session.SessionSettings.Get(ClaveProyecto, Proyecto);
			Resultado.Session.SessionSettings.Get(ClaveAbierta, bAbierta);
			if (Proyecto == IdentificadorProyecto && bAbierta && Resultado.Session.NumOpenPublicConnections > 0)
			{
				Resultados.Add(Resultado);
				FPartidaLANDVL& Partida = Partidas.AddDefaulted_GetRef();
				Resultado.Session.SessionSettings.Get(ClaveNombre, Partida.Nombre);
				Partida.Jugadores = Resultado.Session.SessionSettings.NumPublicConnections - Resultado.Session.NumOpenPublicConnections;
			}
		}
	}
	MostrarMensaje(!bExito ? TEXT("Fallo la busqueda LAN.") :
		Partidas.IsEmpty() ? TEXT("No se encontraron salas abiertas en esta red.") : TEXT("Selecciona una sala para unirte."));
}

void UInstanciaJuegoDVL::UnirseAPartida(int32 Indice)
{
	if (bOcupado || !PrepararSesiones())
	{
		return;
	}
	if (!Resultados.IsValidIndex(Indice))
	{
		MostrarMensaje(TEXT("La sala seleccionada ya no esta disponible. Busca de nuevo."));
		return;
	}
	bOcupado = true;
	MostrarMensaje(TEXT("Conectando..."));
	UnionHandle = Sesiones->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlUnirse));
	if (!Sesiones->JoinSession(0, NAME_GameSession, Resultados[Indice]) && UnionHandle.IsValid())
	{
		AlUnirse(NAME_GameSession, EOnJoinSessionCompleteResult::UnknownError);
	}
}

void UInstanciaJuegoDVL::AlUnirse(FName Nombre, EOnJoinSessionCompleteResult::Type Resultado)
{
	Sesiones->ClearOnJoinSessionCompleteDelegate_Handle(UnionHandle);
	if (Resultado != EOnJoinSessionCompleteResult::Success)
	{
		bOcupado = false;
		MostrarMensaje(TEXT("No se pudo entrar: la sala esta llena, cerro o ya no existe."));
		return;
	}
	FString Direccion;
	APlayerController* Controlador = GetFirstLocalPlayerController();
	if (!Sesiones->GetResolvedConnectString(NAME_GameSession, Direccion) || !Controlador)
	{
		RegresarTrasFallo(TEXT("No se pudo resolver la direccion de la sala."));
		return;
	}
	Controlador->ClientTravel(Direccion, TRAVEL_Absolute);
}

void UInstanciaJuegoDVL::CerrarSalaEIniciar()
{
	if (bOcupado || !PrepararSesiones())
	{
		return;
	}
	FOnlineSessionSettings* Ajustes = Sesiones->GetSessionSettings(NAME_GameSession);
	if (!Ajustes)
	{
		RegresarTrasFallo(TEXT("No existe la sesion del anfitrion."));
		return;
	}
	FOnlineSessionSettings Cerrados = *Ajustes;
	Cerrados.bShouldAdvertise = false;
	Cerrados.bAllowJoinInProgress = false;
	Cerrados.Set(ClaveAbierta, false, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	bOcupado = true;
	ActualizacionHandle = Sesiones->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlActualizar));
	if (!Sesiones->UpdateSession(NAME_GameSession, Cerrados, true) && ActualizacionHandle.IsValid())
	{
		AlActualizar(NAME_GameSession, false);
	}
}

void UInstanciaJuegoDVL::AlActualizar(FName Nombre, bool bExito)
{
	Sesiones->ClearOnUpdateSessionCompleteDelegate_Handle(ActualizacionHandle);
	if (!bExito)
	{
		RegresarTrasFallo(TEXT("No se pudo cerrar el acceso a la sala. Crea una partida nueva."));
		return;
	}
	InicioHandle = Sesiones->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlIniciar));
	if (!Sesiones->StartSession(NAME_GameSession) && InicioHandle.IsValid())
	{
		AlIniciar(NAME_GameSession, false);
	}
}

void UInstanciaJuegoDVL::AlIniciar(FName Nombre, bool bExito)
{
	Sesiones->ClearOnStartSessionCompleteDelegate_Handle(InicioHandle);
	bOcupado = false;
	AModoSalaDVL* Sala = GetWorld()->GetAuthGameMode<AModoSalaDVL>();
	FString Error;
	if (!bExito || !Sala || !Sala->IniciarViaje(Error))
	{
		RegresarTrasFallo(Error.IsEmpty() ? TEXT("No se pudo iniciar la partida.") : Error);
	}
}

void UInstanciaJuegoDVL::VolverAlMenu()
{
	if (!bOcupado)
	{
		RegresarTrasFallo(TEXT("Has salido de la sala."));
	}
}

bool UInstanciaJuegoDVL::ValidarViaje(bool bASala, FString& Error) const
{
	const FString Paquete = (bASala ? MapaMenu : MapaJuego).ToSoftObjectPath().GetLongPackageName();
	if (!GetWorld() || !GetWorld()->GetAuthGameMode() || !FPackageName::DoesPackageExist(Paquete))
	{
		Error = TEXT("No se puede viajar: falta autoridad o el mapa configurado no existe.");
		return false;
	}
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		const IConsoleVariable* Permitir = IConsoleManager::Get().FindConsoleVariable(TEXT("net.AllowPIESeamlessTravel"));
		if (!Permitir || Permitir->GetInt() == 0)
		{
			Error = TEXT("El viaje continuo en PIE esta desactivado. Reinicia Unreal para cargar net.AllowPIESeamlessTravel=1 o prueba en Standalone.");
			return false;
		}
	}
	return true;
}

bool UInstanciaJuegoDVL::ViajarConSesion(bool bASala, FString& Error)
{
	if (!ValidarViaje(bASala, Error))
	{
		return false;
	}
	const FString Paquete = (bASala ? MapaMenu : MapaJuego).ToSoftObjectPath().GetLongPackageName();
	const FString Destino = Paquete + (bASala ? TEXT("?game=/Script/ProyectoDVL.ModoSalaDVL") : TEXT("?game=/Script/ProyectoDVL.ModoJuegoDVL"));
	if (!GetWorld()->ServerTravel(Destino))
	{
		Error = TEXT("No se pudo iniciar el viaje. La conexion sigue abierta; vuelve a intentarlo.");
		return false;
	}
	return true;
}

bool UInstanciaJuegoDVL::ReabrirSala()
{
	FString Error;
	if (bOcupado || !ValidarViaje(true, Error) || !PrepararSesiones())
	{
		MostrarMensaje(Error.IsEmpty() ? TEXT("La sesion esta ocupada o no esta disponible.") : Error);
		return false;
	}
	if (!Sesiones->GetNamedSession(NAME_GameSession))
	{
		MostrarMensaje(TEXT("No existe la sesion que se debe reabrir."));
		return false;
	}
	bOcupado = true;
	MostrarMensaje(TEXT("Regresando todos a la sala..."));
	FinHandle = Sesiones->AddOnEndSessionCompleteDelegate_Handle(
		FOnEndSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlTerminarSesion));
	if (!Sesiones->EndSession(NAME_GameSession) && FinHandle.IsValid())
	{
		AlTerminarSesion(NAME_GameSession, false);
	}
	return true;
}

void UInstanciaJuegoDVL::AlTerminarSesion(FName Nombre, bool bExito)
{
	Sesiones->ClearOnEndSessionCompleteDelegate_Handle(FinHandle);
	const FOnlineSessionSettings* Anteriores = Sesiones->GetSessionSettings(NAME_GameSession);
	if (!bExito || !Anteriores)
	{
		RegresarTrasFallo(TEXT("No se pudo terminar la ronda para reabrir la sala."));
		return;
	}
	FOnlineSessionSettings Abiertos = *Anteriores;
	Abiertos.bShouldAdvertise = true;
	Abiertos.bAllowJoinInProgress = true;
	Abiertos.Set(ClaveAbierta, true, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	ActualizacionHandle = Sesiones->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlReabrir));
	if (!Sesiones->UpdateSession(NAME_GameSession, Abiertos, true) && ActualizacionHandle.IsValid())
	{
		AlReabrir(NAME_GameSession, false);
	}
}

void UInstanciaJuegoDVL::AlReabrir(FName Nombre, bool bExito)
{
	Sesiones->ClearOnUpdateSessionCompleteDelegate_Handle(ActualizacionHandle);
	bOcupado = false;
	FString Error;
	if (!bExito || !ViajarConSesion(true, Error))
	{
		RegresarTrasFallo(Error.IsEmpty() ? TEXT("No se pudo volver a anunciar la sala LAN.") : Error);
	}
}

void UInstanciaJuegoDVL::RegresarTrasFallo(const FString& Error)
{
	if (bVolviendoAlMenu)
	{
		return;
	}
	bVolviendoAlMenu = true;
	LimpiarDelegados();
	bOcupado = true;
	MostrarMensaje(Error);
	if (PrepararSesiones() && Sesiones->GetNamedSession(NAME_GameSession))
	{
		DestruccionHandle = Sesiones->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UInstanciaJuegoDVL::AlDestruir));
		if (!Sesiones->DestroySession(NAME_GameSession) && DestruccionHandle.IsValid())
		{
			AlDestruir(NAME_GameSession, false);
		}
	}
	else
	{
		AbrirMenu();
	}
}

void UInstanciaJuegoDVL::AlDestruir(FName Nombre, bool bExito)
{
	Sesiones->ClearOnDestroySessionCompleteDelegate_Handle(DestruccionHandle);
	if (!bExito)
	{
		MostrarMensaje(TEXT("No se pudo limpiar la sesion. Reinicia el juego antes de volver a conectarte."));
	}
	AbrirMenu();
}

void UInstanciaJuegoDVL::AbrirMenu()
{
	Partidas.Reset();
	Resultados.Reset();
	if (!FPackageName::DoesPackageExist(MapaMenu.ToSoftObjectPath().GetLongPackageName()))
	{
		bOcupado = false;
		bVolviendoAlMenu = false;
		MostrarMensaje(TEXT("No existe MapaMenu. Corrige la configuracion y reinicia el juego."));
		return;
	}
	UGameplayStatics::OpenLevel(this, FName(*MapaMenu.ToSoftObjectPath().GetLongPackageName()), true,
		TEXT("game=/Script/ProyectoDVL.ModoMenuDVL"));
}

void UInstanciaJuegoDVL::AlCargarMapa(UWorld* Mundo)
{
	if (Mundo && Mundo->GetGameInstance() == this)
	{
		bOcupado = false;
		bVolviendoAlMenu = false;
		OnMenuActualizado.Broadcast();
	}
}

void UInstanciaJuegoDVL::AlFallarRed(UWorld* Mundo, UNetDriver* Driver, ENetworkFailure::Type Tipo, const FString& Error)
{
	if (Mundo == GetWorld())
	{
		RegresarTrasFallo(FString::Printf(TEXT("Se perdio la conexion con la sala: %s"), *Error));
	}
}

void UInstanciaJuegoDVL::AlFallarViaje(UWorld* Mundo, ETravelFailure::Type Tipo, const FString& Error)
{
	if (Mundo == GetWorld())
	{
		RegresarTrasFallo(FString::Printf(TEXT("No se pudo cargar el mapa: %s"), *Error));
	}
}
