# TEORÍA — Concurrencia desde cero (para Codexion)

> Para quien nunca ha programado con hilos. Explica **por qué** existe cada pieza,
> qué problema resuelve y qué pasa si falta. Todo con dibujos.
> Versión en diagramas Mermaid: `DIAGRAMAS.md`. Resumen imprimible: `REPASO.md`.
> Estado del código que se usa como ejemplo: **Hito 2 cerrado, 3.4 y 2.8
> cerrados** — `cooldowns_deadline` + `cond_timedwait` cableados y verificados.

---

## Índice

1. [Dos cosas a la vez](#1-dos-cosas-a-la-vez)
2. [Memoria compartida: el origen de todo](#2-memoria-compartida-el-origen-de-todo)
3. [Condición de carrera, paso a paso](#3-condición-de-carrera-paso-a-paso)
4. [La llave: el mutex](#4-la-llave-el-mutex)
5. [Cuando la llave no basta: hay que esperar](#5-cuando-la-llave-no-basta-hay-que-esperar)
6. [Los tres errores clásicos de la espera](#6-los-tres-errores-clásicos-de-la-espera)
7. [Cómo se ve todo esto en Codexion](#7-cómo-se-ve-todo-esto-en-codexion)
8. [El ciclo completo, en el tiempo](#8-el-ciclo-completo-en-el-tiempo)
9. [Los cuelgues que ya nos han pasado](#9-los-cuelgues-que-ya-nos-han-pasado)
10. [Glosario](#10-glosario)

---

## 1. Dos cosas a la vez

Un programa normal ejecuta **una línea detrás de otra**. Un hilo (thread) es una
**segunda línea de ejecución dentro del mismo programa**, que corre "a la vez".

```text
   PROGRAMA SIN HILOS                PROGRAMA CON 3 HILOS

   instrucción 1                     hilo 1 ──> instrucción A ──> B ──> C
   instrucción 2                     hilo 2 ──> instrucción X ──> Y ──> Z
   instrucción 3                     hilo 3 ──> instrucción P ──> Q ──> R
   instrucción 4
   ...                               El sistema operativo decide, en cada
                                     instante, quién avanza. El orden NO es
                                     el orden en que los creaste.
```

**Primera idea clave:** crear el hilo 1 antes que el hilo 2 **no** garantiza que
el hilo 1 empiece antes ni que termine antes. El sistema operativo los alterna.

**Segunda idea clave:** esos tres hilos **comparten la misma memoria** del
programa. Ahí nace el problema y también la utilidad (se comunican por esa
memoria).

---

## 2. Memoria compartida: el origen de todo

```text
              EL PROGRAMA
   ┌──────────────────────────────────┐
   │  ZONA COMPARTIDA (heap)          │   ← la ven TODOS los hilos
   │  ┌────────────────────────────┐  │
   │  │ t_control                  │  │
   │  │   coders[0..n]             │  │
   │  │   dongles[0..n]            │  │
   │  │   mutexes, condición       │  │
   │  └────────────────────────────┘  │
   ├──────────────────────────────────┤
   │  PILA del hilo 1  (privada)      │   ← cada hilo tiene la suya
   │  PILA del hilo 2  (privada)      │      (sus variables locales)
   │  PILA del hilo 3  (privada)      │
   └──────────────────────────────────┘
```

- Todo lo que está en el **heap** (`coders`, `dongles`, `mtx_*`, `cond_*`) es
  **compartido**.
- Las **variables locales** de cada hilo son **privadas**: solo las ve él.

**Regla que resume todo el proyecto:**

> Si **dos hilos** pueden leer o escribir **el mismo dato compartido**, y no hay
> coordinación, el resultado depende del azar. Eso es una **condición de carrera**
> (race condition) y es un bug.

---

## 3. Condición de carrera, paso a paso

Supón que dos hilos quieren hacer `number_of_compiles_done++` sobre un contador
que vale 0. Ese `++` no es una sola operación de la máquina: es **leer, sumar,
escribir**.

```text
   HILO 1                        HILO 2                    contador
   ─────────────────────         ─────────────────────     ────────
   lee el contador → 0                                       0
                                  lee el contador → 0       0
   suma 1 → 1                                                0
                                   suma 1 → 1               0
   escribe 1                                                 1
                                  escribe 1                 1
   ¡Se perdieron una suma!                   Debería ser 2 → queda 1
```

Nadie "hizo nada mal": cada hilo hizo su trabajo. Pero **se intercalaron** y uno
pisó al otro. Esto es una race condition.

```text
   Síntoma típico: el programa "a veces" hace menos ciclos de los pedidos,
   o el log sale desordenado, o un contador se queda corto y el programa
   no termina nunca. Nunca pasa igual dos veces.
```

**Solución:** que solo **un hilo a la vez** pueda estar en la parte de código que
toca el dato compartido. Esa parte se llama **sección crítica**.

---

## 4. La llave: el mutex

Un **mutex** (*MUTual EXclusion*) es como **una única llave de un baño**:

```text
   ┌───────────────────────────────────────────────┐
   │  SECCIÓN CRÍTICA (protegida por mtx_dongles)  │
   │                                               │
   │  lock  ──┐                                    │
   │          │  ← solo un hilo puede estar aquí   │
   │          │                                    │
   │  unlock ─┘                                    │
   └───────────────────────────────────────────────┘

   Hilo B intenta lock mientras A lo tiene:
   B SE QUEDA ESPERANDO (quieto, sin gastar CPU) hasta que A haga unlock.
```

Cómo se usa, siempre igual:

```text
   lock( mtx )
       ... tocar SOLO los datos que protege ese mtx ...
   unlock( mtx )
```

**Qué garantiza el mutex:**

- Que dos hilos no ejecutan la sección crítica **a la vez**.
- Que las lecturas/escrituras de dentro son **exclusivas**.

**Qué NO garantiza (importante):**

- **No** ordena a los hilos (no hay turnos).
- **No** arregla una lógica mal pensada: si compruebas mal, proteges mal.
- **No** te deja dormir dentro: si A hace `usleep` con la llave, B espera
  parado durante todo ese tiempo. **La sección crítica debe ser corta.**

```text
   ✔ BIEN: sección crítica de microsegundos         ✘ MAL: sección crítica de 200 ms
   lock                                             lock
   cambiar un par de datos                          usleep(200 ms)  ← todos parados
   unlock                                           unlock
```

**Regla mental:** *llave = cambiar estado, en pequeño. El trabajo largo va fuera.*

---

## 5. Cuando la llave no basta: hay que esperar

Problema real de Codexion: un coder necesita **dos dongles** y están ocupados.
¿Qué hace mientras?

**Opción mala A — dormir con la llave:**

```text
   lock
   while (dongles no libres)
       usleep(...)          ← los demás NO pueden entrar a liberarlos
   unlock
   → nunca se liberan, porque el que podría liberarlos no entra. CUELGUE.
```

**Opción mala B — soltar la llave y girar:**

```text
   lock ; mira ; unlock
   lock ; mira ; unlock      ← comprobando sin parar (busy-wait)
   lock ; mira ; unlock
   → funciona, pero quema CPU y "adivina" cuándo hay cambio.
```

**Opción buena — la condición (`pthread_cond_t`).** La condición es una **sala de
espera con un timbre**:

```text
   HILO A (espera)                          HILO B (cambia el estado)
   ──────────────────────                   ──────────────────────────
   lock
   while (no disponible)
       cond_wait(cond, mtx)
       ┌──────────────────────────────┐
       │  SUELTA la llave Y duerme    │      lock
       │  (las dos cosas a la vez)    │      ... libera el recurso ...
       │  zzz...                      │      broadcast(cond)  ──TIMBRE──┐
       └──────────────────────────────┘      unlock                     │
   (despierta, RE-Coge la llave)  <─────────────────────────────────────┘
   vuelve a comprobar el while
```

**El detalle que lo hace genial:** `cond_wait` hace **dos cosas a la vez, sin
hueco posible**: suelta la llave y se duerme. Si las hiciera por separado, habría
un instante donde nadie tiene la llave y nadie escucha el timbre → ojo perdido.

```text
   ¿Por qué "a la vez" importa?
   Si hicieras:  unlock(); ...duerme...; lock()
   entre el unlock y el dormir, B podría sonar el timbre.
   A no estaba escuchando todavía → OJO PERDIDO → A duerme para siempre.
   cond_wait elimina ese hueco: es atómico.
```

**`pthread_cond_broadcast` = timbre para TODOS.** (También existe `signal`, que
despierta a uno, pero en este proyecto no está permitida y no la queremos: ver
sección 6.)

### 5.1 Dos motivos de no poder, dos formas de dormir (el concepto que cuesta)

Hasta aquí solo hay **un** motivo de espera: *otro tiene el recurso*. Y **un**
método de despertar: **el timbre** (`broadcast`). Pero fíjate en el límite del
timbre: **solo suena cuando alguien hace algo** (alguien suelta recursos).

¿Y si lo que esperas **no es una acción de nadie, sino que pase el tiempo**?

Ese es el cooldown: a las 60 el dongle estará listo **no porque alguien haga
nada**, sino porque el reloj llega a las 60. A las 60 no hay `broadcast` sonando
— de hecho, quizá ya no quede nadie que pueda sonarlo.

```text
   ¿DE QUÉ ESPERAS?               ¿QUIÉN TE DESPIERTA?           FUNCIÓN
   ─────────────────────────────  ─────────────────────────────   ────────────────────
   a que OTRO suelte lo que       el TIMBRE: solo suena si        cond_wait
   tiene (BLOCKED)                alguien llama al broadcast      (sin fecha posible)

   a que LLEGUE UNA HORA          el DESPERTADOR: tú marcas la    cond_timedwait
   (FREE, enfriando)              hora y el SO te despierta       (usa available_at)
                                  aunque no toque nadie nada
```

Dos ideas que lo rematan:

1. **El `while` no despierta a nadie.** Él solo **revisa** cuando ya despiertas.
   Los que despiertan son **el timbre** o **el despertador**. Los dos, tras
   despertar, **caen en la misma condición del `while`**, que decide si avanza o
   vuelve a dormirse. Por eso cambiar de función no rompe el bucle: solo cambia
   *cómo duermes dentro de él*.

2. **La elección se repite en cada vuelta.** No se decide una vez para siempre.
   En la vuelta 1 puedes dormir con `cond_wait` (el otro compila); en la vuelta 2,
   con `timedwait` (ya solo falta el cooldown). El mismo hilo, dos métodos,
   **el mismo `while` reevaluando**.

```text
   vuelta 1:  right BLOCKED         → "otro lo tiene"     → cond_wait       (timbre)
   vuelta 2:  ambos FREE, enfriando → "espero una hora"   → timedwait(60)   (despertador)
   vuelta 3:  el while vuelve a mirar…
```

**Por qué el `max` de las dos fechas:** necesitas **los dos** dongles, así que
solo avanzas cuando esté listo el *último*. Elegir `max` en vez de `min` es
**eficiencia, no corrección**: con `min` te despertarías antes, re-comprobarías
con el `while` y te volverías a dormir. **El `while` hace segura cualquier
fecha**; el `max` solo evita levantarte para nada.

---

## 6. Los tres errores clásicos de la espera

### Error 1 — despertar y no re-comprobar (`if` en vez de `while`)

```text
   ✘ if (no disponible) cond_wait(...)     ✔ while (no disponible) cond_wait(...)
```

Dos motivos:

1. El sistema puede despertarte **sin que nadie toque el timbre** (despertar
   espurio).
2. El timbre despierta a **todos**, y no todos podrán coger su par. El que no
   puede, debe **volver a dormir**.

Con `if`, un despertado sigue adelante pensando que hay recurso, y quizá no lo
hay. Con `while`, siempre se revisa antes de avanzar.

### Error 2 — timbre antes del cambio

```text
   ✘ broadcast()           ✔ estado (FREE + available_at)
     estado (FREE...)        broadcast()
```

Si suena el timbre **antes** de actualizar el estado, el despertado mira, ve el
mundo **viejo**, no puede coger, y se duerme otra vez… y ya no queda nadie que
vuelva a sonar. Se llama **lost wakeup** y produce cuelgues.

Regla: **primero se cambia el estado, luego se suena el timbre** (y ambos dentro
de la misma llave, para que el cambio sea visible al despertar).

### Error 3 — dormir con la llave puesta

```text
   ✘ lock ; usleep(200 ms) ; unlock      ✔ lock ; cambiar ; unlock ; usleep(200 ms)
```

El trabajo largo (compilar, depurar, refactorizar) va **fuera** de la llave.
También el `log_line` va fuera: `log_line` ya coge su propia llave (`mtx_log`),
y coger una segunda llave dentro de la primera **anida llaves**; bloquear la
misma a mano sería **doble lock del mismo hilo** → se bloquea a sí mismo.

---

## 7. Cómo se ve todo esto en Codexion

### 7.1 Qué protege cada primitiva

```text
   DATO COMPARTIDO                          PRIMITIVA QUE LO PROTEGE
   ────────────────────────────────         ─────────────────────────
   dongles[i].state_dongle              ┐
   dongles[i].available_at              ├──> mtx_dongles
   (futuro: colas de planificación)     ┘
   printf → stdout                          ──> mtx_log
   "dónde espero / a quién despierto"       ──> cond_dongles (no es llave:
                                                  es la sala de espera)
```

### 7.2 El estado de un dongle: dos estados y una fecha

```text
   state_dongle                  available_at
   ────────────                  ────────────
   DONGLE_FREE                   marca temporal (ms) a partir de la cual
   DONGLE_BLOCKED                 se puede volver a coger

   ¿Se puede coger? -> state_dongle == FREE  Y  current_time() >= available_at
```

**Idea importante — el cooldown no es un estado, es una fecha.** Es como una
etiqueta en un táper: "no abrir antes del día 5". El táper está *cerrado*
(FREE) pero con fecha. Si lo hiciéramos un *estado* `COOLDOWN` fijo, el dongle
se quedaría atascado ahí para siempre, porque nadie lo devolvería a `FREE`.

```text
   ♥ REGLA: state_dongle y available_at son UNA SOLA COSA.
     Se leen juntos y se escriben juntos, bajo la misma llave.
     Separados, un hilo puede ver "FREE" con una fecha vieja → coge un dongle
     que todavía está enfriando.
```

### 7.3 Las dos secciones críticas del ciclo

```text
   block_dongles            [LLAVE 1]  comprobar + marcar BLOCKED      (µs)
   log taken / taken / compiling        sin llave
   usleep(time_to_compile)              sin llave   ← el trabajo
   FREE + available_at + broadcast  [LLAVE 2]  liberar y avisar         (µs)
   debug / refactor                     sin llave
```

Dos tramos cortos con llave, y el trabajo largo en medio. Ese es el patrón
entero del proyecto.

---

## 8. El ciclo completo, en el tiempo

Dos coders comparten dongles. Cada línea muestra qué hace cada uno y qué pasa en
la sala de espera.

```text
tiempo   HILO 1 (coder 1)            HILO 2 (coder 2)         sala de espera (cond)
─────────────────────────────────────────────────────────────────────────────────────
   0     lock(mtx_dongles)
   0     mientras no disponibles…  (sí están) → no espera
   0     marca d1,d2 BLOCKED
   0     unlock
   0     log: 1 has taken a dongle
   0     log: 1 has taken a dongle
   0     log: 1 is compiling
   0     usleep(50 ms)   ← compila con los dongles en la mano
  10                                          lock(mtx_dongles)
  10                                          d1 ocupado → cond_wait ──> H2 dormido
  50     lock(mtx_dongles)
  50     d1,d2 → FREE ; available_at = 60
  50     broadcast ────────────────────────────────────────────> despierta a H2
  50     unlock
  50     log: 1 is debugging          H2 re-coge la llave
  50     usleep(30 ms)                H2 mira: d2 libre pero en cooldown (60)
                                      H2 → vuelve a cond_wait ──> H2 dormido otra vez
  80     log: 1 is refactoring
  80     usleep(30 ms)
 110     lock → mira: 110 >= 60, libres → las coge otra vez
```

Fíjate en el momento clave: **H2 se despierta, comprueba, no puede todavía
(cooldown) y se vuelve a dormir**. En ese mundo "correcto", alguien debería
volver a sonar el timbre en el instante 60 (cuando el cooldown expira). **Eso es
exactamente lo que aún NO hacemos** (ver sección 9.3).

---

## 9. Los cuelgues que ya nos han pasado

Los tres son el mismo tipo de error: **un hilo se queda dormido y nadie lo
despierta**, o **nadie puede soltar lo que el otro espera**.

### 9.1 `return` dentro de la sección crítica

```text
   lock
   if (...) { ...; return (1); }     ← sale SIN unlock
   ...
   unlock                             ← nunca se ejecuta
   ────────────────────────────────────
   Síntoma: el primer hilo deja el mutex tomado para siempre;
            el siguiente que haga lock se duerme eternamente.
   Regla:   desbloquea ANTES de cualquier return, o sal solo tras el unlock.
```

### 9.2 Doble lock del mismo hilo (`log_line` dentro de la llave)

```text
   lock(mtx_dongles)
   lock(mtx_log)          ← y log_line YA bloquea mtx_log por dentro…
   log_line(...)              → el hilo se bloquea a sí mismo, al instante
   ────────────────────────────────────
   Síntoma: cuelgue inmediato en cuanto se llama a log_line desde dentro.
   Regla:   dos llaves anidadas = peligro; el log va FUERA de la sección crítica.
```

### 9.3 El cooldown sin timbre — ya resuelto (3.4)

**Estado: RESUELTO el 2026-10-21.** Se usó `pthread_cond_timedwait` (opción a),
descartando el hilo relevo: `available_at` ya es una fecha absoluta sobre el
mismo `CLOCK_REALTIME` que cuenta el `timedwait`, y `ms_to_timespec` ya existía.
Verificado: con cooldown 300 la simulación termina sola (`exit=0`, 9/9 compiles).
El problema y su solución quedan abajo como material de estudio.

```text
   Hilo libera: FREE + available_at = ahora + cooldown
   Hilo: broadcast ──> los que esperaban se despiertan
                       miran: "todavía en cooldown" → vuelven a dormir
   ...el cooldown expira (llega la fecha)...
   ¡NADIE suena el timbre al expirar!  → siguen dormidos
   Si además el último coder activo termina y sale, nadie los despertará
   jamás  →  pthread_join no vuelve  →  el programa CUELGA.
   ────────────────────────────────────
   Solución (elegida): (a) pthread_cond_timedwait — dormir hasta una FECHA
         (available_at), para que el propio hilo se despierte cuando expire.
         [Descartada (b) hilo relevo: ya existe `ms_to_timespec` y
         `available_at` es fecha absoluta sobre el mismo reloj.]
```

Esto se reproducía en vivo: con `./codexion 2 2000 50 30 30 1 10 fifo`, el
coder 1 completaba su ciclo y el programa se quedaba en el `join` del coder 2.
**Hoy ese mismo comando termina solo.**

### 9.4 El busy-spin: dormir mal es peor que no dormir (lo nuevo)

Añadir `timedwait` introduce un fallo que **no existía antes**: que la fecha sea
**pasada**. Si le pides al despertador que sonde a las 00:00 de 1970, no duerme:
**devuelve timeout al instante**. Y como el `while` vuelve a mirar y otra vez
calcula la misma fecha pasada... otra vez timeout... **giras sin dormir nunca.**

```text
   deadline en el PASADO
        │
        ▼
   timedwait → ETIMEDOUT al instante   ─┐
        │                                │ no hay sleep en ninguna parte
        ▼                                │
   vuelve al while → misma fecha pasada ─┘   →  BUCLE CERRADO, 100 % CPU
```

```text
   SÍNTOMA (lo separa de un cuelgue normal):
     cuelgue sano   → estado S (sleeping), 0 % CPU, sin más logs
     giro           → estado R (running),  ~100 % CPU, sin más logs

   Útil en un segundo:  ps -o %cpu,stat -C codexion
```

**Los tres caminos para acabar con una fecha pasada** (los tres dan el mismo
síntoma):

1. Retorno `int` para `available_at`: época en ms ≈ **1.79 billones** y `INT_MAX`
   ≈ 2.1 mil millones → **trunca**, casi seguro a negativo.
2. `available_at` **viejo** de un dongle `BLOCKED` (fecha de una liberación
   anterior, o `0` inicial): en `BLOCKED` **no hay fecha posible** que calcular.
3. **`n == 1`**: `available_at = 0` y la indisponibilidad viene del guard
   `left == right`, **no del tiempo**. Si no lo mandas a espera infinita, giras.

**La defensa común a los tres:** solo usa `timedwait` cuando la fecha sea
**estrictamente futura**; en cualquier otro caso → espera infinita. En la práctica:
`left == right` o `state != FREE` → `DEADLINE_NEVER` (-1) → `cond_wait`. Con
`left != right` y ambos `FREE` **dentro del `while`**, el `max(available_at)` ya
es futuro garantizado (si no lo fuera, `dongles_available` te habría dejado salir).

También es una trampa de diseño: **la fecha no la calcula "la función buena", la
calcula un `if` con las guardas delante.** Primero "¿me importa?", después
"¿qué leo?" — leer estado compartido para descartarlo después es al menos confuso,
y en este proyecto es leer estado compartido sin necesidad.

---

## 10. Glosario

| término | significado en una línea |
|---|---|
| **hilo (thread)** | línea de ejecución dentro del proceso; comparte el heap con los demás. |
| **race condition** | dos hilos tocan el mismo dato sin coordinación; el resultado depende del azar. |
| **sección crítica** | el trozo de código entre `lock` y `unlock`. |
| **mutex** | llave de exclusión mutua: solo un hilo dentro de la sección crítica a la vez. |
| **deadlock** | dos o más hilos esperándose en círculo; nadie puede avanzar. |
| **livelock / busy-wait** | los hilos avanzan pero giran sin progresar útilmente; quema CPU. |
| **condición (condvar)** | sala de espera con timbre: `cond_wait` duerme (y suelta la llave), `broadcast` despierta a todos. |
| **lost wakeup** | el timbre suena antes de que el otro esté escuchando; el dormido no despierta. |
| **wakeup espurio** | el sistema despierta a un hilo sin que nadie suene; por eso se re-comprueba con `while`. |
| **atomicidad** | una operación que ocurre "entera" o no ocurre; no se puede partir por la mitad. |
| **buffer (stdout)** | la salida se acumula y se escribe por bloques; a un pipe puede no verse hasta el final. |

---

## 11. Frases para recordar

```text
   • Llave = cambiar estado, en pequeño. El trabajo largo va fuera.
   • Un dato compartido, una llave. Si lo tocan dos, hay un camino sin vigilar.
   • Si puedes perder un turno esperando, usa while, nunca if.
   • Primero cambia el estado, DESPUÉS suena el timbre.
   • cond_wait suelta la llave y duerme a la vez: nunca hay hueco para perder el timbre.
   • El cooldown es una FECHA, no un estado.
   • Si un hilo se duerme, alguien tiene que poder despertarlo. Siempre.
```
