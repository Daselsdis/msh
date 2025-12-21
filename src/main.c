#include <assert.h>
#include <bits/types/sigset_t.h>
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

extern char **environ;

char *commands[5] = {"cd", "set", "umask", "limit", "gen"};

#define Autosprintf(buff, msg, ...)                                            \
  {                                                                            \
    int AUTOSPRINTF = snprintf(NULL, 0, msg, __VA_ARGS__);                     \
    buff = malloc((AUTOSPRINTF + 1) * sizeof(char));                           \
    snprintf(buff, AUTOSPRINTF + 1, msg, __VA_ARGS__);                         \
  }

typedef struct {
  int size;
  char **argsExp;
} expand;

int strCont(char *str, char cont) {

  int res = 0, i, lenStr = strlen(str);

  for (i = 0; i < lenStr && !res; i++) {
    res = str[i] == cont;
  }

  return res != 0 ? i : -1;
}

/* TODO: Change this, it isn't needed in this form */
int necesitaExpasion(char *str) {
  if (str[0] == '~' || strCont(str, '?') > -1 || strCont(str, '$') > -1)
    return 1;
  return 0;
}

int addExpasion(expand *exp, char *str) {
  exp->size++;
  exp->argsExp = realloc(exp->argsExp, exp->size * sizeof(char *));
  if (exp->argsExp == NULL)
    return 0;
  if (str == NULL) {
    exp->argsExp[exp->size - 1] = NULL;
  } else {
    int strLen = strlen(str);
    exp->argsExp[exp->size - 1] = malloc((strLen + 1) * sizeof(char));
    if (exp->argsExp[exp->size - 1] == NULL)
      return 0;
    memcpy(exp->argsExp[exp->size - 1], str, strLen);
    exp->argsExp[exp->size - 1][strLen] = '\0';
  }
  return 1;
}

void freeExpansion(expand *exp) {
  for (int i = 0; i < exp->size; i++) {
    free(exp->argsExp[i]);
  }
  free(exp->argsExp);
  free(exp);
}

char *getsVarName(char *str) {
  char *rest = NULL;
  int n;

  errno = 0;
  n = sscanf(str, "%m[a-zA-Z0-9_]", &rest);

  if (errno != 0) {
    perror("sscanf");
  } else if (n == 0) {
    /* free(rest); */
    rest = "";
    return rest;
  }
  return rest;
}
void expandirTilde(char **sMod) {
  if (*(sMod)[0] == '~') {
    char *userName = getsVarName(&sMod[0][1]);
    char *dir;
    char *res;
    if (strcmp(userName, "") == 0) {
      char *sub = getenv("HOME");
      if (sub != NULL) {
        dir = sub;
      } else {
        dir = "";
      }
    } else {
      struct passwd pwd;
      struct passwd *result;
      char buff[2048];
      errno = 0;
      getpwnam_r(userName, &pwd, buff, sizeof(buff), &result);
      if (errno != 0) {
        perror("getpwnam_r");
      } else if (result == NULL) {
        dir = "";
      } else {
        dir = strdup(result->pw_dir);
      }
    }
    int offset = strlen(userName);
    Autosprintf(res, "%s%s", dir, &sMod[0][offset + 1]);
    free(*sMod);
    *sMod = res;
  }
}
void expandirVar(char **sMod) {
  int pos;
  /* int len = strlen(sMod[0]); */
  char *varName;
  char *pre;
  char *val;
  char *sub;
  char *res;
  while ((pos = strCont(*sMod, '$')) && pos > -1) {
    pre = malloc((pos + 1) * sizeof(char));
    snprintf(pre, pos, "%s", *sMod);
    /* strncpy(*sMod, pre, pos - 1); */
    /* pre[pos] = '\0'; */

    varName = getsVarName(&sMod[0][pos]);
    if (strcmp(varName, "") == 0) {
      sub = "";
    } else {
      val = getenv(varName);
      if (val == NULL)
        sub = "";
      else
        sub = val;
    }

    int offset = strlen(varName);
    Autosprintf(res, "%s%s%s", pre, sub, &sMod[0][pos + offset]);
    free(pre);
    free(*sMod);
    *sMod = res;
  }
}
void expandirWildcard(expand *res, char *sMod) {
  if (strCont(sMod, '/') > -1 || strCont(sMod, '?') == -1) {
    addExpasion(res, sMod);
  } else {
    glob_t *gRes;
    int r = glob(sMod, 0, NULL, gRes);
    if (r == GLOB_NOMATCH) {
      addExpasion(res, sMod);
    } else if (r != 0) {
      perror("glob");
    } else {
      for (size_t i = 0; i < gRes->gl_pathc; i++) {
        addExpasion(res, strdup(gRes->gl_pathv[i]));
      }
    }
    globfree(gRes);
  }
}
expand *expandir(char **str) {
  expand *res = calloc(1, sizeof(expand));
  char *s;
  for (int i = 0; (s = str[i]); i++) {
    char *sMod = strdup(s);
    if (!necesitaExpasion(s)) {
      addExpasion(res, sMod); /* NO necesita expansión, se añade tal cual */
    } else {
      expandirTilde(&sMod);
      expandirVar(&sMod);
      expandirWildcard(res, sMod);
    }
    free(sMod);
  }

  addExpasion(res, NULL);
  return res;
}

