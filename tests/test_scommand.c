#include <check.h>
#include "test_scommand.h"

#include <signal.h>
#include <assert.h>
#include <string.h> /* para strcmp */
#include <stdlib.h> /* para calloc */
#include <stdio.h> /* para sprintf */

#include "bstring/bstrlib.h"
#include "command.h"

#define MAX_LENGTH 257 /* no hay nada como un primo para molestar */

static scpipe scmd = NULL; /* para armar scpipe temporales */

/* Testeo precondiciones. 
 * Se espera que todos estos tests fallen con un assert(), son todas 
 * llamadas inválidas 
 */
START_TEST (test_destroy_null)
{
	scpipe_destroy (NULL);
}
END_TEST


START_TEST (test_push_back_null)
{
	scpipe_push_back (NULL, bfromcstr ("123"));
}
END_TEST

START_TEST (test_push_back_argument_null)
{
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, NULL);
	scpipe_destroy (scmd); scmd = NULL;
}
END_TEST

START_TEST (test_pop_front_null)
{
	scpipe_pop_front (NULL);
}
END_TEST

START_TEST (test_pop_front_empty)
{
	scmd = scpipe_new (SCOMMAND);
	scpipe_pop_front (scmd);
	scpipe_destroy(scmd); scmd = NULL;
}
END_TEST

START_TEST (test_set_redir_in_null)
{
	scommand_set_redir_in (NULL, bfromcstr("123"));
}
END_TEST

START_TEST (test_set_redir_out_null)
{
	scommand_set_redir_out (NULL, bfromcstr("123"));
}
END_TEST

START_TEST (test_is_empty_null)
{
	scpipe_is_empty (NULL);
}
END_TEST

START_TEST (test_length_null)
{
	scpipe_length (NULL);
}
END_TEST

START_TEST (test_front_null)
{
	scpipe_front (NULL);
}
END_TEST

START_TEST (test_front_empty)
{
	scmd = scpipe_new (SCOMMAND);
	scpipe_front (scmd);
	scpipe_destroy(scmd); scmd = NULL;
}
END_TEST

START_TEST (test_get_redir_in_null)
{
	scommand_get_redir_in (NULL);
}
END_TEST

START_TEST (test_get_redir_out_null)
{
	scommand_get_redir_out (NULL);
}
END_TEST

START_TEST (test_to_string_null)
{
	scommand_to_string (NULL);
}
END_TEST


/* Crear y destruir */
START_TEST (test_new_destroy)
{
	scmd = scpipe_new (SCOMMAND);
	/* Verificamos que se pueda crear y destruir un scpipe sin problemas */
	scpipe_destroy (scmd); scmd = NULL;
}
END_TEST

START_TEST (test_new_is_empty)
{
	scmd = scpipe_new (SCOMMAND);
	/* Un comando recién creado debe ser vacío */
	fail_unless (scpipe_is_empty (scmd), NULL);
	fail_unless (scpipe_length (scmd) == 0, NULL);
	scpipe_destroy (scmd); scmd = NULL;
}
END_TEST

/* Testeo de funcionalidad */

static void setup (void) {
	scmd = scpipe_new (SCOMMAND);
}

static void teardown (void) {
	if (scmd != NULL) {
		scpipe_destroy (scmd);
		scmd = NULL;
	}
}

/* is_empty sea acorde a lo que agregamos y quitamos */
START_TEST (test_adding_emptying)
{
	unsigned int i = 0;
	for (i=0; i<MAX_LENGTH; i++) {
		fail_unless ((i==0) == scpipe_is_empty (scmd), NULL);
		scpipe_push_back (scmd, bfromcstr ("123"));
	}
	for (i=0; i<MAX_LENGTH; i++) {
		fail_unless (!scpipe_is_empty(scmd), NULL);
		scpipe_pop_front (scmd);
	}
	fail_unless (scpipe_is_empty (scmd), NULL);
}
END_TEST

