# CONTROL — Plan de implementación Codexion

> Documento de control del desarrollo. Marcar `[x]` al completar cada micro-objetivo.
> Regla de oro: **un hito se cierra solo cuando sus criterios de verificación pasan**.
> Cada hito debe compilar con `-Wall -Wextra -Werror -pthread` y pasar `norminette` antes de empezar el siguiente.

---

## Estado actual (Hito 2 cerrado; **3.4 y 2.8 cerrados: la simulación termina sola**)

### Hecho — cimientos (Hito 1)

- [x] Parsing y validación de los 8 argumentos (`init.c`), con detección de vacíos, ceros, negativos, no numéricos y overflow de `INT_MAX`.
- [x] Comprobación NULL antes de `arg[0]` en `parse_int`.
- [x] Allocación e inicialización del array de coders (`coders.c`).
- [x] Allocación e inicialización del array de dongles (`dongles.c`).
- [x] Asignación nominal circular de dongles por coder (`dongles.c` → `dongles_assignation`).
- [x] Helper de tiempo en ms con `gettimeofday` (`simulation.c` → `current_time`).
- [x] `free_control` en `main.c`.
- [x] `init_sync` / `destroy_sync` en `sync.c`, con cleanup parcial vía contador de primitivas creadas. Extiende a `pthread_cond_init/destroy` sobre el mismo contador.
- [x] `dongle_ready` + `dongles_available` con predicado correcto (estado `FREE` **y** `current_time >= available_at`) y guard `left_id == right_id`.
- [x] Makefile: `resources.c` y `sync.c` en `SRC`, `codexion.h` como prerequisito de los `.o`, `.PHONY` completo.
- [x] `.gitignore` con `*.o` y `/codexion`.
- [x] `main.c` en Norma y en verde.

### Hecho — ciclo del coder (Hito 2)

- [x] **2.0** Formato de log exacto (`"%d %d %s\n"`) con las cinco constantes en `codexion.h`.
- [x] **2.1** Campo `pthread_t coder_pthread` en `t_coder`. Su uso llega con 2.6.
- [x] **2.2** `coder_routine(void *arg)` con firma correcta, cast a `t_coder *`, `return (NULL)` y condición de bucle que lee estado en vivo.
- [x] **2.3** Fase de compile: log `is compiling` → `usleep(time_to_compile * 1000)` → incremento del contador y `last_compilation` al empezar.
- [x] **2.4** Fase de debug: log + `usleep`.
- [x] **2.5** Fase de refactor: log + `usleep` y vuelta inmediata al bucle.
- [x] Fases separadas en tres funciones; `compiling.c` con 5 funciones (límite Norma).
- [x] Adquisición delegada en `block_dongles` (`compiling.c`) con `while + pthread_cond_wait` — adelanto del micro-objetivo **3.1**.
- [x] Dos secciones críticas separadas por el `usleep`: coger / trabajar / devolver.
- [x] Devolución bajo `mtx_dongles` con `FREE` + `available_at = ahora + cooldown` pareados — adelanto del **3.3**.
- [x] `pthread_cond_broadcast` en la devolución, después de `FREE + available_at` y antes del `unlock`.
- [x] Dos logs `has taken a dongle` antes del `is compiling`, fuera de toda sección crítica (orden de `Codexion.md:232`).
- [x] `block_dongles` en `void`: sin camino de fallo porque el `while` garantiza el par al salir; el `if` muerto de `compiling_phase` eliminado.
- [x] Prototipos en `codexion.h` incluido `block_dongles` y `cond_dongles` en `t_control`.
- [x] **2.6** `run_simulation` en `coders.c`: bucle de `pthread_create` con `&control->coders[i]` y parada en el primer fallo; el contador `i` sirve de "creados" sin variable extra.
- [x] **2.7** `main.c`: `run_simulation` entre `init_sync` y `destroy_sync`; `pthread_join` de los creados antes de destruir nada.
- [x] Prototipo de `run_simulation` en `codexion.h`.

### Verificación actual (2026-10-21)

- `make re`: sin warnings con `-Wall -Wextra -Werror -pthread` (exit 0).
- `norminette`: 9/9 OK. `block_dongles` ≤ 25 líneas (helper `put_dongles_blocked`
  extraído a `dongles.c`), `compiling.c` 5/5.
- **La ejecución completa termina sola.** Criterio de 2.8 cumplido:

  ```text
  $ timeout 8 stdbuf -oL ./codexion 3 3000 50 20 20 1 1 fifo
  0 2 has taken a dongle
  ...
  176 3 is refactoring
  exit=0        15 líneas   3/3 compiles
  ```

- **Cooldown alto (3.4 de verdad):**

  ```text
  $ timeout 8 stdbuf -oL ./codexion 3 3000 50 20 20 3 300 fifo
  exit=0        45 líneas   9/9 compiles
  ```

- **Timestamps:** 0 decrecimientos estrictos; **cada coder estrictamente
  creciente**. (Hay 18 líneas con el mismo ms entre dos coders distintos: es
  normal, no es un desorden.)
- **`n == 1` colgando — y durmiendo de verdad:**

  ```text
  $ timeout 5 ./codexion 1 500 50 50 50 1 10 edf   →  exit=124   (esperado: falta monitor)
  $ ps -o %cpu,stat,wchan -C codexion               →  0.0 %  Sl  futex_wait
  ```

  Estado `Sl` + 0 % = **duerme**. Es la prueba de que no hay busy-spin: si la
  fecha fuera pasada, sería estado `R` a ~100 % CPU.
- **Validación de argumentos:** `0`, `x`, aridad incorrecta y scheduler inválido
  → todos `exit=1`.
- **Valgrind:** `All heap blocks were freed -- no leaks are possible`,
  `ERROR SUMMARY: 0 errors`.

### Deuda pendiente

- [x] **3.4 — el cooldown sin timbre.** **CERRADO (2026-10-21).** `block_dongles`
  decide dentro del `while`: `cooldowns_deadline` → `DEADLINE_NEVER` ⇒ `cond_wait`;
  fecha ⇒ `ms_to_timespec` + `pthread_cond_timedwait`. Verificado con cooldown 300:
  `exit=0`, 9/9 compiles, y `n == 1` durmiendo al 0 % CPU.
