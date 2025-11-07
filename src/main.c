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
#include <unistd.h>
#include <string.h>
extern int obtain_order(char ****argvvp, char *filep[3],
                        int *bgp); /* See parser.y for description */

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
    char path2[200];
    getcwd(path2, 200);

    fprintf(stderr, "\n %s %s", path2,"Ψ >"); /* Prompt */
    ret = obtain_order(&argvv, filev, &bg);
    if (ret == 0)
 //     break; /* EOF */
         continue;
    if (ret == -1)
      continue;      /* Syntax error */
    argvc = ret - 1; /* Line */
    if (argvc == 0) /* ERR */

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
    argvc = 0;
    for (; (argv = argvv[argvc]); argvc++) {
      argc = 0;
        printf("%s ->", argv[argc]);
          if(strcmp(argv[0],"pwd") == 0){

          char path[200];
        
            getcwd(path, 200);
            printf("Current working directory: %s", path);
          }
          else if (strcmp(argv[0], "cd") == 0) {
            char path[200];
            for (int i = 0;i < 200;i++) {
              path[i] = '\0';
            }
            if(!argv[argvc + 1]){
              chdir(getenv("HOME"));

            }
            else{
                if (!strchr(argv[1],'~')) {
                    strcpy(path,argv[1]);
                  }
                else if (strchr(argv[1],'~') != strrchr(argv[1],'~')) {
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

                 }   
              
               


                 if(chdir(path) == -1){
                  fprintf(stderr,"ERROR al buscar %s , no existe ese directorio ",path);
                  }
              break;
            }
                
            getcwd(path, 200);
            printf("Current working directory: %s", path);

        }
        
              printf("\n");
    }
  //exit(0);
 // return 0;
 } 
}
