#include "MenuDVL.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "ControladorJugadorDVL.h"
#include "Engine/World.h"
#include "EstadoJuegoDVL.h"
#include "InstanciaJuegoDVL.h"
#include "Kismet/KismetSystemLibrary.h"
#include "PersonajeBase.h"
#include "PowerupComponent.h"
#include "Styling/CoreStyle.h"

namespace
{
	const FLinearColor Fondo(0.008f, 0.012f, 0.022f);
	const FLinearColor Superficie(0.018f, 0.027f, 0.045f);
	const FLinearColor Elevado(0.035f, 0.052f, 0.083f);
	const FLinearColor Blanco(0.91f, 0.94f, 1.0f);
	const FLinearColor Gris(0.44f, 0.52f, 0.65f);
	const FLinearColor Azul(0.12f, 0.47f, 1.0f);
	const FLinearColor Ambar(1.0f, 0.55f, 0.12f);
	const FLinearColor Verde(0.25f, 0.85f, 0.55f);

	UTextBlock* CrearTexto(UWidgetTree* Arbol, const FString& Texto, int32 Tamano = 18,
		FLinearColor Color = Blanco, bool bNegrita = false)
	{
		UTextBlock* Etiqueta = Arbol->ConstructWidget<UTextBlock>();
		Etiqueta->SetText(FText::FromString(Texto));
		Etiqueta->SetFont(FCoreStyle::GetDefaultFontStyle(bNegrita ? "Bold" : "Regular", Tamano));
		Etiqueta->SetColorAndOpacity(FSlateColor(Color));
		Etiqueta->SetAutoWrapText(true);
		return Etiqueta;
	}

	UTextBlock* AgregarTexto(UWidgetTree* Arbol, UVerticalBox* Panel, const FString& Texto,
		int32 Tamano = 18, FLinearColor Color = Blanco, bool bNegrita = false)
	{
		UTextBlock* Etiqueta = CrearTexto(Arbol, Texto, Tamano, Color, bNegrita);
		Panel->AddChildToVerticalBox(Etiqueta)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		return Etiqueta;
	}

	void AgregarEspacio(UWidgetTree* Arbol, UVerticalBox* Panel, bool bFlexible = false)
	{
		USpacer* Espacio = Arbol->ConstructWidget<USpacer>();
		Espacio->SetSize(FVector2D(1.0f, 12.0f));
		UVerticalBoxSlot* Slot = Panel->AddChildToVerticalBox(Espacio);
		if (bFlexible)
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
	}

	UBorder* CrearTarjeta(UWidgetTree* Arbol, FLinearColor Color = Elevado, float Margen = 20.0f)
	{
		UBorder* Tarjeta = Arbol->ConstructWidget<UBorder>();
		Tarjeta->SetBrush(FSlateRoundedBoxBrush(Color, 14.0f));
		Tarjeta->SetPadding(FMargin(Margen));
		return Tarjeta;
	}

	void EstilizarBoton(UButton* Boton, FLinearColor Color = Elevado)
	{
		FButtonStyle Estilo;
		Estilo.SetNormal(FSlateRoundedBoxBrush(Color, 8.0f));
		Estilo.SetHovered(FSlateRoundedBoxBrush(FMath::Lerp(Color, Blanco, 0.16f), 8.0f));
		Estilo.SetPressed(FSlateRoundedBoxBrush(FMath::Lerp(Color, Fondo, 0.25f), 8.0f));
		Estilo.SetDisabled(FSlateRoundedBoxBrush(Superficie, 8.0f));
		Estilo.SetNormalPadding(FMargin(18.0f, 12.0f));
		Estilo.SetPressedPadding(FMargin(18.0f, 13.0f, 18.0f, 11.0f));
		Boton->SetStyle(Estilo);
		Boton->SetBackgroundColor(FLinearColor::White);
		Boton->SetColorAndOpacity(FLinearColor::White);
	}

	UButton* AgregarBoton(UWidgetTree* Arbol, UVerticalBox* Panel, const FString& Texto,
		FLinearColor Color = Elevado, FLinearColor ColorTexto = Blanco)
	{
		UButton* Boton = Arbol->ConstructWidget<UButton>();
		EstilizarBoton(Boton, Color);
		UTextBlock* Etiqueta = CrearTexto(Arbol, Texto, 18, ColorTexto, true);
		Etiqueta->SetJustification(ETextJustify::Center);
		Boton->AddChild(Etiqueta);
		Panel->AddChildToVerticalBox(Boton)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		return Boton;
	}

	void CambiarEtiqueta(UButton* Boton, const FString& Texto)
	{
		if (UTextBlock* Etiqueta = Cast<UTextBlock>(Boton->GetContent()))
		{
			Etiqueta->SetText(FText::FromString(Texto));
		}
	}

