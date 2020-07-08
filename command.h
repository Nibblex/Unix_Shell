#ifndef COMMAND_H
#define COMMAND_H

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#include "bstring/bstrlib.h"


typedef enum {
    PIPELINE = 0,
    SCOMMAND = 1
} selector;

typedef struct node_s * node_t;

typedef struct redir_s redir_t;

typedef struct scpipe_s * scpipe;


/* SCPIPE OPS */
scpipe scpipe_new(selector sel);

void scpipe_push_back(scpipe self, void *data);

void scpipe_pop_front(scpipe self);

bool scpipe_is_empty(const scpipe self);

unsigned int scpipe_length(const scpipe self);

void *scpipe_front(const scpipe self);

scpipe scpipe_destroy(scpipe self);


/* SCOMMAND OPS */
void scommand_set_redir_in(scpipe self, bstring filename);

void scommand_set_redir_out(scpipe self, bstring filename);

const_bstring scommand_get_redir_in(const scpipe self);

const_bstring scommand_get_redir_out(const scpipe self);

bstring scommand_to_string(const scpipe self);


/* PIPELINE OPS */
void pipeline_set_wait(scpipe self, const bool w);

bool pipeline_get_wait(const scpipe self);

bstring pipeline_to_string(const scpipe self);

#endif