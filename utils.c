#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* For sendfile */
#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/socket.h>
#include <sys/uio.h>
#elif defined(__linux__)
#include <sys/sendfile.h>
#endif

#include "utils.h"


ssize_t write_all(int fd, const void *vptr, size_t n)
{
        size_t remaining;
        ssize_t cc;
        const uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = write(fd, ptr, remaining)) <= 0) {
                        if (cc < 0 && errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                }

                remaining -= cc;
                ptr += cc;
        }
        return n;
}

ssize_t read_all(int fd, void *vptr, size_t n)
{
        size_t remaining;
        ssize_t cc;
        uint8_t *ptr;

        ptr = vptr;
        remaining = n;
        while (remaining > 0) {
                if ((cc = read(fd, ptr, remaining)) < 0) {
                        if (errno == EINTR)
                                cc = 0;
                        else
                                return -1;
                } else if (cc == 0) /* EOF */
                        break;

                remaining -= cc;
                ptr += cc;
        }
        return (n - remaining);
}


/* sendfile(2) è una system call non standard, che ha un formato diverso sotto
 * Linux, sotto FreeBSD e sotto macOS (che ricalca FreeBSD, ma con alcune
 * differenze) e non esiste affatto in altri sistemi, come OpenBSD. Questa
 * funzione ne fornisce una versione più semplice ma portabile, che manda
 * sempre tutto il contenuto del file a partire dall'inizio.
 *
 * In tutti i casi sendfile(2) potrebbe inviare solo una parte dei dati (per
 * esempio, se interrotta da un segnale), quindi va chiamata in un ciclo. */
int portable_sendfile(int fd, int ns)
{
#if defined(__linux__)
        /* Caso Linux: sendfile aggiorna off con la posizione raggiunta */
        struct stat st;
        off_t off = 0;

        if (fstat(fd, &st) < 0)
                return -1;

        while (off < st.st_size) {
                ssize_t cc = sendfile(ns, fd, &off, st.st_size - off);
                if (cc < 0) {
                        if (errno == EINTR)
                                continue;
                        return -1;
                }
                if (cc == 0) /* il file è stato accorciato nel frattempo */
                        break;
        }
        return 0;
#elif defined(__APPLE__)
        /* Caso macOS: len = 0 significa "fino alla fine del file"; in uscita
         * len contiene il numero di byte inviati */
        off_t off = 0;

        while (1) {
                off_t len = 0;
                int err = sendfile(fd, ns, off, &len, NULL, 0);
                off += len;
                if (err == 0)
                        return 0;
                if (errno != EINTR && errno != EAGAIN)
                        return -1;
        }
#elif defined(__FreeBSD__)
        /* Caso FreeBSD: nbytes = 0 significa "fino alla fine del file"; in
         * uscita sbytes contiene il numero di byte inviati */
        off_t off = 0;

        while (1) {
                off_t sbytes = 0;
                int err = sendfile(fd, ns, off, 0, NULL, &sbytes, 0);
                off += sbytes;
                if (err == 0)
                        return 0;
                if (errno != EINTR && errno != EAGAIN)
                        return -1;
        }
#else
        /* Non siamo né sotto Linux né sotto FreeBSD o macOS (per esempio
         * siamo sotto OpenBSD, che non ha sendfile): non usiamo sendfile(2)
         * ma un ciclo di read da file e write su socket. Decisamente meno
         * efficiente, ma tant'è... */
        uint8_t buffer[4096];
        ssize_t nread;

        if (lseek(fd, 0, SEEK_SET) < 0)
                return -1;

        while (1) {
                nread = read(fd, buffer, sizeof(buffer));
                if (nread < 0) {
                        if (errno == EINTR)
                                continue;
                        return -1;
                }
                if (nread == 0) /* EOF */
                        return 0;
                if (write_all(ns, buffer, nread) < 0)
                        return -1;
        }
#endif
}