	UVerticalBox* CrearPanel(UWidgetTree* Arbol, const FString& Subtitulo)
	{
		UBorder* FondoPantalla = Arbol->ConstructWidget<UBorder>();
		FondoPantalla->SetBrushColor(Fondo);
		FondoPantalla->SetPadding(FMargin(20.0f));
		Arbol->RootWidget = FondoPantalla;
		UScaleBox* Escala = Arbol->ConstructWidget<UScaleBox>();
		Escala->SetStretch(EStretch::ScaleToFit);
		FondoPantalla->AddChild(Escala);
		USizeBox* Lienzo = Arbol->ConstructWidget<USizeBox>();
		Lienzo->SetWidthOverride(1440.0f);
		Lienzo->SetHeightOverride(900.0f);
		Escala->AddChild(Lienzo);
		UBorder* Margen = Arbol->ConstructWidget<UBorder>();
		Margen->SetBrushColor(FLinearColor::Transparent);
		Margen->SetPadding(FMargin(64.0f, 40.0f));
		Lienzo->AddChild(Margen);
		UHorizontalBox* Columnas = Arbol->ConstructWidget<UHorizontalBox>();
		Margen->AddChild(Columnas);

		UVerticalBox* Marca = Arbol->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* SlotMarca = Columnas->AddChildToHorizontalBox(Marca);
		SlotMarca->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SlotMarca->SetPadding(FMargin(0.0f, 24.0f, 64.0f, 24.0f));
		AgregarTexto(Arbol, Marca, TEXT("MULTIJUGADOR  /  RED LOCAL"), 14, Ambar, true);
		AgregarEspacio(Arbol, Marca, true);
		AgregarTexto(Arbol, Marca, TEXT("CHASE"), 76, Blanco, true);
		AgregarTexto(Arbol, Marca, TEXT("BY PIGS"), 76, Ambar, true);
		AgregarTexto(Arbol, Marca, TEXT("ATRAPA. ESCAPA. REPITE."), 19, Blanco, true);
		AgregarTexto(Arbol, Marca, Subtitulo, 19, Gris);
		AgregarEspacio(Arbol, Marca);

		UBorder* Bandos = CrearTarjeta(Arbol, Superficie);
		Marca->AddChildToVerticalBox(Bandos);
		UVerticalBox* Detalle = Arbol->ConstructWidget<UVerticalBox>();
		Bandos->AddChild(Detalle);
		AgregarTexto(Arbol, Detalle, TEXT("01  POLICIA"), 22, Azul, true);
		AgregarTexto(Arbol, Detalle, TEXT("Que nadie escape de tu guardia."), 16, Gris);
		AgregarTexto(Arbol, Detalle, TEXT("04  LADRONES"), 22, Ambar, true);
		AgregarTexto(Arbol, Detalle, TEXT("Roba, rescata y cuida de tu equipo."), 16, Gris);
		AgregarEspacio(Arbol, Marca, true);
		AgregarTexto(Arbol, Marca, TEXT("1 VS 4    /    HASTA 5 JUGADORES"), 13, Gris, true);

		UBorder* Tarjeta = CrearTarjeta(Arbol, Superficie, 32.0f);
		FSlateChildSize AnchoContenido(ESlateSizeRule::Fill);
		AnchoContenido.Value = 1.25f;
		Columnas->AddChildToHorizontalBox(Tarjeta)->SetSize(AnchoContenido);
		UVerticalBox* Contenido = Arbol->ConstructWidget<UVerticalBox>();
		Tarjeta->AddChild(Contenido);
		return Contenido;
	}

	UVerticalBox* CrearPagina(UWidgetTree* Arbol, UWidgetSwitcher* Paginas,
		const FString& Paso, const FString& Titulo, const FString& Descripcion)
	{
		UVerticalBox* Pagina = Arbol->ConstructWidget<UVerticalBox>();
		Paginas->AddChild(Pagina);
		AgregarTexto(Arbol, Pagina, Paso, 13, Ambar, true);
		AgregarTexto(Arbol, Pagina, Titulo, 34, Blanco, true);
		AgregarTexto(Arbol, Pagina, Descripcion, 18, Gris);
		AgregarEspacio(Arbol, Pagina);
		return Pagina;
	}

	void AgregarFilaJugador(UWidgetTree* Arbol, UVerticalBox* Lista, const AEstadoJugadorDVL* Jugador,
		const AEstadoJugadorDVL* Propio, const AEstadoJugadorDVL* Anfitrion)
	{
		UBorder* Tarjeta = CrearTarjeta(Arbol, Jugador == Propio ? FLinearColor(0.025f, 0.065f, 0.12f) : Elevado, 10.0f);
		Lista->AddChildToVerticalBox(Tarjeta)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		UHorizontalBox* Fila = Arbol->ConstructWidget<UHorizontalBox>();
		Tarjeta->AddChild(Fila);
		const FString Nombre = Jugador ? Jugador->GetPlayerName()
			+ (Jugador == Propio ? TEXT(" (Tu)") : TEXT(""))
			+ (Jugador == Anfitrion ? TEXT(" [Host]") : TEXT("")) : TEXT("Esperando jugador...");
		UTextBlock* EtiquetaNombre = CrearTexto(Arbol, Nombre, 14, Jugador ? Blanco : Gris);
		Fila->AddChildToHorizontalBox(EtiquetaNombre)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		const EBandoDVL Bando = Jugador ? Jugador->Bando : EBandoDVL::SinElegir;
		const FString Rol = !Jugador ? TEXT("--") : Bando == EBandoDVL::Policia ? TEXT("POLICIA")
			: Bando == EBandoDVL::Ladron ? TEXT("LADRON") : TEXT("ELIGIENDO");
		UTextBlock* EtiquetaRol = CrearTexto(Arbol, Rol, 12,
			Bando == EBandoDVL::Policia ? Azul : Bando == EBandoDVL::Ladron ? Ambar : Gris, true);
		Fila->AddChildToHorizontalBox(EtiquetaRol)->SetPadding(FMargin(10.0f, 0.0f));
		UTextBlock* EtiquetaEstado = CrearTexto(Arbol, Jugador && Jugador->bListo ? TEXT("LISTO") : TEXT("--"), 12,
			Jugador && Jugador->bListo ? Verde : Gris, true);
		Fila->AddChildToHorizontalBox(EtiquetaEstado);
	}
}

