#include <check.h>
#include <signal.h>
#include "test_execute.h"

#include "syscall_mock.h"
#include "execute.h"


/* Precondiciones */

START_TEST (test_pipeline_null)
{
	execute_pipeline(NULL);
}
END_TEST

/* Todos los tests van a ejecutar este scpipe; ponemos codigo genérico de
 * setup 
 */
static scpipe test_pipe = NULL;

static void setup (void) {
	mock_reset_all ();
	test_pipe = scpipe_new (PIPELINE);
}

static void teardown (void) {
	scpipe_destroy (test_pipe);
	test_pipe=NULL;
}

/* Funcionalidad */

START_TEST (test_null)
{
	/* Ejecuta un comando nulo (scpipe vacío) */
	execute_pipeline (test_pipe);
	/* Esto no debería haber tratado de crear ni destruir procesos */
	fail_unless (mock_counter_fork==0, NULL);
	fail_unless (mock_counter_execvp==0, NULL);
	fail_unless (mock_counter_exit==0, NULL);
}
END_TEST

START_TEST (test_builtin_exit)
{
	/* Ejecuta un exit,que debería pasar a una sola syscall de exit sin crear
	 * ni destruir procesos
	 */
	scpipe exit_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (exit_cmd, bfromcstr ("exit"));
	scpipe_push_back (test_pipe, exit_cmd);

	EXIT_PROTECTED(
		execute_pipeline (test_pipe);
	);
	/* Esto no debería haber tratado de crear ni ejecutar procesos */
	fail_unless (mock_counter_fork==0, NULL);
	fail_unless (mock_counter_execvp==0, NULL);
	/* Acá hay dos opciones: el contador de exit es 0, ó es 1. Esto depende
	 * de como se implemente el exit. Se puede llamar a exit() de prepo desde
	 * el módulo de ejecución, en cuyo caso el contador es 1. Pero también
	 * se puede no hacer nada, y que el ciclo principal de baash haga el
	 * corte; esto permite un cleanup más ordenado, porque el módulo de más
	 * arriba puede liberar memoria, cerrar recursos, etc.
	 */
	fail_unless (mock_counter_exit<=1, NULL);
}
END_TEST

START_TEST (test_builtin_chdir)
{
	/* Ejecuta un chdir,que debería pasar a una sola syscall de chdir sin crear
	 * ni destruir procesos
	 */
	const char* test_path="/foo/bar";
	scpipe cd_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (cd_cmd, bfromcstr ("cd"));
	scpipe_push_back (cd_cmd, bfromcstr (test_path));
	scpipe_push_back (test_pipe, cd_cmd);

	execute_pipeline (test_pipe);
	/* Esto no debería haber tratado de crear ni destruir procesos */
	fail_unless (mock_counter_fork==0, NULL);
	fail_unless (mock_counter_execvp==0, NULL);
	fail_unless (mock_counter_exit==0, NULL);
	/* Hizo 1 cambio de directorio */
	fail_unless (mock_counter_chdir==1, NULL);
	fail_unless (biseqcstr (mock_chdir_last, test_path), NULL);
}
END_TEST

START_TEST (test_external_1_simple_parent)
{
	/* Ejecuta un comando simple, sin argumentos. Verifica que el padre haga
	 * lo que corresponde
	 */
	pid_t pids[] = {101, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre al hijo que termina: */
	mock_wait_setup (pids);

	execute_pipeline (test_pipe);

	/* Esto no fabrica pipes, ni redirecciona, ni manipula archivos*/
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==0, NULL);
	fail_unless (mock_counter_close==0, NULL);
	fail_unless (mock_counter_dup==0, NULL);
	fail_unless (mock_counter_dup2==0, NULL);
	/* Estamos mirando al padre. debería haber hecho un fork */
	fail_unless (mock_counter_fork==1, NULL);
	/* Y después un wait, o un waitpid. Pero no ambos */
	fail_unless (mock_counter_wait+mock_counter_waitpid == 1, NULL);
	/* Además, el wait espero algo que realmente terminó, que resulta ser
	 * el mismo proceso que lanzamos:
	 */
	fail_unless (mock_finished_processes_count == 1, NULL);
	fail_unless (mock_finished_processes[0] == pids[0], NULL);
	/* Estamos mirando al padre. No ejecuta nada ni sale */
	fail_unless (mock_counter_execvp==0, NULL);
	fail_unless (mock_counter_exit==0, NULL);
}
END_TEST

