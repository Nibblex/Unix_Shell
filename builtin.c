#include "builtin.h"


cmd_id builtin_index(const scpipe pipe) {
    cmd_id id;
    bstring exit = bfromcstr("exit"), cd = bfromcstr("cd");

    if (!bstrcmp(scpipe_front(scpipe_front(pipe)), exit))
        id = BUILTIN_EXIT;
    else if (!bstrcmp(scpipe_front(scpipe_front(pipe)), cd))
        id = BUILTIN_CHDIR;
    else
        id = BUILTIN_UNKNOWN;

    bdestroy(exit);
    bdestroy(cd);
    return id;
}

void builtin_run(const scpipe pipe) {
    cmd_id id = builtin_index(pipe);
    if (id == BUILTIN_EXIT)
        exit(EXIT_SUCCESS);
    if (id == BUILTIN_CHDIR) {
        char* PATH;
        if (scpipe_length(scpipe_front(pipe)) > 1) {
            scpipe_pop_front(scpipe_front(pipe));
            if (chdir(PATH = bstr2cstr(scpipe_front(scpipe_front(pipe)), '\0')) < 0)
                perror("baash: cd: No existe el fichero o el directorio\n");
            bcstrfree(PATH);
        } else
            perror("baash: cd: Faltan argumentos\n");
    }
}