void UEntradaPartidaDVL::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UButton* Boton = WidgetTree->ConstructWidget<UButton>();
	EstilizarBoton(Boton);
	WidgetTree->RootWidget = Boton;
	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>();
	Boton->AddChild(Panel);
	Texto = AgregarTexto(WidgetTree, Panel, TEXT(""), 20, Blanco, true);
	TextoCupos = AgregarTexto(WidgetTree, Panel, TEXT(""), 14, Gris);
	Boton->OnClicked.AddDynamic(this, &UEntradaPartidaDVL::Unirse);
}

void UEntradaPartidaDVL::Configurar(int32 Indice, const FString& Nombre, int32 Jugadores)
{
	IndicePartida = Indice;
	Texto->SetText(FText::FromString(Nombre));
	TextoCupos->SetText(FText::FromString(FString::Printf(TEXT("%d / 5 JUGADORES    |    LAN    |    UNIRSE  >"), Jugadores)));
	SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
}

void UEntradaPartidaDVL::Unirse()
{
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->UnirseAPartida(IndicePartida);
	}
}

void UMenuDVL::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Panel = CrearPanel(WidgetTree, TEXT("Una ciudad. Dos bandos.\nLa proxima persecucion empieza contigo."));
		Paginas = WidgetTree->ConstructWidget<UWidgetSwitcher>();
		Panel->AddChildToVerticalBox(Paginas)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

		UVerticalBox* Portada = CrearPagina(WidgetTree, Paginas, TEXT("BIENVENIDO A LA PERSECUCION"),
			TEXT("No todos juegan\ndel mismo lado."),
			TEXT("Reune a tus amigos en la misma red.\nDecidan quien persigue y quien escapa."));
		AgregarEspacio(WidgetTree, Portada, true);
		AgregarBoton(WidgetTree, Portada, TEXT("ENTRAR AL JUEGO  >"), Ambar, Fondo)->OnClicked.AddDynamic(this, &UMenuDVL::MostrarPrincipal);
		AgregarBoton(WidgetTree, Portada, TEXT("Salir al escritorio"))->OnClicked.AddDynamic(this, &UMenuDVL::Salir);
		AgregarEspacio(WidgetTree, Portada);

		UVerticalBox* Principal = CrearPagina(WidgetTree, Paginas, TEXT("01  /  MENU PRINCIPAL"),
			TEXT("Arma la partida."),
			TEXT("Abre tu propia sala o entra a la de tus amigos."));
		AgregarEspacio(WidgetTree, Principal, true);
		AgregarBoton(WidgetTree, Principal, TEXT("CREAR SALA  >"), Ambar, Fondo)->OnClicked.AddDynamic(this, &UMenuDVL::MostrarCrear);
		AgregarBoton(WidgetTree, Principal, TEXT("BUSCAR SALAS LAN  >"), Azul)->OnClicked.AddDynamic(this, &UMenuDVL::MostrarBuscar);
		BotonSalir = AgregarBoton(WidgetTree, Principal, TEXT("Salir al escritorio"));
		AgregarEspacio(WidgetTree, Principal);

		UVerticalBox* CrearSala = CrearPagina(WidgetTree, Paginas, TEXT("02  /  SER ANFITRION"),
			TEXT("Tu sala. Tu equipo."),
			TEXT("Ponle nombre a la sala. Tus amigos la veran al buscar en la red local."));
		AgregarTexto(WidgetTree, CrearSala, TEXT("NOMBRE DE LA SALA"), 13, Gris, true);
		NombreSala = WidgetTree->ConstructWidget<UEditableTextBox>();
		NombreSala->SetHintText(FText::FromString(TEXT("Ej. La ultima persecucion")));
		FEditableTextBoxStyle EstiloNombre = NombreSala->GetWidgetStyle();
		EstiloNombre.SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20));
		EstiloNombre.SetForegroundColor(Blanco);
		EstiloNombre.SetBackgroundImageNormal(FSlateRoundedBoxBrush(Elevado, 8.0f));
		EstiloNombre.SetBackgroundImageHovered(FSlateRoundedBoxBrush(Elevado * 1.3f, 8.0f));
		EstiloNombre.SetBackgroundImageFocused(FSlateRoundedBoxBrush(Elevado, 8.0f, Azul, 1.0f));
		EstiloNombre.SetPadding(FMargin(18.0f, 16.0f));
		NombreSala->SetWidgetStyle(EstiloNombre);
		CrearSala->AddChildToVerticalBox(NombreSala);
		AgregarEspacio(WidgetTree, CrearSala);
		AgregarTexto(WidgetTree, CrearSala, TEXT("Hasta 5 jugadores. Elijan bandos y marquense listos antes de iniciar."), 16, Gris);
		AgregarEspacio(WidgetTree, CrearSala, true);
		BotonCrear = AgregarBoton(WidgetTree, CrearSala, TEXT("ABRIR SALA  >"), Ambar, Fondo);
		AgregarBoton(WidgetTree, CrearSala, TEXT("<  Volver"))->OnClicked.AddDynamic(this, &UMenuDVL::MostrarPrincipal);
		AgregarEspacio(WidgetTree, CrearSala);

		UVerticalBox* BuscarSala = CrearPagina(WidgetTree, Paginas, TEXT("03  /  UNIRSE A LA PERSECUCION"),
			TEXT("Encuentra a tu gente."),
			TEXT("Las salas disponibles en tu red aparecen aqui."));
		TextoResultados = AgregarTexto(WidgetTree, BuscarSala, TEXT("SALAS DISPONIBLES"), 13, Gris, true);
		TextoListaVacia = AgregarTexto(WidgetTree, BuscarSala,
			TEXT("Todavia no hay salas. Pide a un amigo que cree una y actualiza la lista."), 18, Gris);
		ListaPartidas = WidgetTree->ConstructWidget<UScrollBox>();
		BuscarSala->AddChildToVerticalBox(ListaPartidas)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AgregarEspacio(WidgetTree, BuscarSala);
		BotonBuscar = AgregarBoton(WidgetTree, BuscarSala, TEXT("Actualizar salas"), Azul);
		AgregarBoton(WidgetTree, BuscarSala, TEXT("<  Volver"))->OnClicked.AddDynamic(this, &UMenuDVL::MostrarPrincipal);
		AgregarEspacio(WidgetTree, BuscarSala);

		UBorder* Estado = CrearTarjeta(WidgetTree, Fondo, 16.0f);
		Panel->AddChildToVerticalBox(Estado);
		UVerticalBox* PanelEstado = WidgetTree->ConstructWidget<UVerticalBox>();
		Estado->AddChild(PanelEstado);
		AgregarTexto(WidgetTree, PanelEstado, TEXT("CONEXION LAN"), 12, Ambar, true);
		TextoEstado = AgregarTexto(WidgetTree, PanelEstado, TEXT(""), 14, Gris);
		Paginas->SetActiveWidgetIndex(0);
	}
	if (!ensureMsgf(NombreSala && BotonCrear && BotonBuscar && BotonSalir && ListaPartidas && TextoEstado,
		TEXT("MenuDVL: el widget de diseno debe incluir los controles documentados con sus nombres exactos.")))
	{
		return;
	}
	BotonCrear->OnClicked.AddDynamic(this, &UMenuDVL::Crear);
	BotonBuscar->OnClicked.AddDynamic(this, &UMenuDVL::Buscar);
	BotonSalir->OnClicked.AddDynamic(this, &UMenuDVL::Salir);
}

