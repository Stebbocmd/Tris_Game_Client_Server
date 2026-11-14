# ======================================================================
# Makefile per gli esercizi basati sul framework libcnunife
#
# Uso (vedi README.txt per i dettagli):
#
#   make            compila in modalità debug (equivale a "make debug")
#   make debug      compila con simboli di debug e sanitizer
#   make release    compila con le ottimizzazioni
#   make analyze    compila eseguendo l'analisi statica del codice
#   make clean      cancella tutti i file generati
#   make dist       crea un archivio zip con il contenuto della cartella
#
# Il Makefile richiede GNU make: su FreeBSD e OpenBSD, dove make è un
# programma diverso, va installato (pacchetto gmake) e usato al suo posto,
# per esempio "gmake release".
#
# Gli eseguibili vengono creati nella cartella build/<modalità>/, per
# esempio build/debug/server.
#
# Non serve modificare questo Makefile: ogni file .c della cartella che
# contiene la funzione main diventa un eseguibile con lo stesso nome (es.
# server.c -> build/debug/server). Gli altri file .c sono moduli (quelli del
# framework ed eventuali moduli dell'esercizio) e vengono collegati a tutti
# i programmi. Ogni file .java diventa un file .class.
# ======================================================================


# ----------------------------------------------------------------------
# Sorgenti
# ----------------------------------------------------------------------

# Programmi: i file .c che contengono una riga che inizia con "int main("
# (/dev/null serve a non bloccare grep se nella cartella non ci sono file .c)
MAIN_REGEX := ^[[:space:]]*int[[:space:]]+main[[:space:]]*\(
PROGRAMS   := $(basename $(shell grep -lE '$(MAIN_REGEX)' $(wildcard *.c) /dev/null))

# Moduli: tutti gli altri file .c
MODULES := $(filter-out $(PROGRAMS),$(basename $(wildcard *.c)))

# Programmi Java
JAVA_CLASSES := $(patsubst %.java,%.class,$(wildcard *.java))

# File oggetto dei moduli (vedi sotto BUILD_DIR)
MODULE_OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(MODULES)))

# Nome dell'archivio creato da "make dist", ricavato dal nome della cartella:
# "1 - Remote Head" -> remote_head
NAME    := $(shell basename "$(CURDIR)" | sed -e 's/^[0-9]* - //' -e 's/ /_/g' | tr '[:upper:]' '[:lower:]')
ARCHIVE := $(NAME)_$(shell date +%Y%m%d).zip


# ----------------------------------------------------------------------
# Modalità di compilazione: debug (default), release o analyze
# ----------------------------------------------------------------------

MODE ?= debug

BUILD_DIR := build/$(MODE)

# Riconosco il sistema operativo (Linux, Darwin per macOS, FreeBSD,
# OpenBSD, ...)
OS := $(shell uname -s)

# Riconosco il compilatore: su macOS, FreeBSD e OpenBSD cc è clang (su macOS
# anche gcc!), su Linux di solito è gcc
ifneq ($(findstring clang,$(shell $(CC) --version 2>/dev/null)),)
  COMPILER := clang
else
  COMPILER := gcc
endif

# Opzioni comuni a tutte le modalità. _DEFAULT_SOURCE serve su Linux per
# rendere visibili le funzioni POSIX (getaddrinfo, sigaction, ...), che
# l'opzione -std=c17 nasconderebbe.
CPPFLAGS := -D_DEFAULT_SOURCE
CFLAGS   := -std=c17 -Wall -Wextra -Wpedantic -Wformat=2 -Werror=format-security
LDFLAGS  :=
LDLIBS   :=

ifeq ($(MODE),debug)
  CFLAGS  += -O1 -g3 -fno-omit-frame-pointer
  # AddressSanitizer e UndefinedBehaviorSanitizer interrompono il programma
  # al primo accesso a memoria non valida o comportamento indefinito. OpenBSD
  # non li supporta.
  ifneq ($(OS),OpenBSD)
    CFLAGS  += -fsanitize=address,undefined
    LDFLAGS += -fsanitize=address,undefined
  endif
else ifeq ($(MODE),release)
  CFLAGS  += -O2 -DNDEBUG
