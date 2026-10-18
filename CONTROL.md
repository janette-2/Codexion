# CONTROL — Plan de implementación Codexion

> Documento de control del desarrollo. Marcar `[x]` al completar cada micro-objetivo.
> Regla de oro: **un hito se cierra solo cuando sus criterios de verificación pasan**.
> Cada hito debe compilar con `-Wall -Wextra -Werror -pthread` y pasar `norminette` antes de empezar el siguiente.

---

## Estado actual (punto de partida)

### Hecho

- [x] Parsing y validación de los 8 argumentos (`init.c`), con detección de vacíos, ceros, negativos, no numéricos y overflow de `INT_MAX`.
- [x] Allocación e inicialización del array de coders (`coders.c`).
- [x] Allocación e inicialización del array de dongles (`dongles.c`).
- [x] Asignación nominal circular de dongles por coder (`dongles.c` → `dongles_assignation`).
- [x] Helper de tiempo en ms con `gettimeofday` (`simulation.c` → `current_time`).
- [x] `free_control` en `main.c`.

### Trabajo reciente revisado

- [ ] Se ha iniciado la inicialización y destrucción de `mtx_dongles`,
  `mtx_log` y `cond_dongles` en `main.c`.
- [ ] El ciclo de vida de los hilos todavía no está implementado: aún no hay
  `pthread_create` ni `pthread_join`.
- [ ] La inicialización de sincronización todavía debe devolver siempre un
  resultado válido y limpiar únicamente los objetos que hayan llegado a
  inicializarse.

### Deuda detectada (corregir en el Hito 1)

- [ ] `resources.c` **no está en el Makefile** (SRC) — se compila a medias o nada.
- [x] `last_compilation` es `int` en `t_coder`; debe ser `long long` (timestamp en ms).
- [ ] `dongles_available` en `resources.c` es incorrecto: exige `available_at != 0`, pero un dongle libre recién creado tiene `available_at = 0` → siempre devuelve "no disponible".
- [ ] `parse_int` comprueba `arg[0] == '\0'` **antes** de validar `arg == NULL` (riesgo de UB si algún día se llama con NULL).
- [ ] `compiling.c` contiene stubs que no compilan la lógica real (`compiling` recibe structs por valor) → se reescribe entero.
- [x] `dongles_args` / estados con números mágicos (`0/1/2`) → pasar a defines normativos.

### Falta (todo el núcleo concorrente)

Hilos, mutexes, condvars, logging serializado, adquisición atómica, cooldown real, colas heap FIFO/EDF, monitor, terminación y protocolo de error.

---

## Mapa de hitos

| # | Hito | Resultado al cerrarlo |
|---|------|-----------------------|
| 1 | Cimientos: sincronización, logging y limpieza de base | El "esqueleto seguro" sobre el que se construye todo |
| 2 | Hilos de coders y ciclo de vida básico | N hilos corriendo el ciclo compile→debug→refactor |
| 3 | Adquisición atómica de dongles + cooldown | Dos personas nunca compilan a la vez; cooldown respeta tiempos |
| 4 | Planificador FIFO con heap | Arbitraje justo por orden de llegada |
| 5 | Planificador EDF con heap | Nadie se agota con parámetros viables |
| 6 | Monitor y terminación precisa | Burnout detectado e impreso en <10 ms; parada limpia |
| 7 | Casos especiales, errores y estrés | 1 coder, fallos de malloc/pthread, sin fugas ni races |
| 8 | README, Norma y entrega | Repositorio listo para peer-evaluation |

---

## HITO 1 — Cimientos: sincronización, logging y limpieza de base

**Objetivo:** dotar a `t_control` de todos los primitivos de sincronización e imprimir el primer log con formato exacto y serializado. Sin hilos aún: solo init/destroy correctos y una función de log que se pueda llamar desde `main` para probarla.

**¿Por qué este y no otro?** No se crea ni un `pthread_create` hasta que existan los mutexes que lo protegerán: crear hilos sobre estado compartido sin sincronización enseña comportamientos erróneos que luego cuesta desaprender.

### Micro-objetivos