void UMenuDVL::MostrarPrincipal()
{
	if (Paginas)
	{
		Paginas->SetActiveWidgetIndex(1);
	}
}

void UMenuDVL::MostrarCrear()
{
	if (Paginas)
	{
		Paginas->SetActiveWidgetIndex(2);
		NombreSala->SetUserFocus(GetOwningPlayer());
	}
}

void UMenuDVL::MostrarBuscar()
{
	if (Paginas)
	{
		Paginas->SetActiveWidgetIndex(3);
		Buscar();
	}
}

void UMenuDVL::NativeConstruct()
{
	Super::NativeConstruct();
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->OnMenuActualizado.AddUniqueDynamic(this, &UMenuDVL::Actualizar);
	}
	Actualizar();
}

void UMenuDVL::NativeDestruct()
{
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->OnMenuActualizado.RemoveDynamic(this, &UMenuDVL::Actualizar);
	}
	Super::NativeDestruct();
}

void UMenuDVL::Actualizar()
{
	const UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	if (!Instancia || !TextoEstado || !ListaPartidas || !BotonCrear || !BotonBuscar || !BotonSalir)
	{
		return;
	}
	TextoEstado->SetText(FText::FromString(Instancia->Mensaje));
	BotonCrear->SetIsEnabled(!Instancia->bOcupado);
	BotonBuscar->SetIsEnabled(!Instancia->bOcupado);
	BotonSalir->SetIsEnabled(!Instancia->bOcupado);
	if (Paginas)
	{
		Paginas->SetIsEnabled(!Instancia->bOcupado);
		TextoResultados->SetText(FText::FromString(Instancia->bOcupado ? TEXT("CONECTANDO CON LA RED...")
			: FString::Printf(TEXT("%d SALAS DISPONIBLES"), Instancia->Partidas.Num())));
		TextoListaVacia->SetVisibility(!Instancia->bOcupado && Instancia->Partidas.IsEmpty()
			? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	ListaPartidas->ClearChildren();
	for (int32 Indice = 0; Indice < Instancia->Partidas.Num(); ++Indice)
	{
		UEntradaPartidaDVL* Entrada = CreateWidget<UEntradaPartidaDVL>(GetOwningPlayer());
		if (ensure(Entrada))
		{
			Entrada->Configurar(Indice, Instancia->Partidas[Indice].Nombre, Instancia->Partidas[Indice].Jugadores);
			Entrada->SetIsEnabled(!Instancia->bOcupado);
			ListaPartidas->AddChild(Entrada);
		}
	}
}

void UMenuDVL::Crear()
{
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->CrearPartida(NombreSala->GetText().ToString());
	}
}

void UMenuDVL::Buscar()
{
	if (UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>())
	{
		Instancia->BuscarPartidas();
	}
}

void UMenuDVL::Salir()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void USalaEsperaDVL::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!WidgetTree->RootWidget)
	{
		UVerticalBox* Panel = CrearPanel(WidgetTree, TEXT("La ciudad esta esperando.\nElige tu lado antes de salir a las calles."));
		AgregarTexto(WidgetTree, Panel, TEXT("PUNTO DE ENCUENTRO  /  LAN"), 13, Ambar, true);
		AgregarTexto(WidgetTree, Panel, TEXT("Sala de espera"), 32, Blanco, true);
		AgregarTexto(WidgetTree, Panel, TEXT("Elige tu bando y preparate para la persecucion."), 16, Gris);
		UHorizontalBox* Bandos = WidgetTree->ConstructWidget<UHorizontalBox>();
		Panel->AddChildToVerticalBox(Bandos)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 8.0f));
		UVerticalBox* Policia = WidgetTree->ConstructWidget<UVerticalBox>();
		Bandos->AddChildToHorizontalBox(Policia)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BotonPolicia = AgregarBoton(WidgetTree, Policia, TEXT("POLICIA  /  1 PLAZA"), Azul);
		UVerticalBox* Ladron = WidgetTree->ConstructWidget<UVerticalBox>();
		UHorizontalBoxSlot* SlotLadron = Bandos->AddChildToHorizontalBox(Ladron);
		SlotLadron->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SlotLadron->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		BotonLadron = AgregarBoton(WidgetTree, Ladron, TEXT("LADRON  /  4 PLAZAS"), Ambar, Fondo);
		TextoJugadores = AgregarTexto(WidgetTree, Panel, TEXT("Esperando jugadores..."), 13, Gris, true);
		UScrollBox* Desplazamiento = WidgetTree->ConstructWidget<UScrollBox>();
		Panel->AddChildToVerticalBox(Desplazamiento)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ListaJugadoresNativa = WidgetTree->ConstructWidget<UVerticalBox>();
		Desplazamiento->AddChild(ListaJugadoresNativa);
		AgregarEspacio(WidgetTree, Panel);
		BotonListo = AgregarBoton(WidgetTree, Panel, TEXT("MARCARME LISTO"), Azul);
		BotonIniciar = AgregarBoton(WidgetTree, Panel, TEXT("INICIAR PERSECUCION  >"), Ambar, Fondo);
		BotonVolver = AgregarBoton(WidgetTree, Panel, TEXT("<  Abandonar sala"));
		TextoEstado = AgregarTexto(WidgetTree, Panel, TEXT(""), 14, Gris);
	}
	if (!ensureMsgf(BotonPolicia && BotonLadron && BotonListo && BotonIniciar && BotonVolver && TextoJugadores && TextoEstado,
		TEXT("SalaEsperaDVL: faltan controles con los nombres documentados.")))
	{
		return;
	}
	BotonPolicia->OnClicked.AddDynamic(this, &USalaEsperaDVL::ElegirPolicia);
	BotonLadron->OnClicked.AddDynamic(this, &USalaEsperaDVL::ElegirLadron);
	BotonListo->OnClicked.AddDynamic(this, &USalaEsperaDVL::AlternarListo);
	BotonIniciar->OnClicked.AddDynamic(this, &USalaEsperaDVL::Iniciar);
	BotonVolver->OnClicked.AddDynamic(this, &USalaEsperaDVL::Volver);
}