- [x] **2.8** Ejecución completa con 3 hilos. **CERRADO (2026-10-21):** `exit=0`
  con `./codexion 3 3000 50 20 20 1 1 fifo` y con cooldown 300.
- [x] **2.9** Flag `simulation_status` en el bucle de `coder_routine`. **CERRADO
  (2026-10-21):** `SIM_RUNNING`/`SIM_STOPPED` en `codexion.h`, comprobación en la
  condición del `while`. Gancho listo para el monitor (Hito 6); la mitad que cubre
  al hilo dormido es el **6.5**.
- [ ] `printf` a un pipe se bufferiza por bloques (en terminal va por líneas). Para testear redirigido: `stdbuf -oL`. Pendiente decidir si afecta a la precisión del log de burnout (Hito 6/7).
- [x] **[2026-10-21] `DONGLE_COOLDOWN` retirado.** Nunca se asignaba (el enfriamiento lo expresa
  `available_at`, no un estado) y no tenía ningún uso. Fuera el `#define` y el
  comentario obsoleto `2 for cooldown` de `state_dongle` en `codexion.h`.
  `cooldown_available_time_dongle` ya no existía en los fuentes. Verificado:
  `make re`, norminette 9/9 y `exit=0` sin regresión.
- [x] ~~`pthread_cond_broadcast` ausente en la devolución.~~ Cerrado.
- [x] ~~Código muerto: `block_dongles` en `int` + `if` siempre cierto.~~ Cerrado, ahora `void`.
- [x] ~~Faltaban los dos `LOG_TAKEN`.~~ Cerrados, en orden correcto.
- [x] ~~`log_line` con formato incorrecto.~~ Cerrado en 2.0.
- [x] ~~`compiling.c` con stubs que pasan structs por valor.~~ Reescrito entero.
- [x] ~~`resources.c` no estaba en el Makefile.~~ Corregido.
- [x] ~~`dongles_available` exigía `available_at != 0`.~~ Corregido con `dongle_ready`.
- [x] ~~`parse_int` comprobaba `arg[0]` antes que `arg == NULL`.~~ Corregido.

### Falta (núcleo concorrente por delante)

`simulation_status` (2.9), colas heap FIFO/EDF (Hito 4–5), monitor y terminación (Hito 6), estrés y errores (Hito 7).

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
- [x] **1.5** Logging con formato exacto del subject. **CERRADO**, movido al Hito 2 como **2.0** (hasta entonces no había estados reales que reportar). Se deja aquí como referencia de que no queda pendiente.
- [x] **1.6** `main.c`: `init_sync` → smoke test → `destroy_sync` → `free_control`. El smoke test de log se añadió y se retiró; el de disponibilidad también se retiró. **Cerrado.**
- [x] **1.7** Makefile: `resources.c` y `sync.c` en `SRC`, `codexion.h` como prerequisito de los `.o` y `.PHONY: all` añadido. **Cerrado.**
- [x] **1.8** `dongles_available` corregido: el predicado era `state == FREE && available_at` (todo timestamp no nulo contaba como disponible). Ahora es `dongle_ready()` = estado `FREE` **y** `current_time() >= available_at`. Más el guard `left_id == right_id`. **Cerrado.**
- [x] **1.9** Orden de comprobación NULL/dígito en `parse_int`. **Cerrado.**
- [x] **1.10** `norminette` en verde 9/9. El bucle del smoke test se eliminó de `main`. **Cerrado.**

### HITO 1 — CERRADO

Compila limpio, norminette 9/9, ejecuta sin salida ni errores con `n == 1` y `n == 4`. Pendiente único: el formato de log, movido al 2.0.

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

- [x] **2.0** Formato de log exacto: `log_line` (`resources.c`) imprime `"%d %d %s\n"` con el tiempo relativo a `start_time` y el mensaje aportado por quien llama. Los cinco estados viven como `#define` en `codexion.h` (`LOG_TAKEN`, `LOG_COMPILING`, `LOG_DEBUGGING`, `LOG_REFACTORING`, `LOG_BURNEDOUT`), de modo que el texto exacto se define en un solo lugar y el salto de línea vive solo en el formateador. El lock `mtx_log` protege el `printf` completo. **Cerrado.**
- [x] **2.1** `pthread_t coder_pthread` en `t_coder`. **Hecho** (campo presente); su uso real llega con `pthread_create` en 2.6.
- [x] **2.2** `coder_routine(void *arg)` en `compiling.c`, reescribiendo los stubs: recibe `t_coder *`, firma `void *` con `return (NULL)`, y el `while` lee `number_of_compiles_done` en vivo. **Hecho**, y ampliado con la comprobación de `simulation_status` en **2.9**.
- [x] **2.3** Fase de compile en `compiling_phase`: log `is compiling` (los logs `has taken a dongle` llegarán en el Hito 3) → `usleep(time_to_compile * 1000)` → incrementar `number_of_compiles_done`, actualizar `last_compilation` al inicio. **Hecho**, con la adquisición delegada en `block_dongles`.
- [x] **2.4** Fase de debug: log `is debugging` → `usleep(time_to_debug * 1000)`. **Hecho.**
- [x] **2.5** Fase de refactor: log `is refactoring` → `usleep(time_to_refactor * 1000)` → vuelta inmediata al bucle. **Hecho.**
- [x] **2.6** `run_simulation(t_control *)`: crea los N hilos con `&control->coders[i]`, para en el primer fallo de `pthread_create` y usa `i` como contador de creados. **Hecho** en `coders.c` (21 líneas).
- [x] **2.7** `main.c`: `run_simulation` tras `init_sync`; `pthread_join` de los creados (dentro de `run_simulation`) antes de `destroy_sync`/`free_control`. **Hecho.**
- [x] **2.8** Smoke test: `./codexion 3 3000 50 20 20 1 1 fifo` imprime el ciclo con timestamps crecientes. **CERRADO (2026-10-21):** `exit=0`, 3/3 compiles; y con cooldown 300, `exit=0` con 9/9 compiles.
- [x] **2.9** Flag `simulation_status` accesible de forma thread-safe para cortar bucles. **CERRADO (2026-10-21).** Hecho: `# define SIM_RUNNING 1` / `SIM_STOPPED 2` / `SIM_FINISHED 3` en `codexion.h` (los valores se reasignaron para que `0` **no** signifique "corriendo": `0` es lo que vale toda memoria sin inicializar, y un `RUNNING 0` haría que un `t_control` sin pasar por `init` pareciese en marcha); `init.c` usa `SIM_RUNNING`; `coder_routine` sigue solo mientras `simulation_status == SIM_RUNNING` (así reacciona tanto a `SIM_STOPPED` como a `SIM_FINISHED`). Hoy la comprobación reacciona al monitor del Hito 6. **Ojo: 2.9 no basta solo** — el hilo dormido está dentro del `while` de `block_dongles`, y ese lo cubre el **6.5**. Ver "Hallazgos del 2.9".
- [ ] **2.9b (deuda abierta)** El problema real de "cortar bucles": hay que comprobar `simulation_status` también **dentro del `while` de `block_dongles`**, tras despertar. Es el micro-objetivo **6.5**; queda anotado aquí para no dar el 2.9 por completo.

