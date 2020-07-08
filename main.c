#include "execute.h"
#include "parser.h"


int gethostname(char *name, size_t len);
void print_prompt(void);

void print_prompt(void) {
    char* cwd;
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    printf("%s@%s:%s$", getenv("USER"), hostname, cwd = getcwd(NULL, 0));
    free(cwd);
}

int main(void){
    Parser pars = parser_new(stdin);
    scpipe apipe = NULL;
    system("clear");
    while (!parser_at_eof(pars)) {
        waitpid(-1, NULL, WNOHANG);
        print_prompt();
        if ((apipe = parse_pipeline(pars)) != NULL) {
            execute_pipeline(apipe);
            scpipe_destroy(apipe);
        }
    }
    parser_destroy(pars);
    printf("\n");
    return 0;
}