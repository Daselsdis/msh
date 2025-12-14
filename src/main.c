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

#include <assert.h>
#include <errno.h>
#include <stddef.h> /* NULL */
#include <stdio.h>  /* setbuf, printf */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define charAscii(a) (a - '\0')
#define cmpChar(cursor, c) (charAscii(cursor) == charAscii(c))
#define charInRange(cursor, a, b)                                              \
  (charAscii(cursor) >= charAscii(a) && charAscii(cursor) <= charAscii(b))
#define cmpA(cursor)                                                           \
  (charInRange(cursor, 'a', 'z') || charInRange(cursor, 'A', 'Z') ||           \
   cmpChar(cursor, '_'))
#define cmpD(cursor) (charInRange(cursor, '0', '9'))

extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */

char *commands = {"cd"};

typedef struct {
  int size;
  char **argsExp;
} expand;

int strContN(char *str, char cont, int skip) {
  int res = 0, tempRes, i, j, lenStr = strlen(str), skipped = 0;

  for (i = 0; i < lenStr && !res; i++) {
    res = str[i] == cont;
    if (res && skipped < skip) {
      res = 0;
      skipped++;
    }
  }
  return !res ? i : -1;
}

int necesitaExpasion(char *str) {
  if (str[0] == '~' || strContN(str, '\\', 0) > -1 ||
      strContN(str, '?', 0) > -1 || strContN(str, '$', 0) > -1)
    return 1;
  return 0;
}

int addExpasion(expand *exp, char *str) {
  exp->size++;
  exp->argsExp = realloc(exp->argsExp, exp->size * sizeof(char *));
  if (exp->argsExp == NULL)
    return 0;
  int strLen = strlen(str);
  exp->argsExp[exp->size - 1] = malloc((strLen + 1) * sizeof(char));
  if (exp->argsExp[exp->size - 1] == NULL)
    return 0;
  memcpy(exp->argsExp[exp->size - 1], str, strLen);
  exp->argsExp[exp->size - 1][strLen] = '\0';
  return 1;
}

char *getsVarName(char *str) {
  char *ret = NULL;
  int n;

  errno = 0;
  n = sscanf(str, "%ms([a-zA-Z_][a-zA-Z0-9_])", &ret);

  if (errno != 0) {
    perror("sscanf");
  } else if (ret == NULL) {
    ret = "";
    return ret;
  }

  return ret;
}

expand *expandir(char *str) {
  expand *res = calloc(1, sizeof(expand));

  if (!necesitaExpasion(str)) {
    if (!addExpasion(res, str))
      assert(0 && "ERROR AÑADIENDO A EXPAND");
    /* TODO: Manejo al fallo en expansión, maybe*/
    return res;
  }
  int pos = strContN(str, '$', 0);
  if (pos == 0 || (pos > 0 && str[pos - 1] != '\\')) {
    char *var = getsVarName(&str[pos]);
  }

  if (str[0] == '~') {
    char *dir = getenv("HOME");
    if (dir != NULL) {
      strcat(dir, &str[1]);
    } else
      assert(0 && "$HOME VAR NOT SET");
    /* TODO: Usar size negativo para errores en la expansión (p.e. size -1 no
     * $HOME), por tanto liberar res.argsExp*/
  }

  return res;
}

int changeDir(char *path) { return 0; }

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
/* TODO: Implementar que si un mandato termina en "\\", se considera error si no
 * hay otro mandato siguiendo "\ ", que el parser corta por el espacio, por
 * ahora no se implementa, pero se puede confacilidad, el resto del proceso
 * debería ser compatible  por defecto.*/
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
    char *ret = getsVarName(filev[0]);
    printf("%s", ret);
  }
  exit(0);
  return 0;
}