### Hallazgos del Hito 2

- **Firma obligatoria de la rutina de hilo.** `pthread_create` solo acepta funciones con firma exacta `void *f(void *arg)`. No admite dos parámetros y no admite retorno `void`. Todo lo que la rutina necesita tiene que colgar de un único puntero de contexto: aquí, el `t_coder *`, que ya lleva `control`, `id` y los dongles.
- **`void *` es dirección sin tipo; la conversión es cosa nuestra.** El hilo recibe `void *`, que solo guarda una dirección. El cast `(t_coder *)arg` le dice al compilador qué hay en esa dirección. `pthread_create` no convierte nada por nosotros.
- **Puntero frente a copia.** `t_coder coder` crea una copia del struct en la pila; `t_coder *coder` apunta al struct original. Con copia, `number_of_compiles_done++` ocurre en la copia y el contador real nunca se mueve. Regla para la defensa: si quitas el `*` y el programa compila igual, acabas de encontrar un bug de copia silencioso.
- **No congelar el estado en la condición del bucle.** Primer intento: `compiles_done = coder->number_of_compiles_done;` antes del `while`, y comparar contra esa variable local. Las fases incrementan el campo del struct, la local queda a 0 y el bucle nunca sale. La condición debe leer el estado en vivo: `coder->number_of_compiles_done < ...`.
- **Lecturas de configuración sí se pueden congelar.** `compiles_required` es constante durante la simulación, así que copiarlo a una local es inocuo y legible. La diferencia con `number_of_compiles_done` es que este lo cambian las fases. Si se copia uno y no el otro, hay que poder justificarlo en la defensa.
- **Trampa de unidades en `usleep`.** El subject da milisegundos y `usleep` espera microsegundos: `usleep(time_to_compile * 1000)`.

### Hallazgos del Hito 2 — concurrencia real

- **"Fase" y "sección crítica" no son lo mismo.** Una *fase* es un paso del algoritmo (coger / trabajar / devolver). Una *sección crítica* va de un `pthread_mutex_lock` a su `pthread_mutex_unlock`. Los límites del lock no existen en el lenguaje: no hay llaves de C que los marcan, solo tu código. El patrón del proyecto es **sección crítica corta → trabajo largo sin llave → sección crítica corta**.
- **`return` dentro de la sección crítica es un cuelgue.** Primer intento de `block_dongles`: los dos `return` salían de la función con `mtx_dongles` puesto; el `unlock` final quedaba como código muerto tras un `else`. Resultado: la primera llamada deja el mutex tomado para siempre y el siguiente hilo se duerme en el `lock`.
- **`log_line` ya bloquea `mtx_log` por dentro.** Un `pthread_mutex_lock(&mtx_log)` manual alrededor de la llamada es un doble lock desde el mismo hilo → auto-cuelgue inmediato. Y anidado dentro de `mtx_dongles`, además dos llaves sin motivo. El log va **fuera** de la sección crítica de dongles.
- **La condición se invierte cuando cambia de signo el objetivo.** `dongles_available` devuelve 1 si se puede coger. El `while (disponible == 0)` pregunta "mientras NO se pueda"; el `if (disponible == 1)` anterior preguntaba lo contrario. Un signo mal puesto hace que se cojan los dongles cuando están ocupados.
- **La devolución solo existe si hubo cogida.** Primer intento con guardia: el bloque 2 se ejecutaba también en el camino de fallo, y escribía `FREE` sobre dongles que otro hilo tenía en uso → liberación ajena. La fase de devolución está condicionada a haber cogido.
- **El cooldown cuenta desde la devolución, no desde la cogida.** Si se marca `available_at = ahora + cooldown` al coger y se compila 200 ms con un cooldown de 50 ms, al devolver el enfriamiento ya expiró. El cooldown es una fecha futura que se escribe al soltar.
- **`state_dongle` y `available_at` son una sola cosa.** Se escriben juntos bajo `mtx_dongles`. Separados, un hilo puede observar `FREE` con un `available_at` viejo y coger un dongle que aún está enfriando.
- **`cond_wait` y `pthread_cond_broadcast` son pareja.** La condición es una sala de espera: `cond_wait` duerme y **suelta la llave** mientras tanto; `broadcast` suena el timbre. Sin broadcast, el que se duerme no se levanta nunca. Sin cond_wait, no hay a quién despertar.
- **`while`, no `if`, alrededor del `cond_wait`.** El SO puede despertar de forma espuria, y el broadcast despierta a todos: el que no pueda coger debe volver a dormirse. Releer la condición es la garantía.
- **El broadcast va dentro del lock, después del cambio de estado.** Si suena antes de escribir el `FREE`, el despertado comprueba y ve que nada cambió → se duerme otra vez y ya nadie vuelve a sonar. El estado debe ser visible cuando suene el timbre.
- **Norma: 25 líneas por función y 5 funciones por archivo.** La fase de compile con cogida, log, contadores y devolución dentro se pasaba de 25. La solución fue extraer la adquisición a `block_dongles`, que lleva `compiling.c` a 5 de 5: el sitio se ha acabado, cualquier función nueva del ciclo tiene que vivir en otro fichero.
- **`n == 1` con la condición activa duerme para siempre, y es correcto.** El guard `left_id == right_id` devuelve "no disponible" y el coder queda en `cond_wait` sin que nadie lo despierte. No es un bug: es el camino de burnout que remata el monitor en el Hito 6. Mientras tanto, `./codexion 1 ...` se colgará en el `pthread_join`.