void USalaEsperaDVL::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TiempoActualizacion -= InDeltaTime;
	if (TiempoActualizacion > 0.0f || !BotonPolicia || !BotonLadron || !BotonListo || !BotonIniciar
		|| !BotonVolver || !TextoJugadores || !TextoEstado)
	{
		return;
	}
	TiempoActualizacion = 0.2f;
	const AEstadoJuegoDVL* Sala = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	const APlayerController* Controlador = GetOwningPlayer();
	const AEstadoJugadorDVL* Propio = Controlador ? Controlador->GetPlayerState<AEstadoJugadorDVL>() : nullptr;
	const UInstanciaJuegoDVL* Instancia = GetGameInstance<UInstanciaJuegoDVL>();
	if (!Sala || !Propio || !Instancia)
	{
		return;
	}
	FString Jugadores = FString::Printf(TEXT("Policias: %d/1 | Ladrones: %d/4\n"), Sala->ContarBando(EBandoDVL::Policia), Sala->ContarBando(EBandoDVL::Ladron));
	for (const APlayerState* Jugador : Sala->PlayerArray)
	{
		if (const AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(Jugador))
		{
			const TCHAR* Bando = Estado->Bando == EBandoDVL::Policia ? TEXT("Policia") :
				Estado->Bando == EBandoDVL::Ladron ? TEXT("Ladron") : TEXT("Sin elegir");
			Jugadores += FString::Printf(TEXT("%s%s%s - %s - %s\n"), *Estado->GetPlayerName(),
				Estado == Propio ? TEXT(" (Tu)") : TEXT(""), Sala->Anfitrion == Estado ? TEXT(" [Anfitrion]") : TEXT(""),
				Bando, Estado->bListo ? TEXT("Listo") : TEXT("No listo"));
		}
	}
	TextoJugadores->SetText(FText::FromString(Jugadores));
	if (ListaJugadoresNativa)
	{
		TextoJugadores->SetText(FText::FromString(FString::Printf(TEXT("EQUIPO  %d / 5    |    POLICIA %d/1    |    LADRONES %d/4"),
			Sala->PlayerArray.Num(), Sala->ContarBando(EBandoDVL::Policia), Sala->ContarBando(EBandoDVL::Ladron))));
		if (UltimaListaJugadores != Jugadores)
		{
			UltimaListaJugadores = Jugadores;
			ListaJugadoresNativa->ClearChildren();
			int32 Filas = 0;
			for (const APlayerState* Jugador : Sala->PlayerArray)
			{
				if (const AEstadoJugadorDVL* Estado = Cast<AEstadoJugadorDVL>(Jugador))
				{
					AgregarFilaJugador(WidgetTree, ListaJugadoresNativa, Estado, Propio, Sala->Anfitrion);
					++Filas;
				}
			}
			for (; Filas < 5; ++Filas)
			{
				AgregarFilaJugador(WidgetTree, ListaJugadoresNativa, nullptr, Propio, Sala->Anfitrion);
			}
		}
		CambiarEtiqueta(BotonPolicia, FString::Printf(TEXT("%sPOLICIA  %d/1"),
			Propio->Bando == EBandoDVL::Policia ? TEXT("TU BANDO: ") : TEXT(""), Sala->ContarBando(EBandoDVL::Policia)));
		CambiarEtiqueta(BotonLadron, FString::Printf(TEXT("%sLADRON  %d/4"),
			Propio->Bando == EBandoDVL::Ladron ? TEXT("TU BANDO: ") : TEXT(""), Sala->ContarBando(EBandoDVL::Ladron)));
		CambiarEtiqueta(BotonListo, Propio->bListo ? TEXT("ESTAS LISTO  /  Cancelar") : TEXT("MARCARME LISTO"));
	}
	TextoEstado->SetText(FText::FromString(Sala->bIniciando ? TEXT("Iniciando partida...") : Instancia->Mensaje));
	FString Error;
	BotonPolicia->SetIsEnabled(Sala->PuedeElegirBando(Propio, EBandoDVL::Policia, Error));
	BotonLadron->SetIsEnabled(Sala->PuedeElegirBando(Propio, EBandoDVL::Ladron, Error));
	BotonListo->SetIsEnabled(!Sala->bIniciando && Propio->Bando != EBandoDVL::SinElegir);
	BotonIniciar->SetVisibility(Sala->Anfitrion == Propio ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BotonIniciar->SetIsEnabled(!Sala->bIniciando && Sala->PuedeIniciar(Error));
	BotonVolver->SetIsEnabled(!Instancia->bOcupado && !Sala->bIniciando);
}

