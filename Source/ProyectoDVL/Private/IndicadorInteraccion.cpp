#include "IndicadorInteraccion.h"

#include "Celda.h"
#include "Botin.h"
#include "ControladorJugadorDVL.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "GameFramework/PlayerState.h"
#include "PersonajeLadron.h"
#include "PersonajePolicia.h"

void UIndicadorInteraccion::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanel* Lienzo = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Lienzo;
	USizeBox* Tamano = WidgetTree->ConstructWidget<USizeBox>();
	Tamano->SetWidthOverride(440.0f);
	UCanvasPanelSlot* Posicion = Lienzo->AddChildToCanvas(Tamano);
	Posicion->SetAnchors(FAnchors(0.5f, 0.85f));
	Posicion->SetAlignment(FVector2D(0.5f, 0.5f));
	Posicion->SetPosition(FVector2D::ZeroVector);
	Posicion->SetAutoSize(true);

	Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
	Panel->SetPadding(FMargin(12.0f));
	Tamano->AddChild(Panel);
	UVerticalBox* Contenido = WidgetTree->ConstructWidget<UVerticalBox>();
	Panel->AddChild(Contenido);
	Texto = WidgetTree->ConstructWidget<UTextBlock>();
	Texto->SetJustification(ETextJustify::Center);
	Texto->SetAutoWrapText(true);
	FSlateFontInfo Fuente = Texto->GetFont();
	Fuente.Size = 18;
	Texto->SetFont(Fuente);
	Contenido->AddChildToVerticalBox(Texto);
	Barra = WidgetTree->ConstructWidget<UProgressBar>();
	Barra->SetFillColorAndOpacity(FLinearColor(0.1f, 0.8f, 0.3f));
	Contenido->AddChildToVerticalBox(Barra);
	Panel->SetVisibility(ESlateVisibility::Hidden);

	USizeBox* TamanoInventario = WidgetTree->ConstructWidget<USizeBox>();
	TamanoInventario->SetWidthOverride(330.0f);
	TamanoInventario->SetMaxDesiredHeight(560.0f);
	UCanvasPanelSlot* PosicionInventario = Lienzo->AddChildToCanvas(TamanoInventario);
	PosicionInventario->SetAnchors(FAnchors(1.0f, 0.0f));
	PosicionInventario->SetAlignment(FVector2D(1.0f, 0.0f));
	PosicionInventario->SetPosition(FVector2D(-24.0f, 24.0f));
	PosicionInventario->SetAutoSize(true);
	PanelInventario = WidgetTree->ConstructWidget<UBorder>();
	PanelInventario->SetBrushColor(FLinearColor(0.012f, 0.02f, 0.035f, 0.92f));
	PanelInventario->SetPadding(FMargin(18.0f));
	TamanoInventario->AddChild(PanelInventario);
	UScrollBox* Desplazamiento = WidgetTree->ConstructWidget<UScrollBox>();
	PanelInventario->AddChild(Desplazamiento);
	TextoInventario = WidgetTree->ConstructWidget<UTextBlock>();
	TextoInventario->SetAutoWrapText(true);
	FSlateFontInfo FuenteInventario = TextoInventario->GetFont();
	FuenteInventario.Size = 14;
	TextoInventario->SetFont(FuenteInventario);
	TextoInventario->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.78f, 0.4f)));
	Desplazamiento->AddChild(TextoInventario);
	PanelInventario->SetVisibility(ESlateVisibility::Collapsed);
}