/* length sea acorde a lo que agregamos y quitamos */
START_TEST (test_adding_emptying_length)
{
	unsigned int i = 0;
	for (i=0; i<MAX_LENGTH; i++) {
		fail_unless (i == scpipe_length (scmd), NULL);
		scpipe_push_back (scmd, bfromcstr ("123"));
	}
	for (i=MAX_LENGTH; i>0; i--) {
		fail_unless (i == scpipe_length (scmd), NULL);
		scpipe_pop_front (scmd);
	}
	fail_unless (0 == scpipe_length (scmd), NULL);
}
END_TEST

/* Meter por atrás y sacar por adelante, da la misma secuencia. */
START_TEST (test_fifo)
{
	unsigned int i = 0;
	bstring *strings = NULL;
	strings = calloc (MAX_LENGTH, sizeof(bstring));
	for (i=0; i<MAX_LENGTH; i++) {
		strings[i] = bformat ("%d", i);
	}
	/* strings = ["0","1", ..., "256"] */
	for (i=0; i<MAX_LENGTH; i++) {
		/* Copia antes de push: scpipe se apropia */
		scpipe_push_back (scmd, bstrcpy(strings[i]));
	}
	for (i=0; i<MAX_LENGTH; i++) {
		/* mismo string */
		fail_unless (biseq (scpipe_front (scmd),strings[i]) == 1, NULL);
		bdestroy(strings[i]);
		scpipe_pop_front (scmd);
	}
	free (strings);
}
END_TEST

/* hacer muchísimas veces front es lo mismo */
START_TEST (test_front_idempotent)
{
	unsigned int i = 0;
	scpipe_push_back (scmd, bfromcstr ("123"));
	for (i=0; i<MAX_LENGTH; i++) {
		fail_unless (biseqcstr (scpipe_front (scmd), "123") == 1, NULL);
	}
}
END_TEST

/* Si hay solo uno, entonces front=back */
START_TEST (test_front_is_back)
{
	scpipe_push_back (scmd, bfromcstr ("123"));
	fail_unless (biseqcstr (scpipe_front (scmd), "123") == 1, NULL);
}
END_TEST

/* Si hay dos distintos entonces front!=back */
START_TEST (test_front_is_not_back)
{
	scpipe_push_back(scmd, bfromcstr ("123"));
	scpipe_push_back(scmd, bfromcstr ("456"));
	fail_unless (biseqcstr (scpipe_front (scmd), "456") != 1, NULL);
}
END_TEST

/* Que la tupla de redirectores sea un par independiente */
START_TEST (test_redir)
{
	scommand_set_redir_in (scmd, bfromcstr ("123"));
	scommand_set_redir_out (scmd, bfromcstr ("456"));
	/* Los redirectores tienen que ser distintos */
	fail_unless (biseq (scommand_get_redir_in (scmd),
			scommand_get_redir_out (scmd)) != 1, NULL);
	/* ahora si ambos idem */
	scommand_set_redir_out (scmd, bfromcstr ("123"));
	fail_unless (biseq (scommand_get_redir_in (scmd), scommand_get_redir_out (scmd)) == 1, NULL);
}
END_TEST

START_TEST (test_independent_redirs)
{
	/* Cuando la gente copia y pega entre las funciones de redir_in y 
	 * redir_out, tiende a olvidarse de hacer s/in/out/. Probamos algunos
	 * casos simples
	 */

	/* Primero: sólo entrada */
	scommand_set_redir_in (scmd, bfromcstr ("123"));
	fail_unless (biseqcstr (scommand_get_redir_in (scmd), "123") == 1, NULL);
	fail_unless (scommand_get_redir_out (scmd) == NULL, NULL);

	/* Segundo: Volvemos ambos a NULL */
	scommand_set_redir_in (scmd, NULL);
	fail_unless (scommand_get_redir_in (scmd) == NULL, NULL);
	fail_unless (scommand_get_redir_out (scmd) == NULL, NULL);

	/* Tercero: sólo salida */
	scommand_set_redir_out (scmd, bfromcstr ("456"));
	fail_unless (scommand_get_redir_in (scmd) == NULL, NULL);
	fail_unless (biseqcstr (scommand_get_redir_out (scmd), "456") == 1, NULL);

	/* Cuarto: ambos */
	scommand_set_redir_in (scmd, bfromcstr ("123"));
	fail_unless (biseqcstr (scommand_get_redir_in (scmd), "123") == 1, NULL);
	fail_unless (biseqcstr (scommand_get_redir_out (scmd), "456") == 1, NULL);
}
END_TEST


