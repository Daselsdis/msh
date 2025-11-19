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

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h> /* NULL */
#include <stdio.h>  /* setbuf, printf */
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */
#define STDIN 0
#define STDOUT 1
#define STDERR 2

int tipo_recurso(char *str) {
  if (!strcmp(str, "cpu")) {
    return RLIMIT_CPU;
  }
  if (!strcmp(str, "fsize")) {
    return RLIMIT_FSIZE;
  }
  if (!strcmp(str, "data")) {
    return RLIMIT_DATA;
  }
  if (!strcmp(str, "stack")) {
    return RLIMIT_STACK;
  }
  if (!strcmp(str, "core")) {
    return RLIMIT_CORE;
  }
  if (!strcmp(str, "nofile")) {
    return RLIMIT_NOFILE;
  }
  return -1;
}

int main(void) {
  char ***argvv = NULL;
  char **argv = NULL;
  char *arg = NULL;
  int argvc;
  int argc;
  char *filev[3] = {NULL, NULL, NULL};
  int bg;
  int ret;

  int stdout_sav, stdin_sav, stderr_sav;
  char *resources[] = {"cpu", "fsize", "data", "stack", "core", "nofile"};
  int resources_size = 6;
  sigset_t sigset;
  setbuf(stdout, NULL); /* Unbuffered */
  setbuf(stdin, NULL);

  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  sigaddset(&sigset, SIGQUIT);

  sigprocmask(SIG_BLOCK, &sigset, NULL);

  stdin_sav = dup(STDIN);
  stdout_sav = dup(STDOUT);
  stderr_sav = dup(STDERR);

  extern char **environ;

  while (1) {
    char path2[200];
    getcwd(path2, 200);

    fprintf(stderr, "\n %s %s", path2, "Ψ >"); /* Prompt */
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      //     break; /* EOF */
      continue;
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)  /* ERR */
      continue;      /* Empty line */

    /*
     * REDIRECCION
     */

    int fd;
    if (filev[0]) {
      fd = open(filev[0], O_RDONLY);
      close(0);
      dup(fd);
      printf("< %s\n", filev[0]);
    }

    if (filev[1]) {
      fd = creat(filev[1], 0666);
      close(1);
      dup(fd);
      printf("> %s\n", filev[1]);
    }

    if (filev[2]) {
      fd = creat(filev[2], 0666);
      close(2);
      dup(fd);
      fprintf(stderr, ">& %s\n", filev[2]);
    }

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {

      for (argc = 0; (arg = argv[argc]); argc++) {
        if (arg[0] == '~') {
          char *arg_aux;
          int sizarg = strlen(argv[argc]);
          int sizhome = strlen(getenv("HOME"));
          arg_aux = malloc(sizhome + sizarg);
          memcpy(arg_aux, getenv("HOME"), sizhome);
          memcpy(arg_aux + sizhome, arg + 1, sizarg);
          free(arg);
          arg = arg_aux;
          argv[argc] = arg;
        }

        char *aux;
        if ((aux = memchr(arg, '$', strlen(arg))) != NULL) {

          while (aux) {
            char *aux2 = aux + 1;
            if (*aux2 == '\0' || !(isalnum(*aux2) || *aux2 == '_')) {
              break;
            }
            while ((isalnum(*aux2) || *aux2 == '_')) {
              aux2++;
            }

            size_t siz_var = (aux2 - (aux + 1));
            size_t siz_init = (aux - arg);
            size_t siz_fin = strlen(aux2);
            size_t offset2 = aux2 - arg;
            char *buff_Aux = malloc(siz_var + 1);
            memcpy(buff_Aux, aux + 1, (siz_var));
            buff_Aux[siz_var] = '\0';

            char *var = getenv(buff_Aux);

            if (!var) {
              siz_var = 0;
            } else {
              siz_var = strlen(var);
            }
            buff_Aux = realloc(buff_Aux, (siz_fin + 1) * sizeof(char));
            memcpy(buff_Aux, aux2, siz_fin + 1);

            arg =
                realloc(arg, sizeof(char) * (siz_init + siz_fin + siz_var + 1));

            memcpy(arg + siz_init + siz_var, buff_Aux, siz_fin + 1);
            memcpy(arg + siz_init, var, siz_var);

            free(buff_Aux);

            aux = memchr(arg + siz_init + siz_var, '$',
                         strlen(arg + siz_init + siz_var));
          }
          argv[argc] = arg;
        }
      }
      if (strcmp(argv[0], "exit") == 0) {
        exit(0);
      }
      if (strcmp(argv[0], "pwd") == 0) {

        char *aux = calloc(200, sizeof(char));
        ;
        getcwd(aux, 200);
        printf("Current working directory: %s", aux);
        free(aux);

      } else if (strcmp(argv[0], "cd") == 0) {

        char *aux = calloc(200, sizeof(char));
        ;
        if (!argv[1]) {
          aux = getenv("HOME");
        }

        else {
          // if (!strchr(argv[1],'~')) {
          strcpy(aux, argv[1]);
          // }
          /* else if (strchr(argv[1],'~') != strrchr(argv[1],'~')) {
             fprintf(stderr,"ERROR, solo puede haber un ~ como maximo");
             break;
           }                else if(strchr(argv[1],'~')){
               if (strcmp(argv[1],"~") == 0) {
                 strcpy(path,getenv("HOME"));
               }
               else if (argv[1][0] == '~') {
                 strcpy(path,getenv("HOME"));
                 char* aux = strtok(argv[1],"~");
                 strcat(path,aux);
               }
               else if (argv[1][strlen(argv[1])-1] == '~') {
               char* aux = strtok(argv[1],"~");
               strcpy(path,aux);
               strcat(path,getenv("HOME"));
               }
               else{
               char* aux = strtok(argv[1],"~");
               strcpy(path,aux);
               strcat(path,getenv("HOME"));
               aux = strtok(argv[1], "~");
               strcat(path,aux);
             }

            } */

          if (chdir(aux) == -1) {
            fprintf(stderr, "ERROR al buscar %s , no existe ese directorio ",
                    aux);
          }
          free(aux);
          break;
        }

      } else if (strcmp(argv[0], "umask") == 0) {
        int aux_int;
        if (!argv[1]) {
          aux_int = umask(0);
          printf("%o \n", aux_int);
          aux_int = umask(aux_int);

        } else {
          aux_int = strtol(argv[1], NULL, 8);

          if (errno == ERANGE || !(aux_int >= 0 && aux_int <= 0x777)) {

            fprintf(stderr, "mascara \" %s \" invalida", argv[1]);
            break;
          }

          umask(aux_int);
        }

      } else if (strcmp(argv[0], "set") == 0) {
        alarm(2);
        if (!argv[1]) {
          int i = 0;
          while (environ[i]) {
            printf("%s\n", environ[i++]); // prints in form of "variable=value"
          }
        } else {
          for (int i = 1; argv[i] != NULL;) {
            char *aux;
            if (argv[i + 1]) {
              aux = malloc(200 * sizeof(char));
              int siz = sprintf(aux, "%s=%s", argv[i], argv[i + 1]);
              aux = realloc(aux, (siz + 1) * sizeof(char));
              if (putenv(aux) != 0) {
                fprintf(stderr, "ERROR putenv");
              }
              i++;
            } else {
              aux = getenv(argv[i]);
              fprintf(stdout, "%s\n", aux);
            }
            i++;
          }
        }
        alarm(0);

      } else if (strcmp(argv[0], "limit") == 0) {
        alarm(2);
        struct rlimit aux_lim;
        struct rlimit *aux = &aux_lim;
        if (!argv[1]) {
          for (int i = 0; i < resources_size; i++) {

            if (tipo_recurso(resources[i]) == -1) {
              fprintf(stderr, "ERROR recurso no encontrado");
            } else {
              getrlimit(tipo_recurso(resources[i]), aux);
              fprintf(stdout, "%s\t%lu", resources[i], aux->rlim_max);
              fprintf(stdout, "\n");
            }
          }

        }

        else {
          for (int i = 1; argv[i] != NULL;) {
            if (argv[i + 1]) {
              if (tipo_recurso(argv[i]) == -1) {
                fprintf(stderr, "ERROR recurso no encontrado");
              } else {

                if (getrlimit(tipo_recurso(argv[i]), aux) == 0) {
                  aux->rlim_max = strtol(argv[i + 1], NULL, 0);
                  if (setrlimit(tipo_recurso(argv[i]), aux) != 0) {
                    perror("ERROR setlimtr");
                  }
                }

                else {
                  perror("ERROR getlimtr");
                }

                i++;
              }
            } else {
              if (tipo_recurso(argv[i]) == -1) {
                fprintf(stderr, "ERROR recurso no encontrado");
              } else {
                if (getrlimit(tipo_recurso(argv[i]), aux) == 0) {

                  fprintf(stdout, "%s\t%ld,  %d", argv[i], (long)aux->rlim_max,
                          tipo_recurso(argv[i]));
                }

                else {
                  perror("ERROR getlimtr");
                }
              }
            }
            i++;
          }
        }
        alarm(0);
      }

      else {
        pid_t pid = fork();

        if (pid == -1) {
          fprintf(stderr, "ERROR fork");
        } else if (pid == 0) {
          sigprocmask(SIG_UNBLOCK, &sigset, NULL);
          if (execvp(argv[0], argv) == -1) {

            fprintf(stderr, "ERROR execvp");
          }
        } else {
          int valor;
          wait(&valor);
          printf("%i", valor);
        }
      }

      printf("\n");
    }
    // exit(0);
    // return 0;
    //
    // DEVOLUCION ESTANDAR
    close(STDIN);
    dup(stdin_sav);
    close(STDOUT);
    dup(stdout_sav);
    close(STDERR);
    dup(stderr_sav);
  }
}
