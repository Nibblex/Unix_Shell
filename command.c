#include "command.h"


struct redir_s {
    bstring in;
    bstring out;
};

union scpipe_u {
    redir_t redir;
    bool wait;
};

struct node_s {
    void *data;
    node_t next;
};

struct scpipe_s {
    node_t first;
    node_t last;
    size_t length;
    selector sel;
    union scpipe_u value;
};


scpipe scpipe_new(selector sel) {
    scpipe self = (scpipe)malloc(sizeof(struct scpipe_s));
    if (!self) return NULL;

    self->first = NULL;
    self->last = NULL;
    self->length = 0;
    self->sel = sel;

    if (sel) {
        self->value.redir.in = NULL;
        self->value.redir.out = NULL;
    } else {
        self->value.wait = true;
    }

    return self;
}

void scpipe_push_back(scpipe self, void *data) {
    assert(self != NULL && data != NULL);
    node_t node = (node_t)malloc(sizeof(struct node_s));
    if (!node) return;

    node->data = data;
    node->next = NULL;

    if (scpipe_is_empty(self))
        self->first = node;
    else
        self->last->next = node;

    self->last = node;
    self->length++;
}

void scpipe_pop_front(scpipe self) {
    assert(self != NULL && !scpipe_is_empty(self));

    if (self->sel)
        bdestroy(scpipe_front(self));
    else
        scpipe_destroy(scpipe_front(self));

    node_t node = self->first;
    self->first = self->first->next;

    if (scpipe_length(self) == 1)
        self->last = NULL;

    free(node);
    node = NULL;
    self->length--;
}

bool scpipe_is_empty(const scpipe self) {
    assert(self != NULL);
    return !self->length;
}

unsigned int scpipe_length(const scpipe self) {
    assert(self != NULL);
    return self->length;
}

void *scpipe_front(const scpipe self) {
    assert(self != NULL && !scpipe_is_empty(self));
    return self->first->data;
}

scpipe scpipe_destroy(scpipe self) {
    assert(self != NULL);

    while (!scpipe_is_empty(self))
        scpipe_pop_front(self);

    if (self->sel) {
        bdestroy(self->value.redir.in);
        bdestroy(self->value.redir.out);
    }

    free(self);
    return (self = NULL);
}

void scommand_set_redir_in(scpipe self, bstring filename) {
    assert(self != NULL);
    bdestroy(self->value.redir.in);
    self->value.redir.in = filename;
}

void scommand_set_redir_out(scpipe self, bstring filename) {
    assert(self != NULL);
    bdestroy(self->value.redir.out);
    self->value.redir.out = filename;
}

const_bstring scommand_get_redir_in(const scpipe self) {
    assert(self != NULL);
    return self->value.redir.in;
}

const_bstring scommand_get_redir_out(const scpipe self) {
    assert(self != NULL);
    return self->value.redir.out;
}

bstring scommand_to_string(const scpipe self) {
    assert(self != NULL);
    bstring result = bfromcstr("");

    node_t curr = self->first;
    for (unsigned int i = 0; i < scpipe_length(self); i++) {
        bconcat(result, curr->data);
        if (i != scpipe_length(self)-1)
            bconchar(result, ' ');
        curr = curr->next;
    }

    if (scommand_get_redir_out(self) != NULL) {
        bconchar(result, '>');
        bconcat(result, scommand_get_redir_out(self));
    }

    if (scommand_get_redir_in(self) != NULL) {
        bconchar(result, '<');
        bconcat(result, scommand_get_redir_in(self));
    }

    return result;
}

void pipeline_set_wait(scpipe self, const bool w) {
    assert(self != NULL);
    self->value.wait = w;
}

bool pipeline_get_wait(const scpipe self) {
    assert(self != NULL);
    return self->value.wait;
}

bstring pipeline_to_string(const scpipe self) {
    assert(self != NULL);
    bstring result = bfromcstr("");

    bstring aux_sc = NULL;
    node_t curr = self->first;
    for (unsigned int i = 0; i < scpipe_length(self); i++) {
        bconcat(result, aux_sc = scommand_to_string(curr->data));
        if (i != scpipe_length(self)-1)
            bconchar(result, '|');
        bdestroy(aux_sc);
        curr = curr->next;
    }

    if (!pipeline_get_wait(self))
        bconchar(result, '&');

    return result;
}