/* Comando nuevo, string vacío */
START_TEST (test_to_string_empty)
{
	bstring str = NULL;
	str = scommand_to_string (scmd);
	fail_unless (blength (str) == 0, NULL);
	bdestroy (str);
}
END_TEST

/* Algo más fuerte. Poner muchos argumentos, mirar el orden 
 * Poner redirectores, mirar el orden
 */
START_TEST (test_to_string)
{
	bstring str = NULL;
	bstring *strings = NULL;
	int i = 0;
	int last_pos = 0;
	strings = calloc (MAX_LENGTH, sizeof(bstring));
	for (i=0; i<MAX_LENGTH; i++) {
		strings[i] = bformat ("%d", i);
	}
	/* strings = ["0","1", ..., "127"] */

	assert (MAX_LENGTH>2);
	/* comando "0 1 2 .... N-3 < N-2 > N-1" tiene que tener todos los números y los dos piquitos */
	for (i=0; i<MAX_LENGTH; i++) {
		if (i<MAX_LENGTH-2) {
			scpipe_push_back (scmd, strings[i]);
		} else if (i==MAX_LENGTH-2) {
			scommand_set_redir_in (scmd, strings[i]);
		} else {
			assert(i==MAX_LENGTH-1);
			scommand_set_redir_out (scmd, strings[i]);			
		}
	}
	str = scommand_to_string (scmd);
	last_pos = 0;
	for (i=0; i<MAX_LENGTH; i++) {
		if (i<MAX_LENGTH-2) {
			fail_unless (binstr(str,0,strings[i])>=last_pos, NULL);
			last_pos = binstr (str,0,strings[i]);
		} else if (i==MAX_LENGTH-2) {
			fail_unless (binstr(str,0,strings[i])>=0, NULL);
			fail_unless (bstrchr(str, '<'), NULL);
			fail_unless (binstr(str,0,strings[i])>bstrchr(str, '<'), NULL);
		} else {
			assert(i==MAX_LENGTH-1);
			fail_unless (binstr(str,0,strings[i])>=0, NULL);
			fail_unless (bstrchr(str, '>'), NULL);
			fail_unless (binstr(str,0,strings[i])>bstrchr(str, '>'), NULL);
		}
	}
	bdestroy (str);
	free(strings);
}
END_TEST


/* Armado de la test suite */

Suite *scommand_suite (void)
{
	Suite *s = suite_create ("scpipe");
	TCase *tc_preconditions = tcase_create ("Precondition");
	TCase *tc_creation = tcase_create ("Creation");
	TCase *tc_functionality = tcase_create ("Functionality");

	/* Precondiciones */
	tcase_add_test_raise_signal (tc_preconditions, test_destroy_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_push_back_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_push_back_argument_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_pop_front_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_pop_front_empty, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_set_redir_in_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_set_redir_out_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_is_empty_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_length_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_front_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_front_empty, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_get_redir_in_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_get_redir_out_null, SIGABRT);
	tcase_add_test_raise_signal (tc_preconditions, test_to_string_null, SIGABRT);
	suite_add_tcase (s, tc_preconditions);

	/* Creation */
	tcase_add_test (tc_creation, test_new_destroy);
	tcase_add_test (tc_creation, test_new_is_empty);
	suite_add_tcase (s, tc_creation);

	/* Funcionalidad */
	tcase_add_checked_fixture (tc_functionality, setup, teardown);
	tcase_add_test (tc_functionality, test_adding_emptying);
	tcase_add_test (tc_functionality, test_adding_emptying_length);
	tcase_add_test (tc_functionality, test_fifo);
	tcase_add_test (tc_functionality, test_front_idempotent);
	tcase_add_test (tc_functionality, test_front_is_back);
	tcase_add_test (tc_functionality, test_front_is_not_back);
	tcase_add_test (tc_functionality, test_redir);
	tcase_add_test (tc_functionality, test_independent_redirs);
	tcase_add_test (tc_functionality, test_to_string_empty);
	tcase_add_test (tc_functionality, test_to_string);
	suite_add_tcase (s, tc_functionality);

	return s;
}