### Hallazgos del Hito 2 — creación de hilos y primera ejecución

- **Se pasa la dirección del elemento del array, no una copia.** `pthread_create(..., &control->coders[i])`. El hilo trabaja sobre el struct del heap; con una copia, el contador se movería en la copia y el bucle no avanzaría.
- **`pthread_create` devuelve 0 o código de error, nunca -1.** La comprobación es `!= 0`.
- **`i` como contador de creados sin variable extra.** Como `i` solo incrementa cuando `pthread_create` tuvo éxito, al salir del bucle `i` **es** el número de hilos creados; el `join` recorre `0..i-1` y el `return` compara con `number_of_coders`. Reutiliza el patrón mental de `created` en `init_sync` sin duplicar variable.
- **`pthread_join` antes de destruir.** Unir todos los creados antes de `destroy_sync`/`free_control`: destruir mutex/cond con hilos vivos es usar objetos destruidos.
- **El orden de creación no es el orden de ejecución.** Al salir del bucle, los hilos ya están corriendo; el primero puede llevar ventaja. No se puede asumir quién compila antes.
- **La salida a un pipe se bufferiza por bloques.** `printf` en terminal va por líneas, pero a un pipe (o redirección) se acumula y no se ve hasta el final; un `timeout` mata el proceso sin volcar el buffer. Para testear redirigido: `stdbuf -oL ./codexion ...`. En terminal la precisión de ms se mantiene.
- **La primera ejecución real cuelga, y es informativo.** Con `./codexion 2 2000 50 30 30 1 10 fifo`, el coder 1 completa su ciclo (logs perfectos) y el coder 2 queda dormido: el cooldown expira pero nadie suena el timbre, y con el coder 1 fuera no hay quien lo haga. Es el **3.4**, no un fallo de 2.6/2.7. *(Resuelto con el 3.4, 2026-10-21.)*
- **Un bug puede quedarse escondido detrás de otro.** En `run_simulation`, el
  bucle de `pthread_join` **no tenía el incremento de `j`**:

  ```c
  j = 0;
  while (j < i)
      pthread_join(control->coders[j++].coder_pthread, NULL);   /* el ++ es lo nuevo */
  ```

  Ese bug **no se veía nunca** porque, mientras los hilos no terminaran, el primer
  `join` se bloqueaba para siempre y el bucle no llegaba a dar una segunda vuelta.
  Al cerrar el 3.4 los tres hilos salieron, el `join` devolvió, y entonces el bucle
  **giró al ~70 % CPU** con solo 1 tarea viva en `/proc/PID/task`. Diagnóstico útil:
  - `ps -L -o lwp,stat,wchan -p <pid>` → todas las worker `Z`/ausentes y main en `R` = el giro está en `main`, no en un hilo.
  - **Estado `R` + `wchan=0` = gira; estado `S`/`Sl` + `futex_wait` = duerme.** Es lo mismo que distingue un busy-spin de un cooldown correcto: *siempre* mira el estado, no solo el `exit`.
  - Regla para la defensa: **al arreglar un cuelgue, re-ejecuta de principio a fin.** No basta con que desaparezca el síntoma viejo; puede haber otro esperando debajo.

### Adquisición — CERRADA (adelantada del Hito 3)

- [x] `pthread_cond_broadcast(&cond_dongles)` en el bloque de devolución, bajo `mtx_dongles`, después de `FREE + available_at`.
- [x] `block_dongles` a `void` y `if` muerto borrado de `compiling_phase`.
- [x] Los dos `LOG_TAKEN` al coger, antes del `LOG_COMPILING` (orden de `Codexion.md:232`).

Ciclo con dongles reales completo: espera → coge los dos → log ×3 → compila → devuelve + cooldown + timbre. **El broadcast convierte la espera en un dormir con despertar garantizado.** Ver el "Mapa visual" al final de este documento.

### Criterios de aceptación del Hito 2

- Salida con formato correcto, mensajes sin mezclar (gracias a `mtx_log`).
- La simulación termina por `number_of_compiles_required` y no por crash.
- Sin fugas: `valgrind --leak-check=full ./codexion 2 500 10 10 10 1 1 fifo` limpio (hilos unidos) — `All heap blocks were freed`, `ERROR SUMMARY: 0 errors`. *(Medido ya sin cortar el proceso, tras cerrar 3.4.)*
- `norminette` en verde.

---

## HITO 3 — Adquisición atómica de dongles + cooldown

**Objetivo:** el corazón del proyecto. Nadie compila sin dos dongles; nadie retiene uno esperando el segundo (anti-deadlock); el cooldown se respeta sin `usleep` bajo mutex.

> **Estado (2026-10-21):** 3.1, 3.2, 3.3, **3.4** y 3.6 están **hechos**. La
> ejecución completa termina sola (`exit=0`). Pendientes reales: **3.5** (el caso
> `n == 1`, que necesita el monitor), y los tests 3.7/3.8. Ver la sección
> "Verificación actual" para el detalle de lo que ya pasa.

### Micro-objetivos