void pArgsAll() {
  char **cursor = environ;
  while (*cursor) {
    printf("%s\n", *cursor++);
  }
}

void pLimtsAll() {
  struct rlimit *l = malloc(sizeof(struct rlimit));
  getrlimit(RLIMIT_CPU, l);
  printf("cpu\t%ld\n", l->rlim_cur);
  getrlimit(RLIMIT_FSIZE, l);
  printf("fsize\t%ld\n", l->rlim_cur);
  getrlimit(RLIMIT_DATA, l);
  printf("data\t%ld\n", l->rlim_cur);
  getrlimit(RLIMIT_STACK, l);
  printf("stack\t%ld\n", l->rlim_cur);
  getrlimit(RLIMIT_NOFILE, l);
  printf("nofile\t%ld\n", l->rlim_cur);
  getrlimit(RLIMIT_CORE, l);
  printf("core\t%ld\n", l->rlim_cur);
  free(l);
}
int setLimit(char *ref, char *val) {
  int cod;
  if (strcmp(ref, "cpu") == 0) {
    cod = RLIMIT_CPU;
  } else if (strcmp(ref, "fsize") == 0) {
    cod = RLIMIT_FSIZE;
  } else if (strcmp(ref, "data") == 0) {
    cod = RLIMIT_DATA;
  } else if (strcmp(ref, "stack") == 0) {
    cod = RLIMIT_STACK;
  } else if (strcmp(ref, "core") == 0) {
    cod = RLIMIT_CORE;
  } else if (strcmp(ref, "nofile") == 0) {
    cod = RLIMIT_NOFILE;
  } else {
    return 1;
  }
  struct rlimit *l = malloc(sizeof(struct rlimit));
  if (val == NULL) {
    getrlimit(cod, l);
    printf("%s\t%ld\n", ref, l->rlim_cur);
  } else {
    int lim = strtol(val, NULL, 10);
    getrlimit(cod, l);
    l->rlim_cur = lim;
    setrlimit(cod, l);
  }
  free(l);
  return 0;
}

int cd(expand args) {
  if (args.size - 1 == 1) {
    char *dir = getenv("HOME");
    if (dir != NULL) {
      printf("%s\n", dir);
      return chdir(dir);
    }
    return 0;
  } else if (args.size - 1 == 2) {
    int res = chdir(args.argsExp[1]);
    if (res == 0) {
      char *pos = getcwd(NULL, 0);
      printf("%s\n", pos);
      free(pos);
    }
    return res;
  } else {
    fprintf(stderr, "cd: Too many args\n");
    return 1;
  }
}

int set(expand args) {
  if (args.size - 1 == 1) {
    pArgsAll();
    return 0;
  } else if (args.size - 1 == 2) {
    char *res = getenv(args.argsExp[1]);
    if (res == NULL)
      pArgsAll();
    else
      printf("%s=%s\n", args.argsExp[1], res);
    return 0;
  } else if (args.size - 1 == 3) {
    setenv(args.argsExp[1], args.argsExp[2], 1);
    return 0;
  } else {
    fprintf(stderr, "set: Too many args\n");
    return 1;
  }
}

int umaskf(expand args) {
  if (args.size - 1 == 1) {
    mode_t m = umask(0);
    umask(m);
    printf("%o\n", m);
    return 0;
  } else if (args.size - 1 == 2) {
    int o = strtol(args.argsExp[1], NULL, 8);
    mode_t m = umask(o);
    printf("%o\n", m);
    return 0;
  } else {
    fprintf(stderr, "limit: Too many args\n");
    return 1;
  }
}

