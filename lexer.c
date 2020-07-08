#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "lexer.h"

struct _lexer_t {
    /* Cinta de entrada. Archivo y caracter actual */
    FILE *input;
    int current;
    bool has_current; /* True cuando `current' tiene un caracter preleido */
    /* Token siendo leído */
    bstring item;

    bool own_item; /* true cuando el lexer es dueño de la referencia item */
    /* Indicadores de estado */
    bool is_started, is_off;
};

/*** Internas ***/

#define BASE_ALLOC 16

static void lexer_accept (Lexer self) {
    /* Precondiciones */
    assert (self != NULL);
    assert (!lexer_is_off (self));
    assert (self->has_current);

    bconchar (self->item, self->current);
    /* Leer */
    self->current = fgetc (self->input);

    assert (self->has_current); /* Esta función mantiene esto invariante */
}

static void lexer_lookahead (Lexer self) {
    /* Precondiciones */
    assert (self != NULL);
    assert (!lexer_is_off (self));

    if (! self->has_current) {
        /* Leer */
        self->current = fgetc (self->input);
        self->has_current = true;
    }

    assert (self->has_current); /* Esta función asegura esto siempre */
}

static void lexer_new_item (Lexer self) {
    assert (self != NULL);

    if (self->own_item) {
        btrunc (self->item, 0);
    } else {
        /* No liberamos self->item, alguien más es dueño del puntero. */
        self->item = bfromcstralloc (BASE_ALLOC, "");
    }
    self->own_item = true;
}

/*** Inicialización y destrucción ***/

Lexer lexer_new (FILE *f) {
    /* Nuevo lexer, leyendo de `f' */
    Lexer result = NULL;
    /* Precondiciones */
    assert (f != NULL);
    
    result = calloc (1, sizeof (*result));
    assert (result != NULL);
    
    result->input      = f;
    result->current    = EOF;
    result->has_current= false;
    result->item       = bfromcstralloc (BASE_ALLOC, "");
    result->own_item   = true;
    result->is_off     = false;
    result->is_started = false;

    return result;
}

Lexer lexer_destroy (Lexer self) {
    /* Destruye `self', liberando recursos asociados. No cierra el archivo */
    assert (self != NULL);

    if (self->own_item)
        bdestroy (self->item);
    free (self);
    self = NULL;
    return self;
}

/*** Consultas ***/

bstring lexer_item (Lexer self) {
    /* Última cadena leída por `self'. El puntero devuelto debe ser liberado
     * por el llamador. 
     */
    bstring result = NULL;
    /* Precondiciones */
    assert (self != NULL);
    assert (!lexer_is_off (self));
    assert (lexer_is_started (self));

    self->own_item = false;
    result = self->item;

    /* Postcondiciones*/
    assert (result != NULL);

    return result;
}

bool lexer_is_started (const Lexer self) {
    /* `self' empezó a leer el archivo? */
    /* Precondiciones */
    assert (self != NULL);

    return self->is_started;
}

bool lexer_is_off (const Lexer self) {
    /* `self' terminó de leer el archivo? */
    /* Precondiciones */
    assert (self != NULL);

    return self->is_off;
}

/*** Operaciones ***/

void lexer_next (Lexer self, const char *charset) {
    /* Lee con `self' caracteres pertenecientes a `charset', tantos como sea
     *  posible. Deja el resultado accesible en lexer_item (self)
     */
    /* Precondiciones */
    assert (self != NULL);
    assert (charset != NULL);

    lexer_new_item (self);
    lexer_lookahead(self);
    while (self->current != EOF && strchr (charset, self->current)) {
        lexer_accept (self);
    }
    self->is_started = true;
    assert (self->has_current);
    self->is_off = (self->current==EOF && blength (self->item)==0);
    /* Postcondiciones */
    assert (lexer_is_started (self));
}

void lexer_next_to (Lexer self, const char *charset) {
    /* Lee con `self' caracteres no pertenecientes a `charset', tantos como sea
     *  posible. Deja el resultado accesible en lexer_item (self)
     */
    /* Precondiciones */
    assert (self != NULL);
    assert (charset != NULL);

    lexer_new_item (self);
    lexer_lookahead(self);
    while (self->current != EOF && !strchr (charset, self->current)) {
        lexer_accept (self);
    }
    self->is_started = true;
    assert (self->has_current);
    self->is_off = (self->current==EOF && blength (self->item)==0);

    /* Postcondiciones */
    assert (lexer_is_started (self));
}

void lexer_next_char (Lexer self, const char *charset) {
    assert (self != NULL);
    assert (charset != NULL);

    lexer_new_item (self);
    lexer_lookahead(self);
    if (self->current != EOF && strchr (charset, self->current)) {
        bconchar (self->item, self->current);
        self->has_current = false;
    }

    self->is_started = true;
    assert (blength (self->item)!=0 || self->has_current);
    self->is_off = (blength (self->item)==0 && self->current==EOF);

    assert (lexer_is_started (self));
}

/* Operaciones extra */
void lexer_skip (Lexer self, const char *charset) {
    lexer_next (self, charset);
    lexer_new_item (self);
}

void lexer_skip_to (Lexer self, const char *charset) {
    lexer_next_to (self, charset);
    lexer_new_item (self);
}