- [x] **1.1** Definir defines de estados (`DONGLE_BLOCKED/FREE/COOLDOWN`) y sustituir los números mágicos en `dongles.c`, `resources.c` y `compiling.c`. _Pendiente: defines de `SIM_RUNNING/STOPPED/FINISHED` para `simulation_status` (se harán al cablear la terminación)._
- [x] **1.2** Corregir tipos: `t_coder.last_compilation` → `long long`; revisar que todo campo de tiempo sea `long long`.
- [x] **1.3** Añadir a `t_control` los campos de sincronización (ya aplicado): `pthread_mutex_t mtx_dongles` (protege estado de dongles + topología + `simulation_status`), `pthread_mutex_t mtx_log` (protege la salida), `pthread_cond_t cond_dongles` (despierta a quien espera recursos).
- [x] **1.4** Implementar `init_sync(t_control *)` y `destroy_sync(t_control *, int created)` con **cleanup en orden inverso** y solo de lo inicializado (si falla el 2º mutex, destruir el 1º). **HECHO** en `sync.c`: devuelve 1/2/3 indicando qué primitiva falló y destruye ella misma lo creado antes de salir. El patrón del contador `created` es el que se reutilizará en el `pthread_create` parcial.
- [ ] **1.5** Logging con formato exacto del subject: `log_line` existe en `resources.c` con lock `mtx_log`, **pero el formato es `"%dms | coder %d | %s"` y es INCORRECTO**. Debe ser `timestamp_in_ms X <estado>` con los cinco estados exactos (`has taken a dongle`, `is compiling`, `is debugging`, `is refactoring`, `burned out`). Pendiente hasta que haya estados reales que reportar.
- [x] **1.6** `main.c`: `init_sync` → smoke test → `destroy_sync` → `free_control`. El smoke test de log se añadió y se retiró (ruido); ahora es un test de disponibilidad temporal. Falta retirarlo antes del commit.
- [x] **1.7** Makefile: `resources.c` y `sync.c` añadidos a `SRC`. **Pendiente menor:** añadir `codexion.h` como prerequisito de los `.o` (hoy, tocar el header no recompila nada) y `.PHONY: all`.
- [x] **1.8** `dongles_available` corregido: el predicado era `state == FREE && available_at` (todo timestamp no nulo contaba como disponible). Ahora es `dongle_ready()` = estado `FREE` **y** `current_time() >= available_at`.
- [ ] **1.9** Corregir el orden de comprobación NULL/dígito en `parse_int`.
- [ ] **1.10** `norminette`: 8/9 ficheros en verde. **`main.c` tiene 9 errores**: línea vacía en función, declaración no al inicio, `//` fuera de scope, espacio en vez de tab, declaración con asignación en la misma línea, y función de más de 25 líneas. El bucle del smoke test debe salir de `main` a su propia función/archivo.

### Criterios de aceptación del Hito 1

- `make re` compila sin warnings con `-Wall -Wextra -Werror -pthread`.
- `./codexion 2 500 100 100 100 1 10 fifo` imprime al menos el par de logs de prueba con timestamp relativo a 0 (o los que se decidan imprimir como smoke test) y sale con código 0.
- `norminette coders/*.c coders/*.h` sin errores.
- Argumentos invsiguientes siguen rechazándose como hasta ahora.

### Decisiones tomadas y hallazgos

- **`n == 1` no se rechaza en la validación.** `Codexion.md:322` describe 1 persona = 1 dongle como configuración válida de mesa, y `Codexion.md:344` dice que la simulación se detiene por burnout. Un primer intento de rechazarlo en `check_args` se revirtió: convertía un caso de prueba en error de argumentos.
- **La invariante "nunca el mismo dongle dos veces" vive en el punto de uso**, dentro de `dongles_available`, no en `dongles_assignation`. Con `n == 1` el assignation entrega `left == right == 1` y el guard se niega a dar luz verde.
- **Un mutex no puede aplicar esa restricción.** `pthread_mutex_lock` dos veces desde el mismo hilo no falla: se bloquea a sí mismo en silencio. La negación es un predicado dentro de la sección crítica.
- **El caso `n == 1` no necesita código propio.** `Codexion.md:340` obliga a un hilo monitor separado que detecta el agotamiento con precisión. El burnout no lo declara el propio hilo del coder. Por eso posponer `n == 1` hasta el Hito 6 no rompe nada.
- **El burnout mientras se espera ya es necesario con `n > 1`.** `Codexion.md:195` aplica a todas las personas, no solo a la sola. El `pthread_cond_timedwait` con deadline `last_compilation + time_to_burnout` es el mecanismo único.
- **`available_at` es timestamp, no flag.** El predicado original trataba cualquier valor no nulo como disponible. Ahora `dongle_ready` evalúa estado y reloj juntos, y esa pareja es lo que `mtx_dongles` protege.
- **`log_line` tiene el formato equivocado** (`%dms | coder %d | %s`). Los cinco formatos del subject se reescriben cuando haya estados reales.