START_TEST (test_external_1_simple_child)
{
	/* Ejecuta un comando simple, sin argumentos. Verifica que el hijo haga
	 * lo que corresponde
	 */
	pid_t pids[] = {0, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva 0, para testear el hijo */
	mock_fork_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Esto no fabrica pipes, ni redirecciona, ni manipula archivos*/
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==0, NULL);
	fail_unless (mock_counter_close==0, NULL);
	fail_unless (mock_counter_dup==0, NULL);
	fail_unless (mock_counter_dup2==0, NULL);
	/* Estamos mirando al hijo. debería haber pasado por un fork */
	fail_unless (mock_counter_fork==1, NULL);
	/* No esperó, por ser el hijo */
	fail_unless (mock_counter_wait==0, NULL);
	fail_unless (mock_counter_waitpid==0, NULL);
	/* En vez, hizo un exec */
	fail_unless (mock_counter_execvp==1, NULL);
	fail_unless (mock_counter_exit==0, NULL);
	/* Hizo el exec con los argumentos correctos */
	fail_unless (strcmp (mock_execvp_last_file,"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[0]!=NULL && strcmp (mock_execvp_last_argv[0],"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[1]==NULL, NULL);
}
END_TEST

START_TEST (test_external_1_simple_background)
{
	/* Ejecuta un comando simple, en bg, sin argumentos. Verifica que el padre
	 * haga lo que corresponde
	 */
	pid_t pids[] = {101, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scpipe_push_back (test_pipe, ext_cmd);
	pipeline_set_wait (test_pipe, false);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);

	execute_pipeline (test_pipe);

	/* Esto no fabrica pipes, ni redirecciona, ni manipula archivos*/
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==0, NULL);
	fail_unless (mock_counter_close==0, NULL);
	fail_unless (mock_counter_dup==0, NULL);
	fail_unless (mock_counter_dup2==0, NULL);
	/* Estamos mirando al padre. debería haber hecho un fork */
	fail_unless (mock_counter_fork==1, NULL);
	/* Background. no hizo wait */
	fail_unless (mock_counter_wait+mock_counter_waitpid == 0, NULL);
	/* Estamos mirando al padre. No ejecuta nada ni sale */
	fail_unless (mock_counter_execvp==0, NULL);
	fail_unless (mock_counter_exit==0, NULL);
}
END_TEST