- [x] **3.1** Protocolo de adquisición en `mtx_dongles`: comprobar **ambos** dongles (`FREE` y `current_time >= available_at`); si no, `pthread_cond_wait(&cond_dongles, &mtx_dongles)`. **Jamás** tomar uno y esperar el otro. **Hecho** en `block_dongles`.
- [x] **3.2** Al adquirir: marcar ambos `BLOCKED`, log dos veces `has taken a dongle` (fuera de la sección crítica, con `mtx_log` dentro de `log_line`), salir de la sección crítica y solo entonces compilar. **Hecho.**
- [x] **3.3** Al liberar (tras compile): entrar en `mtx_dongles`, marcar ambos `FREE`, `available_at = current_time() + dongle_cooldown`, `pthread_cond_broadcast`, salir. **Hecho.** Nota: no se usa el estado `DONGLE_COOLDOWN`; el enfriamiento lo expresa `available_at`, porque `dongle_ready` exige estado `FREE`.
- [x] **3.4** Gestión del cooldown sin dormir bajo mutex. **CERRADO (2026-10-21).**
  `cooldowns_deadline` (`resources.c`, 5/5) devuelve `DEADLINE_NEVER -1` si
  `left == right` o si alguno de los dos no está `FREE` (es decir, `BLOCKED`), y
  `max(available_at)` si ambos están `FREE`; retorno `long long` (el `int`
  truncaría la época en ms). Dentro del `while` de `block_dongles`: `DEADLINE_NEVER`
  ⇒ `cond_wait`; fecha ⇒ `ms_to_timespec` + `pthread_cond_timedwait`. La decisión
  **se repite en cada vuelta**. `compiling.c` se pasó de 25 líneas y el código de
  marcar el par se extrajo a `put_dongles_blocked` (`dongles.c`).
  Verificado: cooldown 300 → `exit=0`, 9/9 compiles; `n == 1` → 0 % CPU, estado `Sl`.
- [ ] **3.5** Caso `number_of_coders == 1`: un solo dongle, el coder nunca puede adquirir el par → debe caer en burnout vía monitor (Hito 6); aquí basta con que **no** se cuelgue: la condvar + monitor lo resolverá. No usar el mismo dongle dos veces, no bloquear el mismo mutex dos veces.
- [x] **3.6** `dongles_available` con la lógica correcta (estado + `available_at <= now`). **Hecho ya en 1.8** (`dongle_ready` + guard `left_id == right_id`); se cierra aquí para no arrastrar un falso pendiente.
- [x] **3.7** Test manual: `./codexion 4 800 200 150 100 3 50 fifo`. **CERRADO (2026-10-21):** cada `is compiling` va precedido de exactamente dos `has taken a dongle` del mismo coder, y nunca hay dos compilaciones solapando. Verificado con `awk` sobre la salida real: 12/12 compiles, 24/24 taken, **0 violaciones**. Nota: el log **no dice qué dongle**, así que la única verificación posible por log es la de "dos taken por compiling"; el solape de recursos se deduce de la topología (anillo) y de que como máximo `floor(n/2)` coders compilan a la vez.
- [x] **3.8** Test de estrés de deadlock: `./codexion 5 200 50 10 10 5 30 fifo` x20. **CERRADO (2026-10-21):** 20/20 terminan (`exit=0`), 0 hangs. Un análisis fino de la salida real (125 líneas = 25 fases) da **0 violaciones**: 2 taken por compiling, ningún coder con dos compiles sin refactor entre medias, y máximo **2 coders compilando a la vez** (`floor(5/2)`, correcto para 5 dongles).

### Hallazgos del 3.4 — elegir cómo dormir

> Esta es la lección de concurrencia más importante de la sesión. No es "cómo se
> llama la función", es **por qué hay dos** y cómo sabes cuál toca.
>
> **Estado: ya implementado y verificado el 2026-10-21.** Se usa `timedwait`
> (descartado el hilo relevo).

**1. Hay dos motivos distintos de "no disponible", y se esperan de forma distinta.**

| situación | quién/desde cuándo te saca de ahí | ¿hay fecha? | espera |
|---|---|---|---|
| `BLOCKED` — otro coder lo tiene en la mano | solo el `broadcast` de quien lo suelta | **no** | `cond_wait` (infinita) |
| `FREE` + `available_at` futuro — enfriando | **pasa el tiempo** y llega la fecha | **exacta** | `pthread_cond_timedwait` |

- En `BLOCKED` **no puedes calcular nada**: "¿cuándo acabará de compilar el otro?" no lo sabe nadie. Calcularle fecha es inventar.
- En cooldown **el reloj es el evento**. Nadie hace nada en `available_at`; simplemente llega la hora.

**2. El `while` NO despierta a nadie.** Solo **revisa**. Los únicos que despiertan son dos, de naturaleza distinta: el **timbre** (`broadcast`, solo si alguien suelta algo) y el **despertador** (el timeout del `timedwait`, marcado por ti). **Ambos caen en la misma condición del `while`**, que decide si avanza o vuelve a dormir.

**3. La función de sueño se reelige en CADA vuelta.** No es una elección única. El mismo hilo puede dormir con `cond_wait` en la vuelta 1 (mientras el otro compila) y con `timedwait` en la vuelta 2 (cuando ya solo falta el cooldown). Por eso las dos conviven dentro del mismo `while`.

**4. Pregunta equivocada, función equivocada.** Primer intento: `if (dongles_available(...) != 0) return NEVER`. Falla por dos vías:
- `dongles_available` responde **"¿puedo?"** (sí/no). La decisión de espera necesita **"¿por qué no?"** (el motivo). No distingue `BLOCKED` de enfriamiento — devuelve "no" en los dos casos.
- **Código muerto**: el `while` es `dongles_available(...) == 0`, así que dentro del bucle esa condición **nunca** se cumple. Quien decide es `state_dongle != DONGLE_FREE`.
- Regla para la defensa: **`dongles_available` es la condición de la puerta; `cooldowns_deadline` es el diagnóstico del motiv**o. Dos preguntas distintas.

**5. Tres bugs distintos, UN solo síntoma: busy-spin.** Cualquier fecha que **no sea estrictamente futura** hace que `pthread_cond_timedwait` devuelva timeout **al instante** → vuelta al `while` → otro timeout → giro al 100 % de CPU sin dormir nunca. Los tres caminos:
- Retorno `int` para `available_at` (**época en ms ≈ 1.79 billones > `INT_MAX` ≈ 2.1 mil millones**) → trunca y probablemente a negativo.
- `available_at` **viejo** de un dongle `BLOCKED` (fecha de una liberación anterior, o `0` inicial).
- **`n == 1`**: `available_at = 0` y la indisponibilidad viene del guard `left == right`, **no** del tiempo → fecha de 1970 → giro. Por eso `left == right` → `DEADLINE_NEVER` **no es opcional**.
- Diagnóstico: **duerme sano = estado `S`, 0 % CPU**. **Gira = estado `R`, ~100 % CPU.** `ps -o %cpu,stat,comm -C codexion` lo separa en un segundo.