### Deudas abiertas

- `main.c` fuera de Norma (9 errores); el bucle del smoke test debe salir a su propia función.
- Smoke test temporal en `main`: imprime un número crudo y debe retirarse antes del commit. Además devuelve exit 1 para `n == 1`, que es entrada válida; no debe decidir el exit code.
- `.gitignore` no excluye `*.o` ni el binario.
- Makefile sin dependencia de `codexion.h`: tocar el header no recompila los `.o`.
- `created = 3` hardcodeado en `main`: duplica el conocimiento que ya tiene `init_sync`.

---

## HITO 2 — Hilos de coders y ciclo de vida básico

**Objetivo:** cada coder es un hilo (`pthread_create`) que ejecuta el ciclo **compilar → depurar → refactorizar** con `usleep`, sin gestión de dongles reales todavía (se simula la adquisición con el log y un usleep de `time_to_compile`).

### Micro-objetivos

- [ ] **2.1** Añadir `pthread_t` a cada `t_coder` (ya está el campo; asegurar uso correcto).
- [ ] **2.2** Escribir `coder_routine(void *arg)` en `compiling.c` (reescritura completa de los stubs): recibe `t_coder *`, ejecuta el ciclo mientras `simulation_status == SIM_RUNNING` y `number_of_compiles_done < required`.
- [ ] **2.3** Fase de compile: log `is compiling` (los logs `has taken a dongle` llegarán en el Hito 3) → `usleep(time_to_compile * 1000)` → incrementar `number_of_compiles_done`, actualizar `last_compilation`.
- [ ] **2.4** Fase de debug: log `is debugging` → `usleep(time_to_debug * 1000)`.
- [ ] **2.5** Fase de refactor: log `is refactoring` → `usleep(time_to_refactor * 1000)` → volver a intentar compilar de inmediato.
- [ ] **2.6** `run_simulation(t_control *)`: crea los N hilos, comprueba fallos de `pthread_create` (si el hilo i falla: unir los 0..i-1, parar, limpiar).
- [ ] **2.7** `main.c`: llamar `run_simulation` tras `init_sync`; `pthread_join` de todos antes de `destroy_sync`/`free_control`.
- [ ] **2.8** Smoke test: `./codexion 3 1000 50 50 50 2 0 fifo` debe imprimir ciclos de los 3 hilos con timestamps crecientes y terminar por número de compiles alcanzado (aún sin burnout real).
- [ ] **2.9** Añadir flag `stop`/`simulation_status = SIM_STOPPED` accesible de forma thread-safe para cortar bucles (aunque el monitor real llegue en el Hito 6).

### Criterios de aceptación del Hito 2

- Salida con formato correcto, mensajes sin mezclar (gracias a `mtx_log`).
- La simulación termina por `number_of_compiles_required` y no por crash.
- Sin fugas: `valgrind --leak-check=full ./codexion 2 500 10 10 10 1 0 fifo` limpio (hilos unidos).
- `norminette` en verde.

---

## HITO 3 — Adquisición atómica de dongles + cooldown

**Objetivo:** el corazón del proyecto. Nadie compila sin dos dongles; nadie retiene uno esperando el segundo (anti-deadlock); el cooldown se respeta sin `usleep` bajo mutex.

### Micro-objetivos

