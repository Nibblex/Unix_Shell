#ifndef _LEXER_H
#define _LEXER_H

#include <stdio.h>   /* Para tener FILE */
#include <stdbool.h> /* Para tener bool */

#include "bstring/bstrlib.h"

/*** Constantes ***/
#define UPPER "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define LOWER "abcdefghijklmnopqrstuvwxyz"
#define DIGIT "0123456789"
#define BLANK "\t\n\r "
#define ALPHA UPPER LOWER
#define ALPHANUMERIC UPPER LOWER DIGIT

/* Tipo opaco */
typedef struct _lexer_t * Lexer;

/*** Inicialización y destrucción ***/
Lexer lexer_new(FILE *f);
    /* Nuevo lexer, leyendo de `f'
     *     f está abierto
     * ENSURES:
     *     Result != NULL
     *     ! lexer_is_started (Result)
     */

Lexer lexer_destroy(Lexer self);
    /* Destruye `self', liberando recursos asociados. No cierra el archivo
     * REQUIRES:
     *     self != NULL
     * ENSURES:
     *     Result == NULL
     */

/*** Consultas ***/

/* Nota: todo side-effect free */
bstring lexer_item(Lexer self);
    /* Última cadena leída por `self'. El puntero devuelto debe ser liberado
     * por el llamador. 
     * REQUIRES:
     *     self != NULL
     *     !lexer_is_off (self)
     *     lexer_is_started (self)
     * ENSURES:
     *     Result != NULL
     */

bool lexer_is_started(const Lexer self);
    /* `self' empezó a leer el archivo?
     * REQUIRES: self != NULL
     */

bool lexer_is_off(const Lexer self);
    /* `self' terminó de leer el archivo?
     * REQUIRES: self != NULL
     */

/*** Operaciones ***/

void lexer_next(Lexer self, const char *charset);
    /* Lee con `self' caracteres pertenecientes a `charset', tantos como sea
     *  posible. Deja el resultado accesible en lexer_item (self)
     * REQUIRES:
     *     self != NULL
     *     charset != NULL
     * ENSURES:
     *     lexer_is_started (self)
     *     lexer_is_off (self) ||
     *     (lexer_item (self) != NULL &&
     *       todos los caracteres de lexer_item (self) están en charset
     *       el próximo avance va a tomar un caracter fuera de charset)
     */

void lexer_next_to(Lexer self, const char *charset);
    /* Lee con `self' caracteres no pertenecientes a `charset', tantos como sea
     *  posible. Deja el resultado accesible en lexer_item (self)
     * REQUIRES:
     *     self != NULL
     *     charset != NULL
     * ENSURES:
     *     lexer_is_started (self)
     *     lexer_is_off (self) ||
     *     (lexer_item (self) != NULL &&
     *       ningún caracter de lexer_item (self) está en charset
     *       el próximo avance va a tomar un caracter en charset)
     */

void lexer_next_char(Lexer self, const char *charset);
    /* Lee con `self' hasta 1 caracter pertenecientes a `charset'. Deja el
     * resultado accesible en lexer_item (self). No consume la cinta original
     * más allá del caracter leído en caso de leer algo.
     * REQUIRES:
     *     self != NULL
     *     charset != NULL
     * ENSURES:
     *     lexer_is_started (self)
     *     lexer_is_off (self) ||
     *     (lexer_item (self) != NULL &&
     *       blength (lexer_item (self))<=1 &&
     *       el primer caracter de lexer_item (self) está en charset)
     */

/* Operaciones extra */
void lexer_skip(Lexer self, const char *charset);
    /* Lee con `self' caracteres pertenecientes a `charset', tantos como sea
     *  posible. Los caracteres leidos se pierden.
     * REQUIRES:
     *     self != NULL
     *     charset != NULL
     * ENSURES:
     *     lexer_is_started (self)
     *     lexer_is_off (self) || biseqcstr (lexer_item (self), "")
     */

void lexer_skip_to(Lexer self, const char *charset);
    /* Lee con `self' caracteres no pertenecientes a `charset', tantos como sea
     *  posible. Los caracteres leidos se pierden.
     * REQUIRES:
     *     self != NULL
     *     charset != NULL
     * ENSURES:
     *     lexer_is_started (self)
     *     lexer_is_off (self) || biseqcstr (lexer_item (self), "")
     */

#endif