**6. `INFINITY` no es una opción.** Es una macro de `<math.h>` que es un **`float`**, no está en la lista de funciones autorizadas y no sirve para milisegundos enteros. Los timestamps del epoch son siempre ≥ 0 → `-1` como centinela, con `# define DEADLINE_NEVER -1` en `codexion.h` (mismo principio que `LOG_*`: **el valor se define en un solo sitio**).

**7. `max(available_at)` es eficiencia, no corrección.** Con `min` te despertarías pronto, re-comprobarías con el `while` y te volverías a dormir: **correcto, solo que te levantas en vano**. El `while` hace segura cualquier fecha; el `max` elimina los despertares inútiles. Esto quita el miedo a "¿y si elijo mal?": la corrección la da el bucle, no la fecha.

**8. Por qué `available_at` sirve de fecha absoluta.** `pthread_cond_timedwait` cuenta en **`CLOCK_REALTIME`**; `current_time()` lee `gettimeofday`, **el mismo reloj**. Por eso `available_at` es una fecha válida para el wait. Límite conocido: `CLOCK_REALTIME` puede saltar con NTP — en una simulación de ms es aceptable, pero conviene poder decirlo en la defensa.

**9. Norma: dónde vive el helper.** `compiling.c` está en **5/5**, no admite funciones nuevas. El helper va a `resources.c` (queda en 4/5), porque es familia de `dongle_ready`/`dongles_available`. Y se llama **dentro de `mtx_dongles`**: lee estado compartido y no lleva lock propio — déjalo escrito junto al prototipo. Si al cablear `block_dongles` se pasa de **25 líneas**, la salida es un `wait_for_pair` en `sync.c` (3 funciones, hueco para 2, y es donde temáticamente va una espera sobre la condvar).

**10. Guardas antes que cálculos.** El primer borrador leía `at_left`/`at_right`/`state_*` y *luego* comprobaba `left_id == right_id`. Sin bug hoy, pero el orden correcto es **primero "¿me importa?", después "¿qué leo?"** — sobre todo porque son lecturas de estado compartido.



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

**Objetivo:** misma cola, clave distinta: `deadline = last_compilation + time_to_burnout`. El de deadline más próximo recibe el dongle. **Con `edf` y parámetros viables, nadie se agota.**

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

- [x] **6.1** Hilo monitor (`pthread_create` en `run_simulation`): bucle mientras `SIM_RUNNING`. **HECHO.** `pthread_t monitor_pthread` en `t_control`; `monitor_routine` en `simulation.c`; `run_simulation` crea el monitor tras los coders y lo une al final.
- [x] **6.4** Detección de éxito: todos los coders con `number_of_compiles_done == required` → `SIM_FINISHED`, el monitor sale. **HECHO.** Contador `finish_coders` **reiniciado en cada vuelta** del bucle exterior (bug: si no, se inflaba y paraba a un coder a medias); comparación `== required`. Verificado 8/8 runs con `2/2/2` compiles y `exit=0`.
- [ ] **6.2** Detección de burnout: para cada coder, `current_time() - last_compilation > time_to_burnout` (el reloj corre desde el inicio de la simulación en la primera compilación). **Origen del cronómetro:** `last_compilation` se inicializa a `control->start_time` en `coder_args` (por eso `start_time` se movió en `main.c` a **después** de `check_args` y **antes** de `init_coders`). Precisión: muestrear cada ≤5 ms o calcular el próximo deadline y dormir solo ese delta.
- [ ] **6.3** Al detectar burnout: log `burned out` (formato exacto, bajo `mtx_log`), fijar `SIM_STOPPED`, monitor sale. **Decisión:** basta el **primer** burnout detectado: se loguea, se fija `SIM_STOPPED` y se sale (no se siguen registrando más coders quemados en la misma vuelta).
- [ ] **6.5** Todas las esperas (dongles, cooldown, relevo) deben comprobar `simulation_status` tras despertar y salir del bucle si no están en `SIM_RUNNING` (evitar hang tras burnout). **Pendiente clave:** el `while` de `block_dongles` aún no comprueba el flag; sin esto, un coder dormido no reacciona a `SIM_STOPPED`.
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

## Próximo paso: **Hito 6 — monitor y terminación**

Hito 1 cerrado. Hito 2 **cerrado** (2.0–2.9). Hito 3: 3.1–3.4, 3.6, 3.7 y 3.8
cerrados; queda 3.5. Hito 6 en curso: **6.1 y 6.4 hechos** (monitor creado +
salida por éxito), faltan 6.2/6.3 (burnout), 6.5, 6.6–6.9.

**Por qué el Hito 6 antes que FIFO/EDF:** hoy el único cuelgue que queda es el
burnout (el caso `n == 1` y cualquier coder que se atasque). El monitor lo
convierte en una **salida limpia con log**, y de paso:
- cierra **3.5** y **6.9** (el caso de 1 coder),
- completa el **2.9** con el **6.5** (el flag en el `while` de `block_dongles`),
- hace testeables FIFO/EDF: hoy, si un scheduler falla, no distingues scheduler de cuelgue.

Factor común de todo el hito: **despertar a quien espera**. El monitor escribe el
estado y hace `broadcast`; las esperas comprueban el flag al despertar.

Preguntas de razonamiento (el corazón del hito):
1. ¿Con qué **llave** escribe el monitor `simulation_status` para que las lecturas de las esperas (que ocurren tras despertar, con la llave re-tomada) lo vean? ¿Por qué la misma `mtx_dongles` y no una nueva?
2. ¿Qué significa "el burnout corre desde el inicio de la simulación en la primera compilación"? ¿Qué campo y qué momento hacen de origen del cronómetro? **Resuelto:** `last_compilation` se inicializa a `start_time` en el arranque (de ahí mover `start_time` antes de `init_coders`).
3. El monitor muestrea cada ≤5 ms. ¿Cómo calculas **cuándo** dormir para cumplir precisión sin girar (6.8 exige <10 ms de error)?
4. Cuando el monitor detecta burnout y hace `broadcast`, ¿basta con fijar el flag para que nadie vuelva a dormirse? Enlaza con el **6.5**.