- [ ] **3.1** Protocolo de adquisición en `mtx_dongles`: comprobar **ambos** dongles (`FREE` y `current_time >= available_at`); si no, `pthread_cond_wait(&cond_dongles, &mtx_dongles)`. **Jamás** tomar uno y esperar el otro.
- [ ] **3.2** Al adquirir: marcar ambos `IN_USE`, log dos veces `has taken a dongle` (bajo `mtx_log`), salir de la sección crítica y solo entonces compilar.
- [ ] **3.3** Al liberar (tras compile): entrar en `mtx_dongles`, marcar ambos `COOLDOWN` y `available_at = current_time() + dongle_cooldown`, `pthread_cond_broadcast`/`signal`, salir.
- [ ] **3.4** Gestión del cooldown sin dormir bajo mutex: quien encuentra un dongle en `COOLDOWN` espera en la condvar. Como `pthread_cond_timedwait` **sí** está autorizada, usarla con un `pthread_mutex_t` + `timespec` calculado desde `available_at` (evita un hilo relevo y clava la precisión). Alternativa: un "relevo" que haga `usleep` el cooldown más corto y luego `broadcast`.
- [ ] **3.5** Caso `number_of_coders == 1`: un solo dongle, el coder nunca puede adquirir el par → debe caer en burnout vía monitor (Hito 6); aquí basta con que **no** se cuelgue: la condvar + monitor lo resolverá. No usar el mismo dongle dos veces, no bloquear el mismo mutex dos veces.
- [ ] **3.6** Reescribir/eliminar `dongles_available` con la lógica correcta (estado + `available_at <= now`).
- [ ] **3.7** Test manual: `./codexion 4 800 200 150 100 3 50 fifo` — verificar que cada `is compiling` está precedido de exactamente dos `has taken a dongle` y que nunca hay dos `is compiling` simultáneos para el mismo dongle (revisar logs).
- [ ] **3.8** Test de estrés de deadlock: `./codexion 5 200 50 10 10 5 30 fifo` en bucle 20 veces; el programa nunca debe colgarse indefinidamente (si alguien se agota, es comportamiento esperado y deberá imprimirse — llega completo en Hito 6; de momento basta con que no haya hang).

### Criterios de aceptación del Hito 3

- Cero interbloqueos en 20 ejecuciones consecutivas del test de estrés.
- El cooldown es observable: tras liberar, el mismo dongle no vuelve a aparecer tomado antes de `dongle_cooldown` ms (comparar timestamps en logs).
- Compilar con TSan si el entorno lo permite (dev-only): `cc -fsanitize=thread ...` sin race reports en el flujo de dongles.

---

## HITO 4 — Planificador FIFO con heap

**Objetivo:** cuando varios coders quieren el mismo dongle, el arbitraje respeta orden de llegada. Cola de prioridad **en el heap** (malloc), como exige el subject.

### Micro-objetivos

- [ ] **4.1** Crear `scheduler.c` con un heap/array dinámico: `t_queue` con `malloc`, `push`, `pop_min`, `destroy`. Sin globals: el heap vive en `t_control` (o en la estructura de dongles) y se accede bajo `mtx_dongles`.
- [ ] **4.2** Definir la clave FIFO: contador monótono de llegada (`arrival_seq`) asignado bajo mutex al entrar en la cola.
- [ ] **4.3** Integrar: al no poder adquirir, el coder se `push` en la cola FIFO y espera en condvar. Al liberarse un dongle, el "relevo"/broadcast hace que los candidatos compitan; el heap devuelve al de menor `arrival_seq`.
- [ ] **4.4** Evitar starving simple: con FIFO y N dongles, comprobar empíricamente que todos avanzan en rondas razonables.
- [ ] **4.5** Tests: mismo test de estrés del Hito 3 con `fifo`; verificar en logs orden de concesión coherente con llegadas.
- [ ] **4.6** `norminette` + sin fugas del heap (`destroy` al final y en rutas de error).

### Criterios de aceptación del Hito 4

- Heap asignado con `malloc` y liberado siempre (éxito, burnout y error).
- Con `fifo`, dos solicitudes casi simultáneas se atienden en orden de llegada (verificable con logs en ejecución controlada).

---

## HITO 5 — Planificador EDF con heap

**Objetivo:** misma cola, clave distinta: `deadline = last_compilation_start + time_to_burnout`. El de deadline más próximo recibe el dongle. **Con `edf` y parámetros viables, nadie se agota.**

### Micro-objetivos