void USalaEsperaDVL::ElegirPolicia()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->ElegirBando(EBandoDVL::Policia);
	}
}

void USalaEsperaDVL::ElegirLadron()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->ElegirBando(EBandoDVL::Ladron);
	}
}

void USalaEsperaDVL::AlternarListo()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		if (const AEstadoJugadorDVL* Estado = Controlador->GetPlayerState<AEstadoJugadorDVL>())
		{
			Controlador->MarcarListo(!Estado->bListo);
		}
	}
}

void USalaEsperaDVL::Iniciar()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->IniciarPartida();
	}
}

void USalaEsperaDVL::Volver()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->SalirAlMenu();
	}
}

void UPartidaDVL::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	UCanvasPanel* Lienzo = WidgetTree->ConstructWidget<UCanvasPanel>();
	WidgetTree->RootWidget = Lienzo;

	USizeBox* TamanoReloj = WidgetTree->ConstructWidget<USizeBox>();
	TamanoReloj->SetWidthOverride(520.0f);
	UCanvasPanelSlot* Posicion = Lienzo->AddChildToCanvas(TamanoReloj);
	Posicion->SetAnchors(FAnchors(0.5f, 0.0f));
	Posicion->SetAlignment(FVector2D(0.5f, 0.0f));
	Posicion->SetPosition(FVector2D(0.0f, 16.0f));
	Posicion->SetAutoSize(true);
	UBorder* FondoReloj = CrearTarjeta(WidgetTree, Superficie, 10.0f);
	TamanoReloj->AddChild(FondoReloj);
	UVerticalBox* Cabecera = WidgetTree->ConstructWidget<UVerticalBox>();
	FondoReloj->AddChild(Cabecera);
	TextoReloj = AgregarTexto(WidgetTree, Cabecera, TEXT("PREPARANDO RONDA"), 28, Ambar, true);
	TextoReloj->SetJustification(ETextJustify::Center);
	TextoArtefactos = AgregarTexto(WidgetTree, Cabecera, TEXT("ARTEFACTOS  0 / 5"), 15, Blanco, true);
	TextoArtefactos->SetJustification(ETextJustify::Center);
	UTextBlock* Ayuda = AgregarTexto(WidgetTree, Cabecera, TEXT("TAB: mochila    |    Q: powerup    |    P: opciones"), 12, Gris);
	Ayuda->SetJustification(ETextJustify::Center);

	USizeBox* TamanoPowerup = WidgetTree->ConstructWidget<USizeBox>();
	TamanoPowerup->SetWidthOverride(300.0f);
	UCanvasPanelSlot* PosicionPowerup = Lienzo->AddChildToCanvas(TamanoPowerup);
	PosicionPowerup->SetAnchors(FAnchors(0.0f, 1.0f));
	PosicionPowerup->SetAlignment(FVector2D(0.0f, 1.0f));
	PosicionPowerup->SetPosition(FVector2D(20.0f, -24.0f));
	PosicionPowerup->SetAutoSize(true);
	UBorder* FondoPowerup = CrearTarjeta(WidgetTree, Superficie, 16.0f);
	FondoPowerup->SetVisibility(ESlateVisibility::HitTestInvisible);
	TamanoPowerup->AddChild(FondoPowerup);
	UVerticalBox* PanelPowerup = WidgetTree->ConstructWidget<UVerticalBox>();
	FondoPowerup->AddChild(PanelPowerup);
	AgregarTexto(WidgetTree, PanelPowerup, TEXT("EQUIPAMIENTO  /  Q"), 12, Ambar, true);
	TextoPowerup = AgregarTexto(WidgetTree, PanelPowerup, TEXT("Sin carga"), 16, Blanco, true);

	USizeBox* TamanoAvisos = WidgetTree->ConstructWidget<USizeBox>();
	TamanoAvisos->SetWidthOverride(360.0f);
	UCanvasPanelSlot* PosicionAvisos = Lienzo->AddChildToCanvas(TamanoAvisos);
	PosicionAvisos->SetAnchors(FAnchors(0.0f, 0.0f));
	PosicionAvisos->SetPosition(FVector2D(20.0f, 155.0f));
	PosicionAvisos->SetAutoSize(true);
	PanelAvisos = CrearTarjeta(WidgetTree, Superficie, 16.0f);
	TamanoAvisos->AddChild(PanelAvisos);
	UVerticalBox* ListaAvisos = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelAvisos->AddChild(ListaAvisos);
	AgregarTexto(WidgetTree, ListaAvisos, TEXT("AVISOS DE LA PARTIDA"), 12, Ambar, true);
	TextoAvisos = AgregarTexto(WidgetTree, ListaAvisos, TEXT(""), 16, Blanco);
	PanelAvisos->SetVisibility(ESlateVisibility::Collapsed);

	UScaleBox* Escala = WidgetTree->ConstructWidget<UScaleBox>();
	UCanvasPanelSlot* PosicionMenu = Lienzo->AddChildToCanvas(Escala);
	PosicionMenu->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	PosicionMenu->SetOffsets(FMargin(24.0f, 145.0f, 24.0f, 20.0f));
	Escala->SetStretch(EStretch::ScaleToFit);
	Escala->SetStretchDirection(EStretchDirection::DownOnly);
	USizeBox* TamanoMenu = WidgetTree->ConstructWidget<USizeBox>();
	TamanoMenu->SetWidthOverride(620.0f);
	TamanoMenu->SetHeightOverride(680.0f);
	Escala->AddChild(TamanoMenu);
	PanelOpciones = CrearTarjeta(WidgetTree, Superficie, 28.0f);
	TamanoMenu->AddChild(PanelOpciones);
	UVerticalBox* Panel = WidgetTree->ConstructWidget<UVerticalBox>();
	PanelOpciones->AddChild(Panel);
	AgregarTexto(WidgetTree, Panel, TEXT("OPERACION BOTIN"), 14, Ambar, true);
	TextoResultado = AgregarTexto(WidgetTree, Panel, TEXT("OPCIONES"), 34, Blanco, true);
	TextoDetalle = AgregarTexto(WidgetTree, Panel, TEXT(""), 17, Gris);
	TextoVotos = AgregarTexto(WidgetTree, Panel, TEXT(""), 15, Blanco);
	AgregarEspacio(WidgetTree, Panel, true);
	BotonContinuar = AgregarBoton(WidgetTree, Panel, TEXT("CONTINUAR  /  P"), Azul);
	BotonAceptar = AgregarBoton(WidgetTree, Panel, TEXT("QUIERO REVANCHA"), Ambar, Fondo);
	BotonRechazar = AgregarBoton(WidgetTree, Panel, TEXT("NO, VOLVER A LA SALA"));
	BotonRegresar = AgregarBoton(WidgetTree, Panel, TEXT("ANFITRION: TERMINAR Y VOLVER A LA SALA"), Azul);
	BotonSalir = AgregarBoton(WidgetTree, Panel, TEXT("ABANDONAR SESION"));
	AgregarTexto(WidgetTree, Panel, TEXT("Si el anfitrion abandona la sesion, todos regresan al menu principal."), 12, Gris);
	BotonContinuar->OnClicked.AddDynamic(this, &UPartidaDVL::Continuar);
	BotonAceptar->OnClicked.AddDynamic(this, &UPartidaDVL::Aceptar);
	BotonRechazar->OnClicked.AddDynamic(this, &UPartidaDVL::Rechazar);
	BotonRegresar->OnClicked.AddDynamic(this, &UPartidaDVL::Regresar);
	BotonSalir->OnClicked.AddDynamic(this, &UPartidaDVL::Salir);
	PanelOpciones->SetVisibility(ESlateVisibility::Collapsed);
}

