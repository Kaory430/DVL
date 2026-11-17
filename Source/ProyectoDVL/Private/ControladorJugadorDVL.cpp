// Fill out your copyright notice in the Description page of Project Settings.


#include "ControladorJugadorDVL.h"

#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "EstadoJuegoDVL.h"
#include "InputMappingContext.h"
#include "InstanciaJuegoDVL.h"
#include "MenuDVL.h"
#include "ModoMenuDVL.h"
#include "ModoSalaDVL.h"
#include "ModoJuegoDVL.h"
#include "InputCoreTypes.h"
#include "PersonajeLadron.h"
#include "PowerupComponent.h"

void AControladorJugadorDVL::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AControladorJugadorDVL::AlternarInventario);
	InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AControladorJugadorDVL::AlternarMenuPartida);
	InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AControladorJugadorDVL::ActivarPowerup);
}

void AControladorJugadorDVL::ActivarPowerup()
{
	if (!bMenuPartidaAbierto)
	{
		Server_ActivarPowerup();
	}
}

void AControladorJugadorDVL::Server_ActivarPowerup_Implementation()
{
	if (APersonajeBase* Personaje = Cast<APersonajeBase>(GetPawn()))
	{
		Personaje->ComponentePowerup->Activar();
	}
}

void AControladorJugadorDVL::AlternarInventario()
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (Cast<APersonajeLadron>(GetPawn()) && (!Estado || Estado->PermiteAcciones()) && !bMenuPartidaAbierto)
	{
		bInventarioAbierto = !bInventarioAbierto;
	}
}

void AControladorJugadorDVL::AlternarMenuPartida()
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (Estado && Estado->bEsPartida && Estado->Fase != EFasePartidaDVL::Finalizada && Estado->Fase != EFasePartidaDVL::Viajando)
	{
		bMenuPartidaAbierto = !bMenuPartidaAbierto;
		bInventarioAbierto = false;
		if (bMenuPartidaAbierto)
		{
			if (APersonajeLadron* Ladron = Cast<APersonajeLadron>(GetPawn()))
			{
				Ladron->CancelarInteraccion();
			}
		}
		ActualizarEntradaPartida();
	}
}

void AControladorJugadorDVL::VotarRevancha(bool bAceptar)
{
	Server_VotarRevancha(bAceptar);
}

void AControladorJugadorDVL::VolverASala()
{
	Server_VolverASala();
}

void AControladorJugadorDVL::Server_VotarRevancha_Implementation(bool bAceptar)
{
	if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
	{
		Modo->VotarRevancha(this, bAceptar);
	}
}

void AControladorJugadorDVL::Server_VolverASala_Implementation()
{
	if (AModoJuegoDVL* Modo = GetWorld()->GetAuthGameMode<AModoJuegoDVL>())
	{
		Modo->SolicitarVolverASala(this);
	}
}

void AControladorJugadorDVL::ElegirBando(EBandoDVL Bando)
{
	Server_ElegirBando(Bando);
}

void AControladorJugadorDVL::MarcarListo(bool bListo)
{
	Server_MarcarListo(bListo);
}

void AControladorJugadorDVL::IniciarPartida()
{
	Server_IniciarPartida();
}

void AControladorJugadorDVL::SalirAlMenu()
{
	if (IsLocalController())
	{
		if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
		{
			Instancia->VolverAlMenu();
		}
	}
}

void AControladorJugadorDVL::Server_ElegirBando_Implementation(EBandoDVL Bando)
{
	if (AModoSalaDVL* Sala = GetWorld()->GetAuthGameMode<AModoSalaDVL>())
	{
		Sala->ElegirBando(this, Bando);
	}
}

void AControladorJugadorDVL::Server_MarcarListo_Implementation(bool bListo)
{
	if (AModoSalaDVL* Sala = GetWorld()->GetAuthGameMode<AModoSalaDVL>())
	{
		Sala->CambiarListo(this, bListo);
	}
}

void AControladorJugadorDVL::Server_IniciarPartida_Implementation()
{
	if (AModoSalaDVL* Sala = GetWorld()->GetAuthGameMode<AModoSalaDVL>())
	{
		Sala->SolicitarInicio(this);
	}
}

void AControladorJugadorDVL::Client_MensajeSala_Implementation(const FString& Mensaje)
{
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->MostrarMensaje(Mensaje);
	}
}

void AControladorJugadorDVL::Client_AvisoPartida_Implementation(const FText& Mensaje)
{
	if (Mensaje.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Se recibio un aviso de partida vacio."));
		return;
	}
	const double Ahora = GetWorld()->GetRealTimeSeconds();
	AvisosPartida.RemoveAll([Ahora](const FAvisoPartidaDVL& Aviso) { return Aviso.Caduca <= Ahora; });
	if (AvisosPartida.Num() >= 4)
	{
		AvisosPartida.RemoveAt(0);
	}
	AvisosPartida.Add({ Mensaje, Ahora + 7.0 });
}