- [ ] **5.1** Extender la clave del heap con campo `deadline` (o comparator intercambiable según `scheduler`).
- [ ] **5.2** Actualizar `last_compilation` (ya `long long`) en cada inicio de compilación real.
- [ ] **5.3** Al elegir ganador, comparar deadlines; el heap devuelve el mínimo.
- [ ] **5.4** Re-evaluación: si mientras esperas entra alguien con deadline más próximo, debe poder adelantar (el diseño de cola+re-eval al despertar lo permite; documentar cómo).
- [ ] **5.5** Test de no-burnout: parámetros viables (p. ej. `5 600 100 50 50 3 20 edf`) — nadie debe imprimir `burned out`; la simulación termina por compiles completados.
- [ ] **5.6** Comparativa fifo vs edf en README (sección decisiones): cuándo fifo puede agotar y edf no.

### Criterios de aceptación del Hito 5

- Con `edf` y escenarios viables: **0** burnouts en 20 ejecuciones.
- Con `fifo` y un escenario deliberadamente apretado: burnout posible (comportamiento aceptado por el subject).
- Sin fugas, `norminette` en verde.

---

## HITO 6 — Monitor y terminación precisa

**Objetivo:** hilo monitor que detecta agotamiento y fin de simulación, imprimiendo `X burned out` **dentro de los 10 ms** posteriores al momento real, y parando todo limpiamente.

### Micro-objetivos

- [ ] **6.1** Hilo monitor (`pthread_create` en `run_simulation`): bucle mientras `SIM_RUNNING`.
- [ ] **6.2** Detección de burnout: para cada coder, `current_time() - last_compilation_start > time_to_burnout` (el reloj corre desde el inicio de la simulación en la primera compilación). Precisión: muestrear cada ≤5 ms o calcular el próximo deadline y dormir solo ese delta.
- [ ] **6.3** Al detectar burnout: log `burned out` (formato exacto, bajo `mtx_log`), fijar `SIM_STOPPED`, broadcast a todas las condvars para despertar a quien espera dongles o cooldown.
- [ ] **6.4** Detección de éxito: todos los coders con `number_of_compiles_done >= required` → `SIM_FINISHED`, broadcast, el monitor sale.
- [ ] **6.5** Todas las esperas (dongles, cooldown, relevo) deben comprobar `simulation_status` tras despertar y salir del bucle si no están en `SIM_RUNNING` (evitar hang tras burnout).
- [ ] **6.6** El caso `1 coder` ahora termina: cuelga en la espera → monitor detecta burnout → log → parada. Verificar.
- [ ] **6.7** `pthread_join` del monitor + de todos los coders + `destroy_sync` + `free_control`.
- [ ] **6.8** Test de precisión: script/manual que ejecute `./codexion 3 300 100 100 100 5 100 fifo` (no viable) y compruebe que `burned out` aparece en <10 ms tras la teórica expiración (timestamp del log vs cálculo).
- [ ] **6.9** `./codexion 1 200 50 50 50 1 10 edf` termina con un `1 burned out` y sin leaks.

### Criterios de aceptación del Hito 6

- Ningún hang tras burnout ni tras éxito.
- Log de burnout dentro de la ventana de 10 ms en ≥9 de 10 intentos (tolerancia de hardware).
- `1 coder` → burnout garantizado, exit 0 del programa y memoria limpia.

---

## HITO 7 — Casos especiales, errores y estrés

**Objetivo:** blindar el programa para la evaluación: entradas inválidas, fallos de recursos, fugas, races y la petición "recode".

### Micro-objetivos

- [ ] **7.1** Matriz de tests de argumentos: nº incorrecto, vacíos, negativos, `0`, no numéricos, overflow, scheduler raro. Todos deben imprimir error y salir con código ≠ 0.
- [ ] **7.2** Fallos de `malloc`: simular/forzar (o al menos estructurar el código para que cada fallo libere lo previo en orden inverso). Revisar `init_coders` (hace `free` redundante sobre puntero ya comprobado NULL).
- [ ] **7.3** Fallo de `pthread_create` a mitad: unir creados, parar simulación, destruir sync, free.
- [ ] **7.4** Suite de estrés (script en `tests/`, no entregable pero recomendado por el subject): matrices de N × tiempos × cooldown × fifo/edf; detectar hangs con timeout.
- [ ] **7.5** Valgrind completo en 5+ configuraciones; cero leaks y cero invalid reads.
- [ ] **7.6** TSan (dev) en las mismas configuraciones; cero data races.
- [ ] **7.7** Revisar que nunca se hace `usleep`/`printf` largo con un mutex distinto al de log held (contestar la pregunta en la defensa).
- [ ] **7.8** Preparar respuestas de defensa: ¿cómo se evita el deadlock (adquisición atómica)? ¿por qué heap? ¿por qué el monitor cumple 10 ms? ¿dónde está el arbitraje EDF?