START_TEST (test_external_arguments)
{
	/* Ejecuta un comando simple, con argumentos. Verifica que exec reciba
	 * bien armada la lista de argumentos.
	 */
	pid_t pids[] = {0, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scpipe_push_back (ext_cmd, bfromcstr ("arg1"));
	scpipe_push_back (ext_cmd, bfromcstr ("-arg2"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva 0, para testear el hijo */
	mock_fork_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Estamos mirando al hijo. debería haber pasado por un fork */
	fail_unless (mock_counter_fork==1, NULL);
	/* Hizo un exec */
	fail_unless (mock_counter_execvp==1, NULL);
	fail_unless (mock_counter_exit==0, NULL);

	/* Hizo el exec con los argumentos correctos */
	fail_unless (strcmp (mock_execvp_last_file,"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[0]!=NULL && strcmp (mock_execvp_last_argv[0],"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[1]!=NULL && strcmp (mock_execvp_last_argv[1],"arg1")==0, NULL);
	fail_unless (mock_execvp_last_argv[2]!=NULL && strcmp (mock_execvp_last_argv[2],"-arg2")==0, NULL);
	fail_unless (mock_execvp_last_argv[3]==NULL, NULL);
}
END_TEST

static void setup_test_pipe (void) {
	/* Auxiliar para los tests de pipe, arma un pipe con 2 comandos:
	 * command arg1 -arg2 | command2 arg3
	 * Mete el valor en test_pipe.
	 */
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scpipe_push_back (ext_cmd, bfromcstr ("arg1"));
	scpipe_push_back (ext_cmd, bfromcstr ("-arg2"));
	scpipe_push_back (test_pipe, ext_cmd);
	ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command2"));
	scpipe_push_back (ext_cmd, bfromcstr ("arg3"));
	scpipe_push_back (test_pipe, ext_cmd);
}

START_TEST (test_pipe2_parent)
{
	/* Ejecuta un pipe de 2 elementos. Verifica que el padre forkee 2 veces
	 * y espere dos veces.
	 * Verifca que el padre cree un pipe y despues haga la cantidad de
	 * close que corresponden (sin chequear mucho los valores de los fds)
	 */
	pid_t pids[] = {101, 102, -1};
	setup_test_pipe ();
	/* Queremos que el fork devuelva 0, para testear el hijo */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre a los hijos que terminan: */
	mock_wait_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Creo un pipe para los dos hijos */
	fail_unless (mock_counter_pipe==1, NULL);
	fail_unless (mock_counter_open==0, NULL);
	/* Solo están conectados stdin/stdout/stderr */
	fail_unless (mock_check_fd (0, KIND_DEV, "ttyin"), NULL);
	fail_unless (mock_check_fd (1, KIND_DEV, "ttyout"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* Las dos puntas del pipe están cerradas: */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
	fail_unless (mock_check_fd (4, KIND_CLOSED, NULL), NULL);
	/* No hace falta hacer dup en el padre */
	fail_unless (mock_counter_dup+mock_counter_dup2 == 0, NULL);
	/* Estamos mirando al padre. Debería haber hecho 2 forks */
	fail_unless (mock_counter_fork==2, NULL);
	/* No hizo exec, exit */
	fail_unless (mock_counter_execvp==0, NULL);
	fail_unless (mock_counter_exit==0, NULL);
	/* Y después un wait, o un waitpid a ambos hijos.
	 * Esto se puede ver en un sleep 1 | sleep 10 contra un sleep 10 | sleep 1
	 */
	fail_unless (mock_counter_wait+mock_counter_waitpid == 2, NULL);
	/* Además, el wait espero algo que realmente terminó, que resulta ser
	 * el mismo proceso que lanzamos:
	 */
	fail_unless (mock_finished_processes_count == 2, NULL);
	fail_unless (mock_finished_processes[0] == pids[0], NULL);
	fail_unless (mock_finished_processes[1] == pids[1], NULL);

}
END_TEST

START_TEST (test_pipe2_child1) {
	/* Ejecuta un pipe de 2 elementos. Verifica que el primer hijo corra el
	 * primer comando y esté bien conectado.
	 * Este test es ligeramente más estricto que lo necesario: el primer hijo
	 * bien podría correr el segundo comando. No me imagino por que hacer eso,
	 * pero no me parece mal si los alumnos cruzan los asserts entre este test
	 * y el que sigue para que eso ande.
	 */
	pid_t pids[] = {0, -1};
	setup_test_pipe ();
	/* Queremos que el fork devuelva 0, para testear el hijo */
	mock_fork_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline(test_pipe);
	);

	/* El padre creo un pipe para los dos hijos _antes_ de forkear */
	fail_unless (mock_counter_pipe==1, NULL);
	fail_unless (mock_counter_open==0, NULL);
	/* Solo están conectados stdin/stderr */
	fail_unless (mock_check_fd (0, KIND_DEV, "ttyin"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* Y stdout a la punta correcta del pipe: */
	fail_unless (mock_check_fd (1, KIND_PIPE, "0"), NULL);
	fail_unless (mock_check_writable (1, true), NULL);
	/* Las puntas originales del pipe están cerradas: */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
	fail_unless (mock_check_fd (4, KIND_CLOSED, NULL), NULL);
	/* Para poner en stdout tiene que haber hecho un dup */
	fail_unless (mock_counter_dup+mock_counter_dup2 == 1, NULL);

	/* Estamos mirando al hijo. Debería haber pasado por 1 fork */
	fail_unless (mock_counter_fork==1, NULL);
	/* No hizo wait/waitpid */
	fail_unless (mock_counter_wait+mock_counter_waitpid == 0, NULL);

	/* Hizo un exec */
	fail_unless (mock_counter_execvp==1, NULL);
	fail_unless (mock_counter_exit==0, NULL);

	/* Hizo el exec con los argumentos correctos */
	fail_unless (strcmp (mock_execvp_last_file,"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[0]!=NULL && strcmp (mock_execvp_last_argv[0],"command")==0, NULL);
	fail_unless (mock_execvp_last_argv[1]!=NULL && strcmp (mock_execvp_last_argv[1],"arg1")==0, NULL);
	fail_unless (mock_execvp_last_argv[2]!=NULL && strcmp (mock_execvp_last_argv[2],"-arg2")==0, NULL);
	fail_unless (mock_execvp_last_argv[3]==NULL, NULL);

}
END_TEST

START_TEST (test_pipe2_child2) {
	/* Ejecuta un pipe de 2 elementos. Verifica que el segundo hijo corra el
	 * segundo comando y esté bien conectado.
	 * Este test es ligeramente más estricto que lo necesario: el segundo hijo
	 * bien podría correr el primer comando. No me imagino por que hacer eso,
	 * pero no me parece mal si los alumnos cruzan los asserts entre este test
	 * y el anterior para que eso ande.
	 */
	pid_t pids[] = {101, 0, -1};
	setup_test_pipe ();
	/* Queremos que el *segundo* fork devuelva 0, para testear el segundo hijo */
	mock_fork_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* El padre creo un pipe para los dos hijos _antes_ de forkear */
	fail_unless (mock_counter_pipe==1, NULL);
	fail_unless (mock_counter_open==0, NULL);
	/* Solo están conectados stdout/stderr */
	fail_unless (mock_check_fd (1, KIND_DEV, "ttyout"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* Y stdin a la punta correcta del pipe: */
	fail_unless (mock_check_fd (0, KIND_PIPE, "0"), NULL);
	fail_unless (mock_check_readable (0, true), NULL);
	/* Las puntas originales del pipe están cerradas: */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
	fail_unless (mock_check_fd (4, KIND_CLOSED, NULL), NULL);
	/* Para poner en stdin tiene que haber hecho un dup */
	fail_unless (mock_counter_dup+mock_counter_dup2 == 1, NULL);

	/* Estamos mirando al hijo. Debería haber pasado por 2 fork */
	fail_unless (mock_counter_fork==2, NULL);
	/* No hizo wait/waitpid */
	fail_unless (mock_counter_wait+mock_counter_waitpid == 0, NULL);

	/* Hizo un exec */
	fail_unless (mock_counter_execvp==1, NULL);
	fail_unless (mock_counter_exit==0, NULL);

	/* Hizo el exec con los argumentos correctos */
	fail_unless (strcmp (mock_execvp_last_file,"command2")==0, NULL);
	fail_unless (mock_execvp_last_argv[0]!=NULL && strcmp (mock_execvp_last_argv[0],"command2")==0, NULL);
	fail_unless (mock_execvp_last_argv[1]!=NULL && strcmp (mock_execvp_last_argv[1],"arg3")==0, NULL);
	fail_unless (mock_execvp_last_argv[2]==NULL, NULL);

}
END_TEST

START_TEST (test_redir_inout_parent)
{
	/* Ejecuta un comando simple, redirigido de salida. Verifica que el padre
	 * haga lo que corresponde
	 */
	pid_t pids[] = {101, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scommand_set_redir_in (ext_cmd, bfromcstr ("input.txt"));
	scommand_set_redir_out (ext_cmd, bfromcstr ("output.txt"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre al hijo que termina: */
	mock_wait_setup (pids);

	execute_pipeline (test_pipe);

	/* Chequeamos que no haya operado archivos desde el padre */
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==0, NULL);
	fail_unless (mock_counter_close==0, NULL);
	fail_unless (mock_counter_dup+mock_counter_dup2==0, NULL);
	/* Solo están conectados stdin/stdout/stderr en el padre */
	fail_unless (mock_check_fd (0, KIND_DEV, "ttyin"), NULL);
	fail_unless (mock_check_fd (1, KIND_DEV, "ttyout"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* El archivo redirigido esta cerrado */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
}
END_TEST

START_TEST (test_redir_out_child)
{
	/* Ejecuta un comando simple, redirigido de salida. Verifica que el hijo
	 * haga lo que corresponde
	 */
	pid_t pids[] = {0, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scommand_set_redir_out (ext_cmd, bfromcstr ("output.txt"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre al hijo que termina: */
	mock_wait_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Chequeamos que haya hecho una apertura de archivo y un dup */
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==1, NULL);
	fail_unless (mock_counter_dup+mock_counter_dup2==1, NULL);
	/* Solo están conectados stdin/stderr en el padre */
	fail_unless (mock_check_fd (0, KIND_DEV, "ttyin"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* stdout va al archivo */
	fail_unless (mock_check_fd (1, KIND_OPEN, "output.txt"), NULL);
	/* Los permisos estan bien, y no están de más */
	fail_unless (mock_check_writable (1, true), NULL);
	fail_unless (mock_check_readable (1, false), NULL);
	/* El archivo original se cerró luego del dup */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
}
END_TEST

START_TEST (test_redir_in_child)
{
	/* Ejecuta un comando simple, redirigido de entrada. Verifica que el hijo
	 * haga lo que corresponde
	 */
	pid_t pids[] = {0, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scommand_set_redir_in (ext_cmd, bfromcstr ("input.txt"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre al hijo que termina: */
	mock_wait_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Chequeamos que haya hecho una apertura de archivo y un dup */
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==1, NULL);
	fail_unless (mock_counter_dup+mock_counter_dup2==1, NULL);
	/* Solo están conectados stdout/stderr en el padre */
	fail_unless (mock_check_fd (1, KIND_DEV, "ttyout"), NULL);
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* stdin viene del archivo */
	fail_unless (mock_check_fd (0, KIND_OPEN, "input.txt"), NULL);
	/* Los permisos estan bien, y no están de más */
	fail_unless (mock_check_readable (0, true), NULL);
	fail_unless (mock_check_writable (0, false), NULL);
	/* El archivo original se cerró luego del dup */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
}
END_TEST

START_TEST (test_redir_inout_child)
{
	/* Ejecuta un comando simple, redirigido x2. Verifica que el hijo
	 * haga lo que corresponde
	 */
	pid_t pids[] = {0, -1};
	scpipe ext_cmd = scpipe_new (SCOMMAND);
	scpipe_push_back (ext_cmd, bfromcstr ("command"));
	scommand_set_redir_in (ext_cmd, bfromcstr ("input.txt"));
	scommand_set_redir_out (ext_cmd, bfromcstr ("output.txt"));
	scpipe_push_back (test_pipe, ext_cmd);
	/* Queremos que el fork devuelva un hijo, para testear el padre: */
	mock_fork_setup (pids);
	/* Queremos que wait encuentre al hijo que termina: */
	mock_wait_setup (pids);

	EXIT_PROTECTED (
		execute_pipeline (test_pipe);
	);

	/* Chequeamos que haya hecho 2 aperturas de archivo y 2 dups */
	fail_unless (mock_counter_pipe==0, NULL);
	fail_unless (mock_counter_open==2, NULL);
	fail_unless (mock_counter_dup+mock_counter_dup2==2, NULL);
	/* Solo está conectado stderr en el padre */
	fail_unless (mock_check_fd (2, KIND_DEV, "ttyout"), NULL);
	/* stdin viene del archivo */
	fail_unless (mock_check_fd (0, KIND_OPEN, "input.txt"), NULL);
	/* Los permisos estan bien, y no están de más */
	fail_unless (mock_check_readable (0, true), NULL);
	fail_unless (mock_check_writable (0, false), NULL);
	/* stdout va al archivo */
	fail_unless (mock_check_fd (1, KIND_OPEN, "output.txt"), NULL);
	/* Los permisos estan bien, y no están de más */
	fail_unless (mock_check_writable (1, true), NULL);
	fail_unless (mock_check_readable (1, false), NULL);
	/* El archivo original se cerró luego del dup */
	fail_unless (mock_check_fd (3, KIND_CLOSED, NULL), NULL);
	fail_unless (mock_check_fd (4, KIND_CLOSED, NULL), NULL);
}
END_TEST


/* TODO:
 * background process, hijo?
 * pipemultiple, padre
 * pipemultiple, primer hijo
 * pipemultiple, hijo al medio
 * pipemultiple, ultimo hijo
 */

/* TODO: Casos para agregar:
 * Dificil: que no se tare si algun fork devuelve -1
 * que haga un exit si execvp() devuelve -1
 * en general, hacer fallar todas las syscalls
 */


/* Armado de la test suite */

Suite *execute_suite (void)
{
	Suite *s = suite_create ("execute");
	TCase *tc_preconditions = tcase_create ("Precondition");
	TCase *tc_functionality = tcase_create ("Functionality");

	/* Precondiciones */
	tcase_add_test_raise_signal(tc_preconditions, test_pipeline_null, SIGABRT);
	suite_add_tcase(s, tc_preconditions);
	/* Funcionalidad */
	tcase_add_checked_fixture (tc_functionality, setup, teardown);
	tcase_add_test (tc_functionality, test_null);
	tcase_add_test (tc_functionality, test_builtin_exit);
	tcase_add_test (tc_functionality, test_builtin_chdir);
	tcase_add_test (tc_functionality, test_external_1_simple_parent);
	tcase_add_test (tc_functionality, test_external_1_simple_child);
	tcase_add_test (tc_functionality, test_external_1_simple_background);
	tcase_add_test (tc_functionality, test_external_arguments);
	tcase_add_test (tc_functionality, test_pipe2_parent);
	tcase_add_test (tc_functionality, test_pipe2_child1);
	tcase_add_test (tc_functionality, test_pipe2_child2);
	tcase_add_test (tc_functionality, test_redir_inout_parent);
	tcase_add_test (tc_functionality, test_redir_out_child);
	tcase_add_test (tc_functionality, test_redir_in_child);
	tcase_add_test (tc_functionality, test_redir_inout_child);
	suite_add_tcase (s, tc_functionality);

	return s;
}
