# REPASO A4 — Codexion

> Hoja de una cara para imprimir. Estado: adquisición de dongles cerrada,
> hilos pendientes (2.6). Detalle completo en `CONTROL.md` y `DIAGRAMAS.md`.

---

## Qué es, en tres líneas

N coders (hilos) comparten N dongles (1 dongle entre vecinos).
Ciclo: **coger 2 dongles → compilar → soltar → depurar → refactorizar**, hasta
`number_of_compiles_required` o hasta burnout.
Fin: todos completan, o alguien se agota (lo detecta el **monitor**).

---

## El ciclo, en el código real

```text
while (compiles_done < required)
    block_dongles         [SECCIÓN CRÍTICA 1]  lock → esperar/comprobar → marcar BLOCKED → unlock
    log taken ×2          fuera de llave
    log compiling         fuera de llave
    last_compilation = ahora
    usleep(compile*1000)  fuera de llave  ← el trabajo, con los dongles en la mano
    FREE + available_at   [SECCIÓN CRÍTICA 2]  lock → ++contador → FREE×2 → cooldown → broadcast → unlock
    debug: log + usleep   fuera de llave
    refactor: log+usleep  fuera de llave
```

**Regla del dibujo:** llave = cambiar estado (microsegundos). Trabajo = sin llave.

---

## Quién protege qué

| recurso | primitiva | nota |
|---|---|---|
| `dongles[i].state_dongle` + `available_at` | `mtx_dongles` | se leen y escriben **juntos**, nunca separados |
| `printf` (stdout) | `mtx_log` | `log_line` ya lo bloquea por dentro |
| espera / despertar | `cond_dongles` | no es lock: es la sala de espera |
| (futuro) colas FIFO/EDF, `simulation_status` | por decidir | Hito 4–6 |

---

## Los 5 logs (formato exacto, `Codexion.md:232`)

```text
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

`X` = **coder**, no dongle. Salto de línea lo pone el formateador.
Un mensaje no se mezcla con otro (por eso `mtx_log`).
Burnout se imprime en < 10 ms del momento real.

---

## Reglas de oro

- **No dormir con la llave puesta.** `usleep` y el trabajo van fuera.
- **No hacer `log_line` dentro de `mtx_dongles`**: `log_line` bloquea `mtx_log`
  por dentro → dos llaves anidadas; a mano sería doble lock del mismo hilo.
- **`while`, nunca `if`, alrededor de `cond_wait`**: despertares espurios +
  el broadcast despierta a todos.
- **`broadcast` dentro del lock y DESPUÉS del cambio de estado.**
- **`broadcast` cada vez que se libera** (estado mejora). Sin timbre = cuelgue.
- **Comprobar los dos dongles, no uno**: jamás coger uno y esperar el otro.
- **La condición del bucle lee estado en vivo**, no una copia local.
- **Un dato = una llave.** Si lo tocan dos, hay un camino sin vigilar.

---

## Trampas

| trampa | correcto |
|---|---|
| unidades de `usleep` | subject en **ms**, `usleep` en **µs** → `* 1000` |
| cooldown | al **devolver** (`available_at = ahora + cooldown`), no al coger |
| estado del dongle al devolver | `FREE` + `available_at` (no un estado `COOLDOWN` fijo: atascaría el dongle) |
| contador | `++` **después** del trabajo (cuenta compilaciones terminadas) |
| `return` dentro de la sección crítica | desbloquear **antes** de salir o salir solo tras el `unlock` |
| firma de la rutina | `void *f(void *arg)` exacta; un solo puntero de contexto |
| struct por valor | siempre `t_coder *`, nunca copia |
| `n == 1` | válido: `left == right` → nunca coge par → burnout vía monitor |

---

## `n == 1` — el caso especial

La mesa tiene **1 dongle**, y `dongles_available` rechaza `left_id == right_id`.
El coder nunca adquiere el par → `cond_wait` para siempre.
No es un bug: es la ruta de burnout; la cierra el **monitor** (Hito 6).

---

## Funciones autorizadas (lista del subject)

`pthread_create`, `pthread_join`, `pthread_mutex_init/lock/unlock/destroy`,
`pthread_cond_init/wait/timedwait/broadcast/destroy`, `gettimeofday`, `usleep`,
`write`, `malloc`, `free`, `printf`, `fprintf`, `strcmp`, `strlen`, `atoi`,
`memset`.
Prohibido: `clock_gettime`, `nanosleep`, `pthread_cond_signal`, libft, globales.

---

## Preguntas de defensa (respuesta en una línea)

- **¿Cómo se evita el deadlock?** Adquisición **atómica** de los dos dongles bajo
  una sola llave: nunca se retiene uno esperando el otro.
- **¿Por qué `broadcast` y no un `signal`?** Los coders esperan pares distintos;
  despertar a uno puede dejar dormido al que sí podía coger.
- **¿Por qué `while` y no `if`?** Despertares espurios y broadcast a todos:
  hay que re-comprobar al despertar.
- **¿Por qué el trabajo está fuera de la sección crítica?** Ocupa cientos de ms;
  con la llave puesta nadie más podría progresar.
- **¿Por qué `available_at` y no un estado `COOLDOWN`?** El enfriamiento es una
  **fecha**; un estado fijo dejaría el dongle atascado para siempre.
- **¿Por qué `block_dongles` es `void`?** El `while` garantiza el par al salir:
  no existe camino de fallo. `n == 1` no devuelve 0: no vuelve nunca.
- **¿Dónde está el cooldown contado desde el momento correcto?** Al soltar, no al
  coger: mide desde que el recurso quedó libre.
- **¿Quién declara el burnout?** El hilo **monitor** (obligatorio), nunca el
  propio coder.