### Criterios de aceptación del Hito 7

- Batería de ≥20 ejecuciones sin hang, sin crash, sin leak.
- Todos los inputs inválidos rechazados con mensaje claro.

---

## HITO 8 — README, Norma y entrega

**Objetivo:** documentación y repo listos para peer-evaluation.

### Micro-objetivos

- [ ] **8.1** README: primera línea en cursiva con los logins (ya está), secciones **Description**, **Instructions**, **Resources** (referencias + uso de IA especificando tareas/parts).
- [ ] **8.2** README: sección **Blocking cases handled** (deadlock/Coffman, inanición, cooldown, precisión de burnout, serialización de logs).
- [ ] **8.3** README: sección **Thread synchronization mechanisms** (mtx_dongles, mtx_log, cond_dongles, diseño del monitor, ejemplos concretos de race evitados).
- [ ] **8.4** Actualizar este CONTROL.md y el "Current Implementation Status" del README al estado real final.
- [ ] **8.5** `norminette` 100% en todos los `.c/.h` entregables; ningún archivo huérfano en `coders/`.
- [ ] **8.6** `make re && make clean && make fclean` verificados; Makefile sin relink, con `NAME all clean fclean re`.
- [ ] **8.7** Commit final y revisión de que solo está lo entregable (los tests no evalúan, pero se pueden dejar fuera del alcance de la Norma).

### Criterios de aceptación del Hito 8

- Un evaluador externo puede compilar, ejecutar y entender el proyecto solo con el README.
- Repo limpio, Norma en verde, sin fugas.

---

## Próximo paso: cierre del **HITO 1**

Lo que queda, en orden:

1. **1.10** `norminette` en `main.c`: subir la declaración de `i` al inicio de la función, quitar las líneas vacías, cambiar el `//` por un comentario normativo, y **sacar el bucle del smoke test a su propia función en otro archivo** (por eso `main` pasa de 25 líneas).
2. Retirar el smoke test temporal de `main` (o dejarlo explícitamente como debugger y no commitearlo).
3. **1.9** `parse_int`: comprobar `arg == NULL` antes de acceder a `arg[0]`.
4. Makefile: `codexion.h` como prerequisito de los `.o`, y `.PHONY: all`.
5. `.gitignore`: añadir `*.o` y `/codexion`.
6. Commit del Hito 1.

**Siguiente hito: HITO 2** — hilos de coders. El primer paso es `coder_routine` en `compiling.c` y el bucle de creación con contador de hilos creados, reutilizando el patrón de `init_sync`. Sin dongles reales todavía: solo el ciclo compilar → depurar → refactorizar.

El guard de `left_id == right_id` ya está escrito y verificado a mano con `n == 1` y `n == 4`; se quedará dormido hasta el Hito 3, cuando el bucle de adquisición lo invoque de verdad.

Cuando el Hito 1 esté cerrado, actualizar los checkboxes de este archivo y pasar al Hito 2.

---

## Convenciones de este plan

- **Un hito = una PR/commits temáticos.** No mezclar scheduler con monitor.
- **Antes de cada commit:** `make re` sin warnings + `norminette` + una ejecución manual de humo.
- **Variables globales prohibidas** (subject): todo estado pasa por `t_control *`.
- **Funciones autorizadas únicamente** (lista del subject): no usar `clock_gettime`, `nanosleep`, `pthread_cond_signal` (no está) ni nada fuera de: `pthread_create, pthread_join, pthread_mutex_init/lock/unlock/destroy, pthread_cond_init/wait/timedwait/broadcast/destroy, gettimeofday, usleep, write, malloc, free, printf, fprintf, strcmp, strlen, atoi, memset`.
- Si un micro-objetivo se descubre mal planteado, reescribirlo aquí antes de implementar.
