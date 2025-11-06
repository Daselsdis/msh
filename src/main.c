/*-
 * main.c
 * Minishell C source
 * Shows how to use "obtain_order" input interface function.
 *
 * Copyright (c) 1993-2002-2019, Francisco Rosales <frosal@fi.upm.es>
 * Todos los derechos reservados.
 *
 * Publicado bajo Licencia de Proyecto Educativo Práctico
 * <http://laurel.datsi.fi.upm.es/~ssoo/LICENCIA/LPEP>
 *
 * Queda prohibida la difusión total o parcial por cualquier
 * medio del material entregado al alumno para la realización
 * de este proyecto o de cualquier material derivado de este,
 * incluyendo la solución particular que desarrolle el alumno.
 *
 * DO NOT MODIFY ANYTHING OVER THIS LINE
 * THIS FILE IS TO BE MODIFIED
 */

#include <stddef.h> /* NULL */
#include <stdio.h>  /* setbuf, printf */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */

char *commands = {"cd"};

int cd(char *args) {
  char *dir;
  if (args == NULL) { /* NOT passed a dir, $HOME */
    dir = getenv("HOME");
    if (dir != NULL) {
      return chdir(dir);
    } else {
    }
  } else {                /* Passed a dir */
    if (args[0] == '~') { /* Passed a dir from $HOME */
      dir = getenv("HOME");
      if (dir != NULL) {
        strcat(dir, args);
        return chdir(dir);
      } else {
        perror("$HOME VAR NOT SET");
      }
    } else {
      strcat(dir, args);
      return chdir(dir);
    }
  }
  return -1;
}

int main(void) {
  char ***argvv = NULL;
  int argvc;
  char **argv = NULL;
  int argc;
  char *filev[3] = {NULL, NULL, NULL};
  int bg;
  int ret;

  setbuf(stdout, NULL); /* Unbuffered */
  setbuf(stdin, NULL);

  while (1) {
    fprintf(stderr, "%s", "msh> "); /* Prompt */
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      break; /* EOF */
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)
      continue; /* Empty line */
#if 0
    /*
     * LAS LINEAS QUE A CONTINUACION SE PRESENTAN SON SOLO
     * PARA DAR UNA IDEA DE COMO UTILIZAR LAS ESTRUCTURAS
     * argvv Y filev. ESTAS LINEAS DEBERAN SER ELIMINADAS.
     */
    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      for (argc = 0; argv[argc]; argc++)
        printf("%s ", argv[argc]);
      printf("\n");
    }
    if (filev[0])
      printf("< %s\n", filev[0]); /* IN */
    if (filev[1])
      printf("> %s\n", filev[1]); /* OUT */
    if (filev[2])
      printf(">& %s\n", filev[2]); /* ERR */
    if (bg)
      printf("&\n");
/*
 * FIN DE LA PARTE A ELIMINAR
 */
#endif

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      for (argc = 0; argv[argc]; argc++) {
        if (strcmp("cd", argv[argc]) == 0) {
          printf("CD EMPEZANDO");
          if (cd(argv[argc + 1]) == -1) {
            perror("Error in cd execution: ");
          }
        }
      }
      /* printf("%s\n", argv[argc]); */
      /* printf("Hasta aquí argv %d\n", argvc); */
    }
    if (filev[0])
      printf("< %s\n", filev[0]); /* IN */
    if (filev[1])
      printf("> %s\n", filev[1]); /* OUT */
    if (filev[2])
      printf(">& %s\n", filev[2]); /* ERR */
    if (bg)
      printf("&\n");
  }
  exit(0);
  return 0;
}