else ifeq ($(MODE),analyze)
  # Con gcc l'analisi statica è svolta dal compilatore stesso; con clang la
  # svolge scan-build (vedi sotto il target analyze)
  CFLAGS  += -O0 -g
  ifeq ($(COMPILER),gcc)
    CFLAGS += -fanalyzer
  endif
else
  $(error Modalità "$(MODE)" sconosciuta: usare debug, release o analyze)
endif


# ----------------------------------------------------------------------
# Supporto opzionale a libunistring, da attivare con:
#
#   make USE_LIBUNISTRING=yes
# ----------------------------------------------------------------------

ifeq ($(USE_LIBUNISTRING),yes)
  CPPFLAGS += -DUSE_LIBUNISTRING=1
  LDLIBS   += -lunistring

  # Su macOS libunistring si installa con Homebrew, su FreeBSD e OpenBSD con
  # il sistema dei pacchetti: in entrambi i casi in cartelle che il
  # compilatore non conosce
  ifneq ($(filter FreeBSD OpenBSD,$(OS)),)
    PKG_PREFIX := /usr/local
  else
    PKG_PREFIX := $(shell brew --prefix 2>/dev/null)
  endif
  ifneq ($(PKG_PREFIX),)
    CPPFLAGS += -I$(PKG_PREFIX)/include
    LDFLAGS  += -L$(PKG_PREFIX)/lib
  endif
endif


# ----------------------------------------------------------------------
# Target principali
# ----------------------------------------------------------------------

.PHONY: all debug release analyze clean dist

# Compilo anche i moduli del framework, così "make" è utile anche nella
# cartella libcnunife, che non contiene programmi
all: $(MODULE_OBJS) $(addprefix $(BUILD_DIR)/,$(PROGRAMS)) $(JAVA_CLASSES)

debug:
	@$(MAKE) --no-print-directory MODE=debug

release:
	@$(MAKE) --no-print-directory MODE=release

# Ricompilo sempre tutto da zero, altrimenti l'analisi verrebbe ripetuta solo
# sui file modificati. scan-build è nel PATH su Linux; su macOS si installa
# con "brew install llvm", su FreeBSD con il pacchetto llvm (che lo mette in
# /usr/local/llvmNN/bin).
SCAN_BUILD ?= $(firstword $(shell command -v scan-build) \
                          $(wildcard /opt/homebrew/opt/llvm/bin/scan-build \
                                     /usr/local/opt/llvm/bin/scan-build \
                                     /usr/local/llvm*/bin/scan-build))

analyze:
	rm -rf build/analyze
ifeq ($(COMPILER),clang)
	@test -n "$(SCAN_BUILD)" || { echo "scan-build non trovato: vedere README.txt"; exit 1; }
	$(SCAN_BUILD) -o build/analyze/report $(MAKE) --no-print-directory MODE=analyze
else
	@$(MAKE) --no-print-directory MODE=analyze
endif


# ----------------------------------------------------------------------
# Compilazione C
# ----------------------------------------------------------------------

# Da foo.c a build/<modalità>/foo.o. Le opzioni -MMD -MP generano il file
# build/<modalità>/foo.d con l'elenco degli header inclusi da foo.c, così
# make sa che deve ricompilare foo.c quando uno di essi cambia.
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

# Ogni programma è collegato a tutti i moduli del framework
$(addprefix $(BUILD_DIR)/,$(PROGRAMS)): $(BUILD_DIR)/%: $(BUILD_DIR)/%.o $(MODULE_OBJS)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

-include $(wildcard $(BUILD_DIR)/*.d)


# ----------------------------------------------------------------------
# Compilazione Java
# ----------------------------------------------------------------------

%.class: %.java
	javac $<


# ----------------------------------------------------------------------
# Pulizia e distribuzione
# ----------------------------------------------------------------------

clean:
	rm -rf build *.class *.zip *~

# L'archivio contiene tutta la cartella tranne i file generati. zip segue i
# link simbolici, quindi l'archivio contiene i file veri e propri.
dist:
	zip -r $(ARCHIVE) . -x 'build/*' '*.class' '*.zip' '*.o' '*.d' '*~' '.*'