/* Para testing de memoria */
void scommand_memory_test (void) {
	/* Las siguientes operaciones deberían poder hacer sin leaks ni doble 
	 * frees.
	 */

	/* Crear y destruir un scpipe vacío */
	scmd = scpipe_new (SCOMMAND);
	scpipe_destroy (scmd);

	/* Crear un scpipe, llenarlo de argumentos, liberarlo
	 * sin liberar los argumentos desde afuera
	 */
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, bfromcstr ("un-argumento"));
	scpipe_push_back (scmd, bfromcstr ("otro-argumento"));
	scpipe_destroy(scmd);

	/* Crear un scpipe, setearle redirectores, liberarlo
	 * sin liberar los redirectores desde afuera
	 */
	scmd = scpipe_new (SCOMMAND);
	scommand_set_redir_in (scmd, bfromcstr ("entrada"));
	scommand_set_redir_out (scmd, bfromcstr ("salida"));
	scpipe_destroy(scmd);

	/* Meter, sacar, meter argumentos alternadamente */
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, bfromcstr ("uno"));
	scpipe_push_back (scmd, bfromcstr ("dos"));
	scpipe_push_back (scmd, bfromcstr ("cinco"));
	scpipe_pop_front (scmd);
	scpipe_push_back (scmd, bfromcstr ("perdon, tres"));
	scpipe_push_back (scmd, bfromcstr ("cuatro"));
	scpipe_destroy(scmd);

	/* Modificar redictores ya seteados */
	scmd = scpipe_new (SCOMMAND);
	scommand_set_redir_out (scmd, bfromcstr ("entrada"));
	scommand_set_redir_out (scmd, bfromcstr ("quise decir salida"));
	scpipe_destroy(scmd);

	/* Usar todos los accesores */
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, bfromcstr ("comando"));
	scpipe_push_back (scmd, bfromcstr ("un-argumento"));
	scpipe_push_back (scmd, bfromcstr ("otro-argumento"));
	scommand_set_redir_out (scmd, bfromcstr ("salida"));
	scpipe_is_empty (scmd);
	scpipe_length (scmd);
	scpipe_front (scmd);
	scommand_get_redir_in (scmd);
	scommand_get_redir_out (scmd);
	scpipe_destroy (scmd);
	
	/* Se puede modificar el TAD luego de acceder a front, aún cuando eso
	 * invalida el resultado anterior de front. Sin leaks, al igual que todos
	 * estos casos.
	 */
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, bfromcstr ("comando"));
	scpipe_front (scmd);
	scpipe_pop_front (scmd);
	/* En este punto, s puede ser no válido. */
	scpipe_destroy (scmd);
	 
	/* Al string que me devuelve to_string lo puedo liberar. Y por dentro, 
	 * no hay leaks, al igual que todos estos casos.
	 */
	scmd = scpipe_new (SCOMMAND);
	scpipe_push_back (scmd, bfromcstr ("comando"));
	scpipe_push_back (scmd, bfromcstr ("argument"));
	scommand_set_redir_out (scmd, bfromcstr ("salida"));
	bdestroy (scommand_to_string (scmd));
	scpipe_destroy (scmd);
}

