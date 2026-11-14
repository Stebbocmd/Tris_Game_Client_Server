#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rxb.h"
#include "simple_csexp.h"

typedef enum {
        START_MESSAGE,
        READ_LENGTH_START,
        READ_LENGTH_CONTINUE
} parser_state_t;

int simple_csexp_read_message(struct rx_buffer *rxb, int fd, char ***out, size_t *out_len)
{
        parser_state_t ps = START_MESSAGE;
        size_t string_index = 0;
        /* length of the next string, computed digit by digit */
        size_t next_string_length = 0;
        char *c;
        size_t bytes_left;
        /* number of bytes of the buffer already processed: the invariants
         * c == rxb_peek(rxb) + processed and
         * bytes_left == rxb_available(rxb) - processed always hold */
        size_t processed = 0;
        int ret = -10;
        char **res;

        res = calloc(SIMPLE_CSEXP_MAX_TOKENS, sizeof(char *));
        if (res == NULL) {
                perror("calloc");
                exit(EXIT_FAILURE);
        }

        /* read some data */
        if (rxb_available(rxb) == 0) {
                int cc = rxb_read_bytes(rxb, fd);
                if (cc <= 0) {
                        /* I/O error or EOF */
                        ret = -2;
                        goto CLEANUP;
                }
        }
        /* initialize c and bytes_left */
        c = (char *)rxb_peek(rxb);
        bytes_left = rxb_available(rxb);

        while (1) {
                /* read more data if needed */
                if (bytes_left == 0) {
                        int cc = rxb_read_bytes(rxb, fd);
                        if (cc <= 0) {
                                /* I/O error or EOF */
                                ret = -3;
                                goto CLEANUP;
                        }
                        /* reinitialize c and bytes_left: the buffer still
                         * contains the bytes we already processed (we drop
                         * them only at the end of the message), so we must
                         * restart from the first unprocessed byte */
                        c = (char *)rxb_peek(rxb) + processed;
                        bytes_left = rxb_available(rxb) - processed;
                }

                switch(ps) {
                        case START_MESSAGE:
                                if (*c != '(') {
                                        ret = -4;
                                        goto CLEANUP;
                                }

                                ps = READ_LENGTH_START;
                                break;

                        case READ_LENGTH_START:
                                if (*c == ')') {
                                        /* drop the bytes in the buffer that we already used */
                                        rxb_drop_bytes(rxb, processed + 1);

                                        /* set out and out_len */
                                        *out = res;
                                        *out_len = string_index;

                                        /* we finished reading the message;
                                         * let's return the number of
                                         * unprocessed characters left in the
                                         * buffer */
                                        return (int)rxb_available(rxb);
                                } else if (*c < '1' || *c > '9') { /* first digit can't be 0 */
                                        /* not a digit */
                                        ret = -5;
                                        goto CLEANUP;
                                }

                                /* make sure we have room for another string */
                                if (string_index >= SIMPLE_CSEXP_MAX_TOKENS) {
                                        ret = -11;
                                        goto CLEANUP;
                                }

                                /* first digit of the string length */
                                next_string_length = (size_t)(*c - '0');

                                ps = READ_LENGTH_CONTINUE;
                                break;

                        case READ_LENGTH_CONTINUE:
                                if (*c == ':') {
                                        char *string;

                                        /* make sure we have enough data in the buffer */
                                        while (bytes_left < next_string_length + 1) {
                                                int cc = rxb_read_bytes(rxb, fd);
                                                if (cc <= 0) {
                                                        /* I/O error, EOF, or
                                                         * message larger than
                                                         * the buffer */
                                                        ret = -7;
                                                        goto CLEANUP;
                                                }
                                                bytes_left += cc;
                                        }

                                        /* extract string */
                                        string = calloc(1, next_string_length + 1);
                                        if (string == NULL) {
                                                ret = -6;
                                                goto CLEANUP;
                                        }
                                        memcpy(string, c + 1, next_string_length);
                                        res[string_index++] = string;

                                        /* update counters */
                                        c += next_string_length;
                                        bytes_left -= next_string_length;
                                        processed += next_string_length;

                                        /* reset string length */
                                        next_string_length = 0;

                                        ps = READ_LENGTH_START;
                                        break;
                                } else if (*c < '0' || *c > '9') {
                                        /* not a digit */
                                        ret = -8;
                                        goto CLEANUP;
                                }

                                /* add digit to the string length */
                                next_string_length = next_string_length * 10 + (size_t)(*c - '0');

                                /* a string larger than the receive buffer
                                 * could never be read, so we reject it right
                                 * away (this also prevents overflows of
                                 * next_string_length) */
                                if (next_string_length > rxb_size(rxb)) {
                                        ret = -12;
                                        goto CLEANUP;
                                }

                                ps = READ_LENGTH_CONTINUE;
                                break;

                        default:
                                /* unrecognized state */
                                ret = -9;
                                goto CLEANUP;
                }
                /* update counters */
                processed += 1; ++c; --bytes_left;
        }

CLEANUP:
        /* free all allocated strings */
        while (string_index > 0) {
                free(res[--string_index]);
        }
        free(res);
        *out = NULL; *out_len = 0;
        return ret;
}