Advertencias de prueba:
- **Cómo distinguir "duerme bien" de "gira":** `ps -o %cpu,stat,wchan -C codexion`.
  `Sl` + `futex_wait` + 0 % = durmiendo (correcto). `R` + `wchan=0` + ~100 % =
  gira: fecha pasada en el `timedwait`, o un bucle sin incremento.
- `./codexion 1 ...` hoy cuelga **durmiendo**; tras el Hito 6 debe salir con un
  `burned out` y `exit=0`.
- El parser rechaza `0` en **todos** los argumentos, incluido `dongle_cooldown`.
- Para ver la salida redirigida: `stdbuf -oL ./codexion ...`.

---

## MAPA VISUAL — el ciclo del coder, para estudiar

> Esta sección es material de estudio y de defensa. Cuando toque el Hito 8, buena
> parte de esto alimenta la sección "Thread synchronization mechanisms" del README
> que exige `Codexion.md:407`.
>
> Versiones complementarias en archivos aparte:
> - **`TEORIA.md`** — concurrencia desde cero para quien no ha usado hilos: qué es
>   un hilo, qué es una race condition, por qué el mutex, por qué la condición,
>   los tres errores clásicos de la espera, los cuelgues que ya han pasado y un
>   glosario. Muy visual, con propósitos y lógica de cada pieza.
> - **`DIAGRAMAS.md`** — los mismos esquemas en Mermaid (se renderizan como dibujo
>   en GitHub/VS Code): flujo del ciclo, secuencia hilo-contra-hilo, Gantt de la
>   línea temporal, estados del dongle y mapa mental.
> - **`REPASO.md`** — hoja de una cara A4 para imprimir: ciclo, quién protege qué,
>   logs, trampas y preguntas de defensa con respuesta corta.

### 1. Quién protege qué

```text
        ESTADO COMPARTIDO                    LO PROTEGE
  ----------------------------------    ----------------
  dongles[i].state_dongle            ---\
  dongles[i].available_at            ---/-->  mtx_dongles
  (futuro: colas del planificador)   ---/

  printf -> stdout                        -->  mtx_log

  "dónde duermo / a quién despierto"      -->  cond_dongles (NO es un lock:
                                                es la sala de espera)
```

Regla mental: **cada dato compartido tiene UNA llave.** Si un dato lo tocan dos
llaves distintas, hay un camino que nadie vigila. Si lo toca una sola, la
sección crítica de esa llave es el único sitio donde mirarlo.

### 2. Ciclo de vida del hilo (esquema del código real)

```text
              coder_routine(void *arg)          <- firma obligatoria de pthread
                        |
      +-----------------v---------------------------+
      |  while (compiles_done < compiles_required)  |  <- lee ESTADO EN VIVO
      +-----------------+---------------------------+
                        |
  +---------------------v------------------------------------+
  |  compiling_phase(coder)                                   |
  |                                                           |
  |   [A] lock mtx_dongles         <--- SECCIÓN CRÍTICA 1     |
  |       while (!dongles_available(...))                     |
  |           cond_wait  <-- duerme AQUÍ y SUELTA la llave    |
  |       marca ambos BLOCKED                                 |
  |       unlock                                              |
  |                                                           |
  |       log has taken a dongle   <- log 1                   |
  |       log has taken a dongle   <- log 2    (sin llaves)   |
  |       log is compiling                                    |
  |       last_compilation = ahora                            |
  |                                                           |
  |       usleep(time_to_compile * 1000)   <-- SIN LLAVE      |
  |                                          (el trabajo)     |
  |                                                           |
  |   [B] lock mtx_dongles         <--- SECCIÓN CRÍTICA 2     |
  |       ++number_of_compiles_done      (trabajo terminado)  |
  |       ambos FREE + available_at = ahora + cooldown        |
  |       broadcast(cond_dongles)          <-- EL TIMBRE      |
  |       unlock                                              |
  +---------------------+-------------------------------------+
                        |
  +---------------------v-------------------------------------+
  |  debugging_phase:   log + usleep(time_to_debug * 1000)     |
  +---------------------+-------------------------------------+
                        |
  +---------------------v-------------------------------------+
  |  refactoring_phase: log + usleep(time_to_refactor * 1000)  |
  +---------------------+-------------------------------------+
                        |
              vuelve al while de arriba
```

Clave del dibujo: **dos secciones críticas cortas con el trabajo largo en medio.**
Todo lo que dura (compilar) ocurre sin llave. Todo lo que toca estado compartido
ocurre con llave y en microsegundos.

### 3. La espera y el timbre, hilo contra hilo

```text
      HILO A (espera dongles)                 HILO B (los devuelve)

  lock mtx_dongles
  dongle 1 ocupado -> while no sale
  cond_wait ............................
        zzz (dormido, llave SUELTA)          lock mtx_dongles
        zzz                                  FREE + available_at
        zzz                                  broadcast ~~~~~"\  timbre
        zzz                                  unlock             |
        |<----------- despierta <-------------------------------"
  (reengancha) lock mtx_dongles
  ¿disponible? SÍ (B ya lo liberó)
  marca BLOCKED
  unlock
```

Dos razones por las que el `broadcast` va **dentro** del lock y **después** del
cambio de estado:

```text
  timbre ANTES del cambio:           timbre DESPUÉS del cambio (correcto):
  A despierta, mira, ve viejo        A despierta, mira, ve el FREE
  -> se duerme otra vez              -> sigue adelante
  -> nadie volverá a sonar           -> nadie más tiene que sonar
     (cuelgue)                          (sano)
```

### 4. Línea temporal — dos coders, sin cooldown