int limit(expand args) {

  if (args.size - 1 == 1) {
    pLimtsAll();
    return 0;
  } else if (args.size - 1 == 2) {
    setLimit(args.argsExp[1], args.argsExp[2]);
    return 0;
  } else if (args.size - 1 == 3) {
    setLimit(args.argsExp[1], args.argsExp[2]);
    return 0;
  } else {
    fprintf(stderr, "limit: Too many args\n");
    return 1;
  }
}

int gen(expand args) { return execvp(args.argsExp[0], args.argsExp); }

void setIniVars() {
  setenv("prompt", "msh >", 1);
  char *Tbuff;
  Autosprintf(Tbuff, "%d", getpid());
  setenv("mypid", Tbuff, 1);
  free(Tbuff);
  setenv("bgpid", "-1", 1);
  setenv("status", "0", 1);
}

int (*acc[5])(expand) = {&cd, &set, &umaskf, &limit, &gen};

int main(void) {

  sigset_t mGen;
  sigaddset(&mGen, SIGINT);
  sigaddset(&mGen, SIGQUIT);
  sigprocmask(SIG_BLOCK, &mGen, NULL);

  int fd0OG = dup(0);
  int fd1OG = dup(1);
  int fd2OG = dup(2);

  char ***argvv = NULL;
  int argvc;
  char **argv = NULL;
  int argc;
  char *filev[3] = {NULL, NULL, NULL};
  int bg;
  int ret;

  setbuf(stdout, NULL); /* Unbuffered */
  setbuf(stdin, NULL);

  setIniVars();

  expand *args;
  /* int pipa[2]; */
  int prevPipaSalida;
  int sec;
  pid_t hijoSac, nieto, bgpid;
  int nf;
  int status;
  char *prompt;

  while (1) {

    prompt = getenv("prompt");
    if (prompt == NULL) {
      /* I don't think this is possible, but yk, just in case*/
      prompt = "";
    }
    fprintf(stderr, "%s", prompt); /* Prompt */
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
      break; /* EOF */
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0)
      continue; /* Empty line */

    sec = 0;
    int pipa[2] = {-1, -1};

    for (argvc = 0; (argv = argvv[argvc]); argvc++) {

      if (strcmp("cd", argv[0]) == 0) {
        /* acc = &cd; */
        nf = 0;
      } else if (strcmp("set", argv[0]) == 0) {
        /* acc = &set; */
        nf = 1;
      } else if (strcmp("umask", argv[0]) == 0) {
        /* acc = &umask; */
        nf = 2;
      } else if (strcmp("limit", argv[0]) == 0) {
        /* acc = &limit; */
        nf = 3;
      } else {
        /* acc = &gen; */
        nf = 4;
      }

      if (sec)
        prevPipaSalida = pipa[0];
      else {
        prevPipaSalida = -1;
      }

      sec = argvv[argvc + 1] != NULL;
      args = expandir(argv);

      if (sec) {
        if (pipe(pipa) < 0) {
          perror("pipe");
          return 1;
        }
      } else {
        pipa[0] = -1;
        pipa[1] = -1;
      }

      if (nf < 4) {      /* Internal */
        if (sec || bg) { /* Llamar en bg */
          hijoSac = fork();

          if (hijoSac == -1) {
            perror("fork");
            return 1;
          } else if (hijoSac == 0) { /* HijoSac */
            nieto = fork();

            if (nieto == -1) {
              perror("fork");
              return 1;
            } else if (nieto == 0) { /* Nieto */
              if (prevPipaSalida != -1) {
                close(0);
                dup(prevPipaSalida);
                close(prevPipaSalida);
              }
              if (sec) {
                close(1);
                dup(pipa[1]);
                close(pipa[1]);
              }
              acc[nf](*args);
              exit(0);
            } else {                      /* HijoSac */
              if (prevPipaSalida != -1) { /* Idk bt this one */
                close(prevPipaSalida);
              }
              if (sec) {
                close(pipa[0]);
                close(pipa[1]);
              }
              printf("[%d]\n", nieto);
              exit(nieto);
            }

          } else { /* msh */
            wait(&bgpid);
            char *tBuff;
            if (prevPipaSalida != -1) {
              close(prevPipaSalida);
            }
            if (sec) {
              close(pipa[1]);
            }
            Autosprintf(tBuff, "%d", bgpid);
            setenv("bgpid", tBuff, 1);
            free(tBuff);
          }
        } else { /* Llamar en fg */
          if (prevPipaSalida != -1) {
            close(0);
            dup(prevPipaSalida);
            close(prevPipaSalida);
          }
          if (sec) {
            close(1);
            dup(pipa[1]);
            close(pipa[1]);
          }
          int fd;
          if (filev[0]) {
            fd = open(filev[0], O_RDONLY);
            if (fd != -1) {
              close(0);
              dup(fd);
              close(fd);
            } else {
              perror("open");
              exit(1);
            }
          }
          if (filev[1]) {
            fd = creat(filev[1], 0666);
            if (fd != -1) {
              close(1);
              dup(fd);
              close(fd);
            } else {
              perror("creat");
              exit(1);
            }
          }
          if (filev[2]) {
            fd = creat(filev[2], 0666);
            if (fd != -1) {
              close(2);
              dup(fd);
              close(fd);
            } else {
              perror("creat");
              exit(1);
            }
          }

          status = acc[nf](*args);

          close(0);
          close(1);
          close(2);

          dup(fd0OG);
          dup(fd1OG);
          dup(fd2OG);

          char *tBuff;
          Autosprintf(tBuff, "%d", status);
          setenv("status", tBuff, 1);
          free(tBuff);
        }
      } else { /* external */
        hijoSac = fork();

        if (hijoSac == -1) {
          perror("fork");
          return 1;
        } else if (hijoSac == 0) { /* hijoSac */
          if (sec || bg) {         /* Llamar en bg */
            nieto = fork();
            if (nieto == -1) {
              perror("fork");
              return 1;
            } else if (nieto == 0) { /* nieto */
              if (prevPipaSalida != -1) {
                close(0);
                dup(prevPipaSalida);
                close(prevPipaSalida);
              }
              if (sec) {
                close(1);
                dup(pipa[1]);
                close(pipa[1]);
              }
              acc[nf](*args);
              exit(0);
            } else {                      /* hijoSac*/
              if (prevPipaSalida != -1) { /* Idk bt this one */
                close(prevPipaSalida);
              }
              if (sec) {
                close(pipa[0]);
                close(pipa[1]);
              }
              printf("[%d]\n", nieto);
              exit(nieto);
            }
          } else { /* Foreground */
            sigset_t mProc;
            sigemptyset(&mProc);
            sigprocmask(SIG_SETMASK, &mProc, NULL);
            if (prevPipaSalida != -1) {
              close(0);
              dup(prevPipaSalida);
              close(prevPipaSalida);
            }
            if (sec) {
              close(1);
              dup(pipa[1]);
              close(pipa[1]);
            }
            int fd;
            if (filev[0]) {
              fd = open(filev[0], O_RDONLY);
              if (fd != -1) {
                close(0);
                dup(fd);
                close(fd);
              } else {
                perror("open");
                exit(1);
              }
            }
            if (filev[1]) {
              fd = creat(filev[1], 0666);
              if (fd != -1) {
                close(1);
                dup(fd);
                close(fd);
              } else {
                perror("creat");
                exit(1);
              }
            }
            if (filev[2]) {
              fd = creat(filev[2], 0666);
              if (fd != -1) {
                close(2);
                dup(fd);
                close(fd);
              } else {
                perror("creat");
                exit(1);
              }
            }

            status = acc[nf](*args);

            close(0);
            close(1);
            close(2);

            dup(fd0OG);
            dup(fd1OG);
            dup(fd2OG);

            exit(status);
          }
        } else { /* msh */
          int rets;
          wait(&rets);
          if (prevPipaSalida != -1) {
            close(prevPipaSalida);
          }
          if (sec) {
            close(pipa[1]);
          }
          char *tBuff;
          if (bg || sec) {
            Autosprintf(tBuff, "%d", rets);
            setenv("bgpid", tBuff, 1);
          } else {
            Autosprintf(tBuff, "%d", rets);
            setenv("status", tBuff, 1);
          }
          free(tBuff);
        }
      }
      freeExpansion(args);
    }
  }
  printf("\n");
  exit(0);
  return 0;
}
