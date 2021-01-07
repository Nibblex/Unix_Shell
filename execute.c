#include "execute.h"


static int panic(int n, const char * err_msg) {
    if (n < 0) {
        perror(err_msg);
        exit(EXIT_FAILURE);
    }
    return n;
}

static void sc2array(char** sc_array, scpipe apipe) {
    unsigned int k = 0;
    do {
        sc_array[k++] = bstr2cstr(scpipe_front(scpipe_front(apipe)), '\0');
        scpipe_pop_front(scpipe_front(apipe));
    } while (!scpipe_is_empty(scpipe_front(apipe)));
    sc_array[k] = NULL;
}

void execute_pipeline(scpipe apipe) {
    if (scpipe_is_empty(apipe))
        return;

    const unsigned int pipeLen = scpipe_length(apipe);
    const unsigned int numPipes = pipeLen-1;
    pid_t pid[pipeLen];
    int file, pipefds[2*pipeLen];

    if (builtin_index(apipe) != BUILTIN_UNKNOWN) { // Checks for internal commands
        builtin_run(apipe);
        return;
    }

    for (unsigned int i = 0; i < numPipes; i++) // Create all necesary pipes
        panic(pipe(&pipefds[i*2]), "pipe");

    unsigned int j = 0, k = 0;
    while (!scpipe_is_empty(apipe)) {
        const unsigned int scLen = scpipe_length(scpipe_front(apipe));
        char** sc_array = (char**)calloc(scLen+1, sizeof(char*));
        const_bstring in = scommand_get_redir_in(scpipe_front(apipe));
        const_bstring out = scommand_get_redir_out(scpipe_front(apipe));
        sc2array(sc_array, apipe); // Copy an sc into an array
        switch (panic(pid[k++] = fork(), "fork")) {
            case 0: // Child process
                if (in) { // sc has redir in
                    panic(file = open(bstr2cstr(in, '\0'), O_RDONLY, 0), "open in");
                    panic(dup2(file, 0), "dup2 in");
                    panic(close(file), "close");
                } else
                    if (scpipe_length(apipe) != pipeLen) // Not first sc
                        panic(dup2(pipefds[j-2], 0), "dup2 in");

                if (out) { // sc has redir out
                    panic(file = open(bstr2cstr(out, '\0'), O_WRONLY|O_CREAT|O_TRUNC, 0640), "open out");
                    panic(dup2(file, 1), "dup2 out");
                    panic(close(file), "close");
                } else
                    if (scpipe_length(apipe) > 1) // Not last sc
                        panic(dup2(pipefds[j+1], 1), "dup2 out");

                for (unsigned int i = 0; i < 2*numPipes; i++) // Close child's pipes
                    panic(close(pipefds[i]), "close");

                panic(execvp(sc_array[0], sc_array), "execvp"); // Execute command
                panic(close(file), "close");
                break;
            default: // Parent process
                for (unsigned int i = 0; i < scLen; i++) // sc_array memory release
                    bcstrfree(sc_array[i]);
                free(sc_array);
                scpipe_pop_front(apipe); // Pop the current sc from the pipe
                break;
        }
        j+=2;
    }

    for (unsigned int i = 0; i < 2*numPipes; i++) // Close all parent's pipes
        panic(close(pipefds[i]), "close");

    if (pipeline_get_wait(apipe)) // Wait for child process termination
        for (unsigned int i = 0; i < pipeLen; i++)
            panic(waitpid(pid[i], NULL, 0), "waitpid");
}