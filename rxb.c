#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "rxb.h"

void rxb_init(struct rx_buffer *buf, size_t buf_size)
{
        buf->buffer = malloc(buf_size);
        if (buf->buffer == NULL) {
                perror("malloc");
                exit(EXIT_FAILURE);
        }
        buf->bytes_received = 0;
        buf->size = buf_size;
}

void rxb_destroy(struct rx_buffer *buf)
{
        free(buf->buffer);
        buf->buffer = NULL;
        buf->bytes_received = 0;
        buf->size = 0;
}

int rxb_readline(struct rx_buffer *buf, int sock, void *dest_buf, size_t *dest_len)
{
        uint8_t *dest = dest_buf;
        uint8_t *term_ptr = NULL;

        /* Leggo i dati in un buffer */
        do {
                int cc;

                /* Controllo se ho ricevuto il terminatore '\n' */
                term_ptr = memchr(buf->buffer, '\n', buf->bytes_received);
                if (term_ptr != NULL) {
                        /* Calcolo quanti dati devo copiare */
                        size_t to_copy = term_ptr - buf->buffer;

                        /* Verifico che dest sia sufficientemente grande */
                        if (*dest_len < to_copy) {
                                return -3;
                        }

                        /* Copio dati da buffer a dest e aggiorno dest_len */
                        memcpy(dest, buf->buffer, to_copy);
                        *dest_len = to_copy;

                        /* Scarto dal buffer la linea appena copiata e il
                         * terminatore '\n' */
                        rxb_drop_bytes(buf, to_copy + 1);

                        return 0;
                }

                /* Non ho ancora una linea completa: leggo altri dati. Se il
                 * buffer è pieno, il peer ha inviato una linea troppo lunga */
                cc = rxb_read_bytes(buf, sock);
                if (cc < 0) {
                        return cc;
                }
        } while (1);
}

int rxb_read_bytes(struct rx_buffer *buf, int sock)
{
        size_t left = buf->size - buf->bytes_received;
        ssize_t cc;

        if (left == 0) {
                /* Buffer pieno */
                return -3;
        }

        /* Se la read viene interrotta da un segnale (per esempio SIGCHLD) la
         * ripeto */
        do {
                cc = read(sock, buf->buffer + buf->bytes_received, left);
        } while (cc < 0 && errno == EINTR);

        if (cc == 0)
                return -1;
        if (cc < 0)
                return -2;

        buf->bytes_received += cc;
        return (int)cc;
}

size_t rxb_size(const struct rx_buffer *buf)
{
        return buf->size;
}

size_t rxb_available(const struct rx_buffer *buf)
{
        return buf->bytes_received;
}

uint8_t *rxb_peek(const struct rx_buffer *buf)
{
        return buf->buffer;
}

int rxb_drop_bytes(struct rx_buffer *buf, size_t num_bytes)
{
        if (num_bytes > buf->bytes_received) {
                /* nonsense request */
                return -1;
        }

        memmove(buf->buffer, buf->buffer + num_bytes, buf->bytes_received - num_bytes);
        buf->bytes_received -= num_bytes;

        return 0;
}