FText AControladorJugadorDVL::ObtenerAvisosPartida() const
{
	const double Ahora = GetWorld()->GetRealTimeSeconds();
	FText Resultado;
	for (const FAvisoPartidaDVL& Aviso : AvisosPartida)
	{
		if (Aviso.Caduca > Ahora)
		{
			Resultado = Resultado.IsEmpty() ? Aviso.Mensaje
				: FText::Format(NSLOCTEXT("AvisosDVL", "Linea", "{0}\n\n{1}"), Resultado, Aviso.Mensaje);
		}
	}
	return Resultado;
}

void AControladorJugadorDVL::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	if (!IsLocalController())
	{
		return;
	}
	if (MundoInterfaz.Get() != GetWorld())
	{
		LimpiarInterfaz();
		MundoInterfaz = GetWorld();
	}
	UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (!Instancia || !Estado)
	{
		return;
	}
	const bool bMenu = GetWorld()->GetAuthGameMode<AModoMenuDVL>() && !Estado->bEsSala;
	const int32 NuevoTipo = Estado->bEsSala ? 2 : bMenu ? 1 : (Estado->bEsPartida || GetPawn()) ? 3 : 0;
	if (NuevoTipo == 3 && TipoInterfaz == 3)
	{
		ActualizarEntradaPartida();
	}
	if (NuevoTipo == 0 || NuevoTipo == TipoInterfaz)
	{
		return;
	}
	if (Interfaz)
	{
		Interfaz->RemoveFromParent();
		Interfaz = nullptr;
	}
	TipoInterfaz = NuevoTipo;
	if (NuevoTipo == 1 || NuevoTipo == 2)
	{
		TSubclassOf<UUserWidget> Clase = NuevoTipo == 1 ? TSubclassOf<UUserWidget>(Instancia->ClaseMenu)
			: TSubclassOf<UUserWidget>(Instancia->ClaseSala);
		Interfaz = CreateWidget<UUserWidget>(this, Clase);
		if (!ensureMsgf(Interfaz, TEXT("Configura ClaseMenu y ClaseSala en InstanciaJuegoDVL.")))
		{
			return;
		}
		Interfaz->AddToPlayerScreen();
		bShowMouseCursor = true;
		FInputModeUIOnly Modo;
		Modo.SetWidgetToFocus(Interfaz->TakeWidget());
		SetInputMode(Modo);
		if (NuevoTipo == 2)
		{
			Instancia->MostrarMensaje(TEXT("Elige tu bando y pulsa Listo. Solo el anfitrion inicia."));
		}
	}
	else
	{
		Interfaz = CreateWidget<UPartidaDVL>(this);
		if (ensure(Interfaz))
		{
			Interfaz->AddToPlayerScreen(10);
		}
		bShowMouseCursor = false;
		SetInputMode(FInputModeGameOnly());
		ActualizarEntradaPartida();
		if (ULocalPlayer* Local = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Entradas = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (UInputMappingContext* Contexto = Instancia->ContextoJuego.LoadSynchronous())
				{
					Entradas->AddMappingContext(Contexto, 0);
				}
				else
				{
					Instancia->MostrarMensaje(TEXT("No se pudo cargar IMC_Default. Revisa ContextoJuego."));
				}
			}
		}
	}
}

void AControladorJugadorDVL::ActualizarEntradaPartida()
{
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	const bool bFinal = Estado && Estado->bEsPartida && Estado->Fase == EFasePartidaDVL::Finalizada;
	const bool bUI = bMenuPartidaAbierto || bFinal;
	if (bUI != bEntradaInterfaz)
	{
		bEntradaInterfaz = bUI;
		bShowMouseCursor = bUI;
		if (bUI)
		{
			FInputModeGameAndUI Modo;
			Modo.SetHideCursorDuringCapture(false);
			SetInputMode(Modo);
			bInventarioAbierto = false;
		}
		else
		{
			SetInputMode(FInputModeGameOnly());
		}
	}
	const bool bBloquear = bUI || (Estado && Estado->bEsPartida && !Estado->PermiteAcciones());
	if (bMovimientoBloqueado != bBloquear)
	{
		bMovimientoBloqueado = bBloquear;
		SetIgnoreMoveInput(bBloquear);
		SetIgnoreLookInput(bBloquear);
	}
}

void AControladorJugadorDVL::LimpiarInterfaz()
{
	AvisosPartida.Reset();
	if (Interfaz)
	{
		Interfaz->RemoveFromParent();
		Interfaz = nullptr;
	}
	TipoInterfaz = -1;
	bInventarioAbierto = false;
	bMenuPartidaAbierto = false;
	bEntradaInterfaz = false;
	if (bMovimientoBloqueado)
	{
		SetIgnoreMoveInput(false);
		SetIgnoreLookInput(false);
		bMovimientoBloqueado = false;
	}
}

void AControladorJugadorDVL::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	LimpiarInterfaz();
	Super::EndPlay(EndPlayReason);
}
