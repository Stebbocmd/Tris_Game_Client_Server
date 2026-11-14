#ifndef UNIFE_RXB_H
#define UNIFE_RXB_H

#include <stdint.h>
#include <sys/types.h>

/* Buffer di ricezione: accumula i byte letti da una socket finché non è
 * disponibile un messaggio completo (per esempio, una linea terminata da
 * '\n'). I byte letti in eccesso restano nel buffer e vengono usati dalla
 * chiamata successiva, quindi per ogni connessione va usato un solo buffer,
 * da inizializzare una sola volta. */
typedef struct rx_buffer {
        uint8_t *buffer;
        /* Queste variabili tracciano rispettivamente la dimensione del buffer
         * e il numero di byte usati */
        size_t size;
        size_t bytes_received;
} rxb_t;

/* Alloca un buffer di buf_size byte. Termina il processo se l'allocazione
 * fallisce. */
void rxb_init(struct rx_buffer *buf, size_t buf_size);

/* Libera la memoria allocata da rxb_init. */
void rxb_destroy(struct rx_buffer *buf);

/* Legge dalla socket sock una linea terminata da '\n' e ne copia il
 * contenuto, senza il '\n', in dest_buf.
 *
 * In ingresso *dest_len indica quanti byte si possono scrivere in dest_buf;
 * in uscita indica quanti byte sono stati effettivamente copiati.
 *
 * ATTENZIONE: la stringa copiata in dest_buf NON viene terminata con '\0'.
 * Lo schema d'uso tipico è quindi:
 *
 *     char linea[MAX_LINE + 1];
 *     size_t linea_len;
 *
 *     memset(linea, 0, sizeof(linea));
 *     linea_len = sizeof(linea) - 1;
 *     if (rxb_readline(&rxb, sd, linea, &linea_len) < 0) {
 *             ... connessione chiusa o errore ...
 *     }
 *
 * Restituisce:
 *    0 se ha letto una linea;
 *   -1 se la connessione è stata chiusa prima di ricevere una linea completa;
 *   -2 in caso di errore di lettura (errno indica il motivo);
 *   -3 se la linea è più lunga del buffer di ricezione o di dest_buf (in
 *      questo caso la connessione andrebbe chiusa, perché non è più possibile
 *      capire dove inizia il messaggio successivo). */
int rxb_readline(struct rx_buffer *buf, int sock, void *dest_buf, size_t *dest_len);

/* Legge dalla socket sock i byte disponibili (al più quanti ne servono a
 * riempire il buffer) e li accoda a quelli già presenti nel buffer.
 *
 * Restituisce il numero di byte letti, oppure:
 *   -1 se la connessione è stata chiusa;
 *   -2 in caso di errore di lettura (errno indica il motivo);
 *   -3 se il buffer è pieno. */
int rxb_read_bytes(struct rx_buffer *buf, int sock);

/* Restituisce la dimensione del buffer. */
size_t rxb_size(const struct rx_buffer *buf);

/* Restituisce il numero di byte presenti nel buffer. */
size_t rxb_available(const struct rx_buffer *buf);

/* Restituisce un puntatore al primo byte presente nel buffer. */
uint8_t *rxb_peek(const struct rx_buffer *buf);

/* Scarta i primi num_bytes byte presenti nel buffer. Restituisce 0, oppure
 * -1 se nel buffer ci sono meno di num_bytes byte. */
int rxb_drop_bytes(struct rx_buffer *buf, size_t num_bytes);

#endif /* UNIFE_RXB_H */
