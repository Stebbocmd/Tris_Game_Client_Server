#ifndef UNIFE_UTILS_H
#define UNIFE_UTILS_H

#include <sys/types.h>

/* Scrive su fd tutti gli n byte puntati da vptr, ripetendo la write finché
 * necessario. Restituisce n, oppure -1 in caso di errore (errno indica il
 * motivo). */
ssize_t write_all(int fd, const void *vptr, size_t n);

/* Legge da fd esattamente n byte e li copia in vptr, ripetendo la read finché
 * necessario. Restituisce il numero di byte letti, che è minore di n se la
 * connessione è stata chiusa prima, oppure -1 in caso di errore (errno indica
 * il motivo). */
ssize_t read_all(int fd, void *vptr, size_t n);

/* Invia sulla socket ns l'intero contenuto del file aperto fd, dall'inizio
 * del file e indipendentemente dalla posizione corrente in fd. Usa la system
 * call sendfile(2) su Linux, FreeBSD e macOS e un ciclo di read/write sugli
 * altri sistemi (per esempio OpenBSD). Restituisce 0, oppure -1 in caso di
 * errore (errno indica il motivo). */
int portable_sendfile(int fd, int ns);

#endif /* UNIFE_UTILS_H */
