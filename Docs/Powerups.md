# Powerups

## Colocarlos en el mapa

Los recursos ya estan creados en **Content > Powerups**:

- **BP_Powerup_Pulso**: punto azul exclusivo del policia.
- **BP_Powerup_Adrenalina**: punto ambar exclusivo de ladrones.
- **M_Pulso**: material que permite ver las siluetas a traves de paredes.

1. Abre de nuevo Unreal para cargar el codigo compilado.
2. Abre el mapa de la partida, normalmente **ThirdPersonMap**.
3. En el Content Browser, abre **Content > Powerups**. Si no ves la carpeta,
   vuelve a la raiz **Content** y limpia los filtros de busqueda.
4. Arrastra **BP_Powerup_Pulso** al mapa una vez.
5. Arrastra **BP_Powerup_Adrenalina** al mapa dos veces, en lugares distintos.
6. Coloca el origen de cada actor a la altura del suelo. La esfera y el
   detector ya tienen una altura relativa: no hace falta elevar todo el actor.
   Usa espacios accesibles y separados de los puntos de aparicion.
7. Guarda el mapa.

No agregues nodos, componentes ni eventos de teclado a estos Blueprints.
Tampoco cambies BP_Botin ni los cinco artefactos: los powerups son actores
distintos y no se cuentan para ganar la partida.

Los Blueprints heredan de **PuntoPowerup**. Si duplicas uno, **Class Defaults >
Powerups > Tipo** permite elegir **Pulso** o **Adrenalina**; tambien se puede
cambiar por instancia en Details. No uses **Ninguno** en un punto.

La esfera blanca, luz de color y letrero son una representacion inicial incluida,
sin modelos adicionales. El texto y la luz distinguen el tipo. Puedes cambiar
el Static Mesh del componente **Mesh** por un modelo decorativo, conservando
**ZonaRecogida** y el resto de la logica.

## Recogida y controles

- Pasa por el punto de tu bando para guardar una carga. No requiere E.
- **Q** activa la carga guardada; el HUD muestra el tipo y el tiempo restante.
- Solo puedes guardar una carga. No puedes recoger otra mientras tengas una
  guardada o un efecto activo; tampoco acumular duraciones o multiplicadores.
- El punto desaparece para todos al recogerlo. Reaparece **90 segundos despues
  de la recogida**, no despues de usar la carga.
- Cada punto tiene su propio temporizador. Solo un jugador recibe cada carga.
- Un personaje del otro bando no consume el punto.
- Los derribados, cargados, encarcelados y espectadores no pueden recoger ni
  activar powerups. Tampoco se pueden usar antes de empezar o al acabar la ronda.
- Son independientes de la mochila: no ocupan espacio entre los objetos robados.
- No crees un segundo evento Q en BP_Ladron o BP_Policia: ya lo enlaza el
  controlador C++.

Si un punto reaparece con un jugador elegible dentro, puede entregarle la carga.
Si ya estaba disponible pero rechazaba la recogida porque tenias una carga o
un efecto, sal de su zona y vuelve a entrar cuando puedas recoger.

## Pulso del policia

- Dura **5 segundos** y alcanza **1500 unidades de Unreal: 15 metros**.
- Dibuja siluetas azules de ladrones cercanos, incluso detras de paredes.
- Solo el policia que lo activa ve las siluetas.
- Los objetivos se actualizan durante el efecto: entrar o salir del radio
  cambia si son detectados.
- No muestra encarcelados ni eliminados definitivamente.
- Los ladrones reciben un aviso al activarlo.

No requiere configurar Custom Depth ni un volumen de postproceso.
El material M_Pulso ya esta creado para mallas esqueleticas, con translucencia,
emision y **Disable Depth Test** activado. Conservalo para mantener la vision
a traves de paredes. Si falta, la activacion muestra un error y no gasta la carga.

## Adrenalina de ladrones

- Dura **6 segundos** y aumenta la velocidad de movimiento un **30%**.
- No acelera los diez segundos de robo, reanimacion ni rescate.
- Al acabar, restaura la velocidad anterior del personaje.
- Un derribo cancela inmediatamente el efecto.
- Los efectos se cancelan tambien al terminar la ronda, perder el personaje
  o cambiar de mapa. Una revancha crea personajes y puntos nuevos.

## Comprobacion multijugador

El codigo y las pruebas automatizadas estan compilados. Esta lista es para
probar manualmente en Unreal; no se ha ejecutado una partida para verificar
el resultado visual o la sincronizacion entre equipos.

1. Inicia una ronda desde la sala con un policia y al menos dos ladrones.
2. Pasa con un ladron por el punto azul: no debe consumirlo. Repite con el
   policia en un punto de adrenalina.
3. Recoge el azul con el policia y pulsa Q con un ladron detras de una pared
   a menos de quince metros. Comprueba que solo el policia ve la silueta y que
   los ladrones reciben el aviso.
4. Acerca/aleja al ladron del limite y comprueba que las siluetas desaparecen
   al acabar los cinco segundos. Repite con el policia como cliente, no solo
   como anfitrion, y con un ladron encarcelado.
5. Recoge adrenalina como ladron cliente. Pulsa Q, comprueba los seis segundos,
   la mayor velocidad y el regreso a la normalidad. Repite siendo derribado
   durante el efecto.
6. Intenta recoger dos cargas y pulsa Q varias veces. No deben acumularse.
7. Cronometra noventa segundos desde una recogida: el punto debe reaparecer
   para todos. Prueba tambien dos ladrones entrando simultaneamente.
8. Termina la ronda con un efecto activo y pide revancha: no deben conservarse
   efectos ni cargas de la ronda anterior.

Las pruebas nuevas estan en **ProyectoDVL.Powerups.Adrenalina** y
**ProyectoDVL.Powerups.PulsoYRecogida**, junto a las pruebas de interacciones.
Cubren velocidad, cancelacion, duracion, radio, restricciones y reaparicion.
Se han compilado, pero no se han ejecutado.

## Recursos y empaquetado

El directorio **/Game/Powerups** ya esta incluido en Always Cook del proyecto.
Los puntos colocados y los personajes referencian sus recursos normales;
no se necesita Python en el juego empaquetado.

**Scripts/CrearRecursosPowerups.py** crea los recursos desde el editor con
PythonScriptPlugin habilitado si faltan, o valida los que ya existen sin
sobrescribirlos. Es una herramienta de creacion, no un paso para jugar ni
algo que debas ejecutar para colocar los Blueprints incluidos.