void UIndicadorInteraccion::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TiempoActualizacion -= InDeltaTime;
	if (TiempoActualizacion > 0.0f)
	{
		return;
	}
	TiempoActualizacion = 0.1f;
	const AEstadoJuegoDVL* Partida = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	if (Partida && !Partida->PermiteAcciones())
	{
		Panel->SetVisibility(ESlateVisibility::Hidden);
		PanelInventario->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FString Mensaje;
	bool bMostrarBarra = false;
	float Progreso = 0.0f;
	if (const APersonajeLadron* Ladron = Cast<APersonajeLadron>(GetOwningPlayerPawn()))
	{
		FString Inventarios = TEXT("BOTIN DEL EQUIPO\n\n");
		Inventarios += FString::Printf(TEXT("TU MOCHILA  [%d/%d]\n"), Ladron->Inventario.Num(), Ladron->ObtenerCapacidadInventario());
		if (Ladron->Inventario.IsEmpty())
		{
			Inventarios += TEXT("  Vacia\n");
		}
		for (const ABotin* Objeto : Ladron->Inventario)
		{
			Inventarios += TEXT("  - ") + (IsValid(Objeto) ? Objeto->NombreObjeto.ToString() : TEXT("Sincronizando...")) + TEXT("\n");
		}
		Inventarios += TEXT("\n");
		if (const AEstadoJuegoDVL* Juego = GetWorld()->GetGameState<AEstadoJuegoDVL>())
		{
			for (const APlayerState* Jugador : Juego->PlayerArray)
			{
				const AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(Jugador);
				if (Estado && Estado != Ladron->GetPlayerState() && Estado->Bando == EBandoDVL::Ladron)
				{
					Inventarios += FString::Printf(TEXT("%s  [%d/%d]\n"), *Estado->GetPlayerName(), Estado->ObjetosRobados.Num(), Juego->CapacidadInventario);
					if (Estado->ObjetosRobados.IsEmpty())
					{
						Inventarios += TEXT("  Vacia\n");
					}
					for (const FText& Nombre : Estado->ObjetosRobados)
					{
						Inventarios += TEXT("  - ") + Nombre.ToString() + TEXT("\n");
					}
					Inventarios += TEXT("\n");
				}
			}
		}
		TextoInventario->SetText(FText::FromString(Inventarios));
		const AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer());
		PanelInventario->SetVisibility(Controlador && Controlador->bInventarioAbierto
			? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (IsValid(Ladron->CeldaActual))
		{
			Mensaje = TEXT("Estas encarcelado. Espera a que un companero te libere.");
		}
		else if (Ladron->bSiendoCargado)
		{
			Mensaje = TEXT("Un policia te esta transportando.");
		}
		else if (Ladron->bDerribado)
		{
			Mensaje = TEXT("Estas derribado. Espera a que un companero te reanime.");
		}
		else if (Ladron->bReanimando)
		{
			Progreso = Ladron->ProgresoReanimacion;
			bMostrarBarra = true;
			Mensaje = FString::Printf(TEXT("Manten E - %s: %d%%"),
				Ladron->bRescatando ? TEXT("Liberando") : TEXT("Reanimando"), FMath::FloorToInt(Progreso * 100.0f));
		}
		else if (Ladron->bRobando)
		{
			Progreso = Ladron->ProgresoRobo;
			bMostrarBarra = true;
			const FString Nombre = IsValid(Ladron->ObjetivoRobo) ? Ladron->ObjetivoRobo->NombreObjeto.ToString() : TEXT("objeto");
			Mensaje = FString::Printf(TEXT("Manten E - Robando %s: %d%%"), *Nombre, FMath::FloorToInt(Progreso * 100.0f));
		}
		else if (Ladron->BuscarCeldaParaRescatar())
		{
			Mensaje = TEXT("Manten E durante 10 segundos para liberar a tu companero.");
		}
		else if (Ladron->BuscarCompaneroReanimable())
		{
			Mensaje = TEXT("Manten E durante 10 segundos para reanimar.");
		}
		else if (const ABotin* Botin = Ladron->BuscarBotinCercano())
		{
			Mensaje = Ladron->PuedeRobar()
				? FString::Printf(TEXT("Manten E durante 10 segundos para robar: %s."), *Botin->NombreObjeto.ToString())
				: FString::Printf(TEXT("Mochila llena (%d/%d). No puedes llevar mas objetos."), Ladron->Inventario.Num(), Ladron->ObtenerCapacidadInventario());
		}
	}
	else
	{
		PanelInventario->SetVisibility(ESlateVisibility::Collapsed);
		const APersonajePolicia* Policia = Cast<APersonajePolicia>(GetOwningPlayerPawn());
		if (Policia)
		{
			if (IsValid(Policia->LadronCargado))
			{
				if (const ACelda* Celda = ACelda::BuscarEnZona(Policia))
				{
					Mensaje = IsValid(Celda->Prisionero)
						? TEXT("Celda ocupada. Busca otra celda o alejate para soltar al ladron.")
						: TEXT("Pulsa E para encarcelar al ladron.");
				}
				else
				{
					Mensaje = TEXT("Pulsa E para soltar al ladron.");
				}
			}
			else if (Policia->BuscarLadronParaCargar())
			{
				Mensaje = TEXT("Pulsa E para cargar al ladron derribado.");
			}
		}
	}

	Texto->SetText(FText::FromString(Mensaje));
	Barra->SetPercent(Progreso);
	Barra->SetVisibility(bMostrarBarra ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	Panel->SetVisibility(Mensaje.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
}
