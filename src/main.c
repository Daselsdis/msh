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
#include <glob.h>
#include <pwd.h>
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

void liberar_environ(char **env, int n_var) {
  for (int i = 0; i < n_var; i++) {
    free(env[i]);
  }
  free(env);
}

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

  int contador_sentencias;

  int bgpid;
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

  int n_envar = 4;
  char **v_envar = malloc(sizeof(char *) * n_envar);

  v_envar[0] = malloc(snprintf(NULL, 0, "%s=%s", "prompt", "msh>") + 1);
  v_envar[1] = malloc(snprintf(NULL, 0, "%s=%d", "mypid", getpid()) + 1);
  v_envar[2] = malloc(snprintf(NULL, 0, "%s=%s", "bgpid", "0") + 1);
  v_envar[3] = malloc(snprintf(NULL, 0, "%s=%s", "status", "0") + 1);

  sprintf(v_envar[0], "%s=%s", "prompt", "msh> ");
  sprintf(v_envar[1], "%s=%d", "mypid", getpid());
  sprintf(v_envar[2], "%s=%s", "bgpid", "0");
  sprintf(v_envar[3], "%s=%s", "status", "0");
  for (int i = 0; i < n_envar; i++) {
    putenv(v_envar[i]);
  }

  while (1) {
    char path[200];
    getcwd(path, 200);

    fprintf(stderr, " %s", getenv("prompt")); /* Prompt */
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0) {
      liberar_environ(v_envar, n_envar);
      break; /* EOF */
    }
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)  /* ERR */
      continue;      /* Empty line */

    /*
     * REDIRECCION y BG
     */

    int fd, fd_reddirIN, fd_reddirOUT, fd_reddirERR;
    if (filev[0]) {
      fd = open(filev[0], O_RDONLY);
      close(0);
      dup(fd);
      fd_reddirIN = fd;
    } else {
      fd_reddirIN = stdin_sav;
    }

    if (filev[1]) {
      fd = creat(filev[1], 0666);
      close(1);
      dup(fd);
      fd_reddirOUT = fd;
    } else {
      fd_reddirOUT = stdout_sav;
    }

    if (filev[2]) {
      fd = creat(filev[2], 0666);
      close(2);
      dup(fd);
      fd_reddirERR = fd;
    } else {
      fd_reddirERR = stderr_sav;
    }

    if (bg) {

      int fd[2];
      pipe(fd);

      bgpid = fork();

      if (bgpid == -1) {
        close(fd[0]);
        close(fd[1]);

        fprintf(stderr, "error fork \n");

      } else if (bgpid != 0) {
        close(fd[1]);
        read(fd[0], &bgpid, 4);
        close(fd[0]);
        wait(NULL);

        char *aux = malloc(snprintf(NULL, 0, "%s=%d", "bgpid", bgpid) + 1);
        sprintf(aux, "%s=%d", "bgpid", bgpid);
        putenv(aux);
        free(v_envar[2]);
        v_envar[2] = aux;

        continue;

      } else {
        bgpid = fork();
        if (bgpid == -1) {

          close(fd[0]);
          close(fd[1]);

          fprintf(stderr, "error fork \n");

        } else if (bgpid != 0) {
          close(fd[0]);
          write(fd[1], &bgpid, 4);
          close(fd[1]);
          printf("[%d]\n", bgpid);
          exit(0);
        } else {
          close(fd[0]);
          close(fd[1]);
        }
      }
    }

    /* preprocesado ,
     *
     * extension de variables y
     *
     *
     * */

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {

      for (argc = 0; (arg = argv[argc]); argc++) {

        char *aux;

        if (!(memchr(arg, '/', strlen(arg)))) {

          if ((memchr(arg, '?', strlen(arg)))) {

            glob_t globaux;

            if (glob(arg, 0, NULL, &globaux) == 0) {

              char **aux_arg;
              char **aux_arg2;

              aux_arg = &argv[argc];
              aux_arg2 = aux_arg + 1;

              while (*aux_arg2) {

                aux_arg2++;
              }

              int siz_fin = (aux_arg2 - (aux_arg + 1));
              int siz_in = argc;
              int siz_var = globaux.gl_pathc;

              char **aux_argv2 =
                  malloc((siz_fin + siz_in + siz_var + 1) * sizeof(char *));

              memcpy(aux_argv2, argv, (siz_in) * sizeof(char *));

              for (int i = 0; i < siz_var; i++) {

                *(aux_argv2 + siz_in + i) = strdup(globaux.gl_pathv[i]);
              }

              /*memcpy(aux_argv2 + (siz_in), globaux.gl_pathv,
                     siz_var * sizeof(char *));*/

              memcpy((aux_argv2 + siz_in + siz_var), aux_arg + 1,
                     siz_fin * sizeof(char *));
              aux_argv2[siz_in + siz_var + siz_fin] = NULL;

              free(arg);
              free(argv);

              globfree(&globaux);

              argvv[argvc] = aux_argv2;
              argv = argvv[argvc];
              arg = argv[argc];
            }
          }
        }

        if (arg[0] == '~') {

          char *aux2 = arg + 1;

          while (*aux2 == '_' || isalnum(*aux2)) {

            aux2++;
          }
          int siz_usr = aux2 - arg;
          if (siz_usr == 1) {

            char *arg_aux;
            int sizarg = strlen(argv[argc]);
            int sizhome = strlen(getenv("HOME"));
            arg_aux = malloc(sizhome + sizarg);
            memcpy(arg_aux, getenv("HOME"), sizhome);
            memcpy(arg_aux + sizhome, arg + 1, sizarg);
            free(arg);
            arg = arg_aux;
            argv[argc] = arg;

          } else {

            char *arg_aux;
            struct passwd *psswd;
            char *user_str = calloc(siz_usr, sizeof(char));
            memcpy(user_str, arg + 1, siz_usr - 1);
            psswd = getpwnam(user_str);
            free(user_str);
            if (psswd) {

              char *home_dir = psswd->pw_dir;
              int sizarg = strlen(argv[argc]);
              int sizhome = strlen(home_dir);

              int siz_fin = sizhome + sizarg - siz_usr;
              arg_aux = calloc(siz_fin + 1, sizeof(char));
              memcpy(arg_aux, home_dir, sizhome);

              memcpy(arg_aux + sizhome, arg + siz_usr, sizarg - siz_usr);
              printf("%s  %i \n", arg_aux, sizhome + sizarg - siz_usr + 1);
              free(arg);
              arg = arg_aux;
              argv[argc] = arg;

            } else {
              perror("error al cojer usuario \n");
            }
          }
        }

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
      contador_sentencias = argvc;
    }
    contador_sentencias++;

    /*
     *
     * Inicializacion pipes
     *
     * */

    int (*pipes)[2] = malloc((contador_sentencias - 1) * sizeof *pipes);

    /*mandatos internnos
     *
     * mandatos no internos  Execv
     */

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {
      if (argvc > 0) {

        close(0);
        dup(pipes[argvc - 1][0]);
        close(pipes[argvc - 1][0]);
      }
      if (argvv[argvc + 1]) {

        if (pipe(pipes[argvc]) == -1) {
          fprintf(stderr, "error pipes \n");
        }
        close(1);
        dup(pipes[argvc][1]);
        close(pipes[argvc][1]);

      } else {

        close(1);
        dup(fd_reddirOUT);
      }

      if (strcmp(argv[0], "exit") == 0) {

        liberar_environ(v_envar, n_envar);
        exit(0);
      }
      if (strcmp(argv[0], "pwd") == 0) {

        char *aux = calloc(200, sizeof(char));

        getcwd(aux, 200);
        printf("Current working directory: %s \n", aux);
        free(aux);

      } else if (strcmp(argv[0], "cd") == 0) {

        char *aux = calloc(200, sizeof(char));

        if (!argv[1]) {
          aux = getenv("HOME");
        }

        else {

          strcpy(aux, argv[1]);

          if (chdir(aux) == -1) {
            fprintf(stderr, "ERROR al buscar %s , no existe ese directorio \n",
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

            fprintf(stderr, "mascara \" %s \" invalida \n", argv[1]);
            break;
          }

          umask(aux_int);
        }

      } else if (strcmp(argv[0], "set") == 0) {
        alarm(2);
        if (!argv[1]) {
          int i = 0;
          while (environ[i]) {
            printf("%s\n", environ[i++]);
          }
        } else {
          for (int i = 1; argv[i] != NULL;) {
            char *aux, *aux2;
            if (argv[i + 1]) {

              aux = getenv(argv[i]);

              if (!aux) {
                n_envar++;
                v_envar = realloc(v_envar, n_envar * sizeof(char *));
                v_envar[n_envar - 1] = malloc(
                    snprintf(NULL, 0, "%s=%s", argv[i], argv[i + 1]) + 1);
                sprintf(v_envar[n_envar - 1], "%s=%s", argv[i], argv[i + 1]);
                if (putenv(v_envar[n_envar - 1])) {
                  perror("ERROR putenv");
                }

              } else {

                aux = malloc(snprintf(NULL, 0, "%s=%s", argv[i], argv[i + 1]) +
                             1);

                sprintf(aux, "%s=%s", argv[i], argv[i + 1]);
                putenv(aux);

                /*
                 * Mem leack inevitable si se usa putenv
                 *
                 * */
              }

              i++;
            } else {
              if (getenv(argv[i])) {
                printf("%s\n", getenv(argv[i]));
              } else {
                printf("variable no declarada\n");
              }
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
              fprintf(stderr, "ERROR recurso no encontrado \n");
            } else {
              getrlimit(tipo_recurso(resources[i]), aux);
              fprintf(stdout, "%s\t%ld", resources[i], aux->rlim_max);
              fprintf(stdout, "\n");
            }
          }

        }

        else {
          for (int i = 1; argv[i] != NULL;) {
            if (argv[i + 1]) {
              if (tipo_recurso(argv[i]) == -1) {
                fprintf(stderr, "ERROR recurso no encontrado \n");
              } else {

                if (getrlimit(tipo_recurso(argv[i]), aux) == 0) {
                  aux->rlim_max = strtol(argv[i + 1], NULL, 0);
                  if (aux->rlim_cur >= aux->rlim_max) {
                    aux->rlim_cur = aux->rlim_max - 1;
                  }
                  if (setrlimit(tipo_recurso(argv[i]), aux) != 0) {
                    perror("ERROR setlimtr \n");
                  }
                }

                else {
                  perror("ERROR getlimtr \n");
                }

                i++;
              }
            } else {
              if (tipo_recurso(argv[i]) == -1) {
                fprintf(stderr, "ERROR recurso no encontrado \n");
              } else {
                if (getrlimit(tipo_recurso(argv[i]), aux) == 0) {

                  fprintf(stdout, "%s\t%ld,  %d \n", argv[i],
                          (long)aux->rlim_max, tipo_recurso(argv[i]));
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
          fprintf(stderr, "ERROR fork \n");
        } else if (pid == 0) {
          sigprocmask(SIG_UNBLOCK, &sigset, NULL);
          if (execvp(argv[0], argv) == -1) {

            perror("ERROR execvp ");
            exit(-1);
          }
        } else {
          int valor;
          wait(&valor);

          char *aux = malloc(snprintf(NULL, 0, "%s=%d", "status", valor) + 1);
          sprintf(aux, "%s=%d", "status", valor);
          putenv(aux);
          free(v_envar[3]);
          v_envar[3] = aux;
        }
      }
    }
    free(pipes);

    close(STDIN);
    dup(stdin_sav);
    close(STDOUT);
    dup(stdout_sav);
    close(STDERR);
    dup(stderr_sav);
  }
}
