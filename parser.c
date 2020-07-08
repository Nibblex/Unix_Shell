#include <assert.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"

struct parser_s {
    Lexer input;
};

#define SEPARATORS " \t"
#define OP_PIPE "|"
#define OP_BACKGROUND "&"
#define OP_REDIR_IN "<"
#define OP_REDIR_OUT ">"
#define OP_SPECIAL BLANK OP_PIPE OP_BACKGROUND OP_REDIR_IN OP_REDIR_OUT


static bool lex_operator(Lexer input, char *op) {
    bool result = true;

    assert(input != NULL);

    result = !lexer_is_off(input);
    /* Saltear separadores */
    if (result) {
        lexer_skip(input, SEPARATORS);
        result = !lexer_is_off(input);
    }
    /* Buscar operador */
    if (result) {
        lexer_next_char(input, op);
        result = !lexer_is_off(input);
    }
    /* Verificar resultado */
    if (result) {
        result = (blength(lexer_item(input)) == 1);
        bdestroy(lexer_item(input));
    }
    return result;
}


static bstring lex_word(Lexer input) {
    bool success = true;
    bstring result = NULL;

    assert(input != NULL);

    success = !lexer_is_off(input);
    /* Saltear separadores */
    if (success) {
        lexer_skip(input, SEPARATORS);
        success = !lexer_is_off(input);
    }
    /* Buscar argumento */
    if (success) {
        lexer_next_to(input, OP_SPECIAL);
        success = !lexer_is_off(input);
    }
    /* Extraer resultado */
    if (success) {
        result = lexer_item(input);
        if (blength(result) == 0) {
            bdestroy(result); result = NULL;
        }
    }

    return result;
}


typedef enum {
    ARG_NORMAL,
    ARG_INPUT,
    ARG_OUTPUT
} arg_kind_t; /* Tipo auxiliar para parse_argument */

static bstring parse_argument(Lexer input, arg_kind_t *type) {
    if (lex_operator(input, OP_REDIR_IN)) {
        *type = ARG_INPUT;
    } else if (lex_operator(input, OP_REDIR_OUT)) {
        *type = ARG_OUTPUT;
    } else {
        *type = ARG_NORMAL;
    }
    return lex_word(input);
}


static scpipe parse_scommand(Lexer input) {
    scpipe result = scpipe_new(SCOMMAND);
    arg_kind_t type = ARG_NORMAL;
    bstring element = NULL;

    assert(input != NULL);
    element = parse_argument(input, &type);
    if (element == NULL) {
        /* Si el primer elemento es inválido se devuelve NULL */
        /* Esto sucede cuando el comando es vacío, i.e. ""    */
        result = scpipe_destroy(result);
    }
    while (element != NULL) {
        switch (type) {
            case ARG_NORMAL: scpipe_push_back(result, element); break;
            case ARG_INPUT: scommand_set_redir_in(result, element); break;
            case ARG_OUTPUT: scommand_set_redir_out(result, element); break;
        }
        /* Viene otro? */
        element = parse_argument(input, &type);
    }
    if (type != ARG_NORMAL) {
        /* Error de sintaxis. En el último elemento hubo un operador de
         * redirección, pero sin indicar a donde redirigir
         */
        scpipe_destroy(result);
        result = NULL;
    }

    return result;
}


static scpipe parse_pipeline_internal(Lexer input) {
/* Lee todo un scpipe de `input' hasta llegar aun fin de línea o de archivo
 *   devuelve un nuevo scpipe (a liberar por el llamador), o NULL en caso
 *   de error.
 * REQUIRES:
 *     input != NULL
 *     !lexer_is_off(input)
 * ENSURES:
 *     Se consumió input hasta el primer error o hasta completar el scpipe
 *       más largo posible. No se consumió ningun \n.
 *     Si lo que se consumió es un scpipe valido, `result' contiene la
 *       estructura correspondiente.
 */
    scpipe result = scpipe_new(PIPELINE);
    scpipe element = NULL;
    bool another = false, error = false;

    assert(input != NULL);
    assert (!lexer_is_off(input));

    /* Lista de comandos separados por OP_PIPE */
    element = parse_scommand(input);
    error = (element==NULL); /* Comando inválido al empezar */
    while (element != NULL && !error) {
        scpipe_push_back(result, element);
        element = NULL;
        /* Y ahora vemos si sigue un "| <otro comando>" */
        another = lex_operator(input, OP_PIPE);
        if (another) {
            element = parse_scommand(input);
            /* Hay error sii luego del OP_PIPE falta un comando */
            error = (element==NULL);
        }
    }
    /* Opcionalmente un OP_BACKGROUND al final */
    if (lex_operator(input, OP_BACKGROUND)) {
        pipeline_set_wait(result, false);
    }

    /* Tolerancia a espacios posteriores */
    if (!lexer_is_off(input)) {
        lexer_skip(input, SEPARATORS);
    }

    /* Si hubo error, hacemos cleanup */
    if (error) {
        scpipe_destroy(result);
        result = NULL;
    }

    return result;
}


static bool parse_next_line_internal(Lexer input) {
/* Consume el fin de línea. Acepta si hay espacios en blanco antes.
 *   Indica si encontro basura antes del fin de línea.
 * Devuelve true en caso de éxito (se consumió hasta el fin de línea
 * o archivo sin encontrar nada más que blancos). Devuelve false si
 * encuentra algo diferente a espacios, pero igual los consume.
 * REQUIRES:
 *     input != NULL
 * ENSURES:
 *     no se consumió más entrada de la necesaria
 *     el lexer esta detenido justo luego de un \n o en el fin de archivo.
 */
    bool result = true;
    assert(input != NULL);

    if (!lexer_is_off(input))
        lexer_skip(input, SEPARATORS); /* consumimos blancos */

    if (!lexer_is_off(input)) {
        lexer_next_to(input, "\n"); /* Consumimos basura*/
        if (!lexer_is_off(input)) {
            if (blength(lexer_item(input)) > 0 ) {
                result = false; /* Se encontró basura */
            }
            bdestroy(lexer_item(input));
        }
    }
    if (!lexer_is_off(input)) {
        lexer_next_char(input, "\n"); /* Consumir newline */
    }

    return result;
}


Parser parser_new(FILE *input) {
    Parser result = NULL;
    Lexer lexer = NULL;

    assert(input != NULL);

    lexer = lexer_new (input);
    if (lexer != NULL) {
        result = calloc(1, sizeof(struct parser_s));
        assert(result != NULL);
        result->input = lexer;
    }
    return result;
}


Parser parser_destroy(Parser parser) {
    assert(parser != NULL);
    lexer_destroy (parser->input);
    parser->input = NULL;
    free(parser);
    parser = NULL;
    return parser;
}


scpipe parse_pipeline(Parser parser) {
    scpipe pipe = NULL;
    bool garbage = false;

    assert(parser != NULL);

    pipe = parse_pipeline_internal(parser->input);
    garbage = !parse_next_line_internal(parser->input);
    if (garbage && pipe != NULL) {
        scpipe_destroy(pipe);
        pipe = NULL;
    }
    return pipe;
}


bool parser_at_eof(Parser parser) {
    assert(parser != NULL);
    return lexer_is_off(parser->input);
}