void UPartidaDVL::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	TiempoActualizacion -= InDeltaTime;
	if (TiempoActualizacion > 0.0f)
	{
		return;
	}
	TiempoActualizacion = 0.1f;
	const AEstadoJuegoDVL* Estado = GetWorld()->GetGameState<AEstadoJuegoDVL>();
	const AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer());
	if (!Estado || !Estado->bEsPartida || !Controlador)
	{
		return;
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	const bool bFinal = Estado->Fase == EFasePartidaDVL::Finalizada;
	const bool bViajando = Estado->Fase == EFasePartidaDVL::Viajando;
	const APersonajeBase* Personaje = Cast<APersonajeBase>(Controlador->GetPawn());
	if (Personaje)
	{
		const UPowerupComponent* Powerup = Personaje->ComponentePowerup;
		const EPowerupDVL Tipo = Powerup->Efecto != EPowerupDVL::Ninguno ? Powerup->Efecto : Powerup->Carga;
		const FString Nombre = Tipo == EPowerupDVL::Pulso ? TEXT("PULSO DE BUSQUEDA") : TEXT("ADRENALINA");
		const FString EstadoPowerup = Powerup->Efecto != EPowerupDVL::Ninguno
			? FString::Printf(TEXT("%s\nActivo: %.1f s"), *Nombre, Powerup->SegundosRestantes())
			: Powerup->Carga != EPowerupDVL::Ninguno ? Nombre + TEXT("\nQ: activar")
			: TEXT("Sin carga\nBusca un punto de tu bando.");
		TextoPowerup->SetText(FText::FromString(EstadoPowerup));
	}
	else
	{
		TextoPowerup->SetText(FText::FromString(TEXT("Espectador")));
	}
	const FText Avisos = Controlador->ObtenerAvisosPartida();
	TextoAvisos->SetText(Avisos);
	PanelAvisos->SetVisibility(!Avisos.IsEmpty() && !bViajando && !bFinal && !Controlador->bMenuPartidaAbierto
		? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	const int32 Segundos = Estado->SegundosRestantes();
	const FString Reloj = Estado->Fase == EFasePartidaDVL::EnCurso
		? FString::Printf(TEXT("%02d:%02d"), Segundos / 60, Segundos % 60)
		: bFinal ? TEXT("RONDA FINALIZADA") : bViajando ? TEXT("CAMBIANDO DE MAPA...") : TEXT("ESPERANDO JUGADORES...");
	TextoReloj->SetText(FText::FromString(Reloj));
	TextoArtefactos->SetText(FText::FromString(FString::Printf(TEXT("ARTEFACTOS EN MOCHILAS  %d / 5"), Estado->ArtefactosRobados)));
	PanelOpciones->SetVisibility(bFinal || bViajando || Controlador->bMenuPartidaAbierto
		? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	const AEstadoJugadorDVL* Propio = Controlador->GetPlayerState<AEstadoJugadorDVL>();
	const bool bResultadoValido = Estado->BandoGanador != EBandoDVL::SinElegir;
	const bool bGano = Propio && Propio->Bando == Estado->BandoGanador;
	TextoResultado->SetText(FText::FromString(bViajando ? TEXT("UN MOMENTO...")
		: bFinal ? bResultadoValido ? bGano ? TEXT("VICTORIA") : TEXT("DERROTA") : TEXT("RONDA CANCELADA") : TEXT("OPCIONES")));
	TextoResultado->SetColorAndOpacity(FSlateColor(bFinal && bResultadoValido ? bGano ? Verde : Ambar : Blanco));
	TextoDetalle->SetText(FText::FromString(bFinal ? Estado->MotivoResultado
		: bViajando ? TEXT("Preparando el siguiente mapa sin desconectar a los jugadores.")
		: TEXT("La partida continua mientras estas en este menu.")));
	FString Votos;
	if (bFinal && bResultadoValido)
	{
		for (const APlayerState* Jugador : Estado->PlayerArray)
		{
			if (const AEstadoJugadorDVL* Votante = Cast<AEstadoJugadorDVL>(Jugador))
			{
				Votos += FString::Printf(TEXT("%s: %s\n"), *Votante->GetPlayerName(),
					Votante->bQuiereRevancha ? TEXT("Quiere revancha") : TEXT("Esperando voto"));
			}
		}
	}
	TextoVotos->SetText(FText::FromString(Votos));
	BotonAceptar->SetVisibility(bFinal && bResultadoValido ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BotonAceptar->SetIsEnabled(Propio && !Propio->bQuiereRevancha);
	BotonRechazar->SetVisibility(bFinal && bResultadoValido ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BotonContinuar->SetVisibility(!bFinal && !bViajando ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BotonRegresar->SetVisibility(!bViajando && Propio && Estado->Anfitrion == Propio
		? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	BotonSalir->SetIsEnabled(!bViajando);
}

void UPartidaDVL::Aceptar()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->VotarRevancha(true);
	}
}

void UPartidaDVL::Rechazar()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->VotarRevancha(false);
	}
}

void UPartidaDVL::Regresar()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->VolverASala();
	}
}

void UPartidaDVL::Continuar()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->AlternarMenuPartida();
	}
}

void UPartidaDVL::Salir()
{
	if (AControladorJugadorDVL* Controlador = Cast<AControladorJugadorDVL>(GetOwningPlayer()))
	{
		Controlador->SalirAlMenu();
	}
}
