#ifndef UNIFE_SIMPLE_CSEXP_H
#define UNIFE_SIMPLE_CSEXP_H

#include <stddef.h>
#include "rxb.h"

/* Numero massimo di stringhe in un messaggio */
#define SIMPLE_CSEXP_MAX_TOKENS 16

/* Legge dalla socket fd un messaggio in formato canonical S-expression
 * semplificato, cioè una lista di stringhe senza liste annidate:
 *
 *     (5:hello3:abc)
 *
 * Ogni stringa è preceduta dalla sua lunghezza in byte (in decimale, senza
 * zeri iniziali) e da ':'. L'intero messaggio deve stare nel buffer rxb, che
 * va usato per tutti i messaggi della stessa connessione.
 *
 * In caso di successo *out punta a un array di *out_len stringhe terminate da
 * '\0', allocate dinamicamente: il chiamante deve liberare con free() ogni
 * stringa e poi l'array. Attenzione: se una stringa contiene a sua volta un
 * byte '\0', le funzioni della libreria standard la vedranno troncata.
 *
 * Restituisce un valore >= 0 (il numero di byte ancora presenti nel buffer,
 * appartenenti ai messaggi successivi) in caso di successo, oppure un valore
 * negativo in caso di connessione chiusa, errore o messaggio malformato:
 *   -2       connessione chiusa (o errore) prima dell'inizio del messaggio;
 *   -3, -7   connessione chiusa, errore o messaggio più grande del buffer;
 *   -4       il messaggio non inizia con '(';
 *   -5, -8   lunghezza di una stringa non valida;
 *   -6       memoria esaurita;
 *   -11      più di SIMPLE_CSEXP_MAX_TOKENS stringhe;
 *   -12      stringa più lunga del buffer. */
int simple_csexp_read_message(struct rx_buffer *rxb, int fd, char ***out, size_t *out_len);

#endif /* UNIFE_SIMPLE_CSEXP_H */