```text
 ms    HILO 1 (coder 1)                  HILO 2 (coder 2)          log que sale
 ------------------------------------------------------------------------------
  0    coge d1 + d2 -> BLOCKED
  0    log taken/taken/compiling                                    0 1 has taken a dongle
                                                                    0 1 has taken a dongle
                                                                    0 1 is compiling
 10    duerme compilando (50 ms)
 10                                      intenta d1 -> ocupado
 10                                      cond_wait -> DORMIDO
 60    FREE d1+d2 + BROADCAST ---timbre---> despierta
 60    log is debugging                                            60 1 is debugging
 60                                       d1 y d2 libres -> coge
 60                                       log taken/taken/compiling 60 2 has taken a dongle
                                                                    60 2 has taken a dongle
                                                                    60 2 is compiling
 60    duerme depurando (30 ms)           duerme compilando (50 ms)
 90    log is refactoring                                          90 1 is refactoring
 ...   vuelve a intentar compilar        ...
```

Mientras 1 compila, 2 duerme en la condvar sin gastar CPU. El timbre de 1 es lo
que arranca a 2. Ese es el ciclo entero del proyecto.

### 5. El peligro que ya estaba: el cooldown sin timbre (3.4 — RESUELTO)

```text
 ms    HILO 1                                HILO 2
 ---------------------------------------------------------
  0    coge d1+d2
 10    suelta: FREE + available_at=60 + broadcast
 10                                          despierta, mira: ¡cooldown! -> duerme
 10    reintenta coger: ¡cooldown! -> duerme
       (nadie tiene los dongles, ambos libres pero enfriando)
       **** NADIE volverá a sonar el timbre al expirar el cooldown ****
       AMBOS DORMIDOS -> pthread_join no vuelve -> el programa CUELGA
```

**Ese era el problema. La solución aplicada:**

```text
       dentro del while, CADA vuelta:
           deadline = cooldowns_deadline(...)      ← ¿por qué no puedo?
           si  deadline == DEADLINE_NEVER (-1)  -> cond_wait        (TIMBRE)
           si  hay fecha futura                 -> cond_timedwait  (DESPERTADOR)
                                                    hasta available_at
       al despertar, el while RE-MIRA. Sirve el timbre o el despertador.
```

El `broadcast` avisa cuando un dongle **se libera**; el `timedwait` cubre
**cuando termina de enfriarse**. Las dos fuentes de despertar conviven dentro del
mismo bucle, y la elección se repite en cada vuelta.

### 6. Propósito de cada decisión (para la defensa)

| decisión | por qué |
|---|---|
| dos secciones críticas, no una | el trabajo dura cientos de ms; con la llave puesta durante el trabajo, nadie más podría progresar. Llave = cambiar estado, no trabajar. |
| `cond_wait` en vez de girar comprobando | dormir sin gastar CPU y ser despertado por quien cambia el estado. El giro es correcto pero desperdicia el procesador y adivina cuándo volver. |
| `while` y no `if` | el SO puede despertar de forma espuria, y el `broadcast` despierta a todos: hay que re-comprobar al despertar. |
| `broadcast` y no `signal` | `signal` despierta a uno: puede despertar al que no puede coger su par y dejar dormido al que sí podía. Ojo perdido = cuelgue. |
| `broadcast` dentro del lock y tras el cambio | el despertado debe VER el nuevo estado; si el timbre suena antes, vuelve a dormirse y ya nadie suena. |
| cooldown al devolver, no al coger | el enfriamiento se cuenta desde que el recurso quedó libre, no desde que lo usaste. |
| `available_at` y no un estado `COOLDOWN` | `dongle_ready` exige estado `FREE`; un estado `COOLDOWN` estático atascaría el dongle para siempre (nadie lo devuelve a `FREE`). El enfriamiento es una fecha, no un estado. |
| logs fuera de la sección crítica | `log_line` bloquea `mtx_log` por dentro: llamarlo con `mtx_dongles` puesto anida dos llaves, y bloquear `mtx_log` a mano alrededor sería doble lock del mismo hilo (auto-cuelgue). Además el log es un informe, no una negociación. |
| `block_dongles` devuelve `void` | el `while` garantiza el par al salir: no hay camino de fallo. El caso `n==1` no devuelve 0, simplemente **no vuelve nunca** (lo remata el monitor). |
| `number_of_compiles_done++` al final | cuenta compilaciones **terminadas**: si sube al empezar, un lector (el monitor) ve trabajo que aún no ocurrió. |
| `timedwait` y no hilo relevo | `available_at` es una fecha absoluta sobre el mismo `CLOCK_REALTIME` que usa `timedwait`, y `ms_to_timespec` ya existía. Un relevo dormiría y además inventaría un segundo mecanismo de despertar. |
| `DEADLINE_NEVER` para `BLOCKED` y para `left == right` | no hay fecha calculable. Usar la `available_at` vieja de un `BLOCKED` (o el `0` inicial de `n==1`) daría una fecha pasada → `timedwait` en timeout perpetuo → **busy-spin al 100 % CPU** en vez de dormir. |
| `long long` para el deadline | la época en ms son ~1.79 billones y `INT_MAX` es ~2.1 mil millones: con `int` se trunca a un valor del pasado. |
| helper `put_dongles_blocked` en `dongles.c` | `compiling.c` estaba en 5/5 y `block_dongles` se pasaba de 25 líneas con la rama nueva. El código de marcar el par no aporta a la espera, así que fue lo natural de sacar. |

---

## Convenciones de este plan

- **Un hito = una PR/commits temáticos.** No mezclar scheduler con monitor.
- **Antes de cada commit:** `make re` sin warnings + `norminette` + una ejecución manual de humo.
- **Variables globales prohibidas** (subject): todo estado pasa por `t_control *`.
- **Funciones autorizadas únicamente** (lista del subject): no usar `clock_gettime`, `nanosleep`, `pthread_cond_signal` (no está) ni nada fuera de: `pthread_create, pthread_join, pthread_mutex_init/lock/unlock/destroy, pthread_cond_init/wait/timedwait/broadcast/destroy, gettimeofday, usleep, write, malloc, free, printf, fprintf, strcmp, strlen, atoi, memset`.
- Si un micro-objetivo se descubre mal planteado, reescribirlo aquí antes de implementar.
