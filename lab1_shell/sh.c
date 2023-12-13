#include "list.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PERM (0644)   /* default permission rw-r--r-- */
#define MAXBUF (512)  /* max length of input line. */
#define MAX_ARG (100) /* max number of cmd line arguments. */

typedef enum {
  AMPERSAND, /* & */
  NEWLINE,   /* end of line reached. */
  NORMAL,    /* file name or command option. */
  INPUT,     /* input redirection (< file) */
  OUTPUT,    /* output redirection (> file) */
  PIPE,      /* | for instance: ls *.c | wc -l */
  SEMICOLON  /* ; */
} token_type_t;

static char *progname;             /* name of this shell program. */
static char input_buf[MAXBUF];     /* input is placed here. */
static char token_buf[2 * MAXBUF]; /* tokens are placed here. */
static char *input_char;           /* next character to check. */
static char *token;                /* a token such as /bin/ls */

static list_t* path_dir_list;             /* list of directories in PATH. */
static list_t* background_processes_list; /* list of background processes. */
static list_t* piping_processes_list;     /* list of piping processes. */
static char previous_dir[MAXBUF];         /* previsous directory. */
static int input_fd;                      /* for i/o redirection or pipe. */
static int output_fd;                     /* for i/o redirection or pipe */
static int read_pipe[2];                  /* pipe for "reading" command. */
static bool read_from_pipe = false;       /* True if read from pipe. */

/* fetch_line: read one line from user and put it in input_buf. */
int fetch_line(char *prompt) {
  int c;
  int count;
  char current_dir[MAXBUF];

  input_char = input_buf;
  token = token_buf;
  if (!getcwd(current_dir, MAXBUF)) {
    current_dir[0] = 0;
  }

  printf("%s%s", current_dir, prompt);
  fflush(stdout);

  count = 0;

  for (;;) {

    c = getchar();

    if (c == EOF)
      return EOF;

    if (count < MAXBUF)
      input_buf[count++] = c;

    if (c == '\n' && count < MAXBUF) {
      input_buf[count] = 0;
      return count;
    }

    if (c == '\n') {
      printf("too long input line\n");
      return fetch_line(prompt);
    }
  }
}

/* end_of_token: true if character c is not part of previous token. */
static bool end_of_token(char c) {
  switch (c) {
  case 0:
  case ' ':
  case '\t':
  case '\n':
  case ';':
  case '|':
  case '&':
  case '<':
  case '>':
    return true;

  default:
    return false;
  }
}

/* gettoken: read one token and let *outptr point to it. */
int gettoken(char **outptr) {
  token_type_t type;

  *outptr = token;

  while (*input_char == ' ' || *input_char == '\t')
    input_char++;

  *token++ = *input_char;

  switch (*input_char++) {
  case '\n':
    type = NEWLINE;
    break;

  case '<':
    type = INPUT;
    break;

  case '>':
    type = OUTPUT;
    break;

  case '&':
    type = AMPERSAND;
    break;

  case '|':
    type = PIPE;
    break;

  case ';':
    type = SEMICOLON;
    break;

  default:
    type = NORMAL;

    while (!end_of_token(*input_char))
      *token++ = *input_char++;
  }

  *token++ = 0; /* null-terminate the string. */

  return type;
}

/* error: print error message using formatting string similar to printf. */
void error(char *fmt, ...) {
  va_list ap;

  fprintf(stderr, "%s: error: ", progname);

  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);

  /* print system error code if errno is not zero. */
  if (errno != 0) {
    fprintf(stderr, ": ");
    perror(0);
  } else
    fputc('\n', stderr);
}

static void add_to_process_list(list_t** process_list, pid_t pid, bool print) {
  pid_t* pid_p = (pid_t*)calloc(1, sizeof(pid_t));
  
  *pid_p = pid;
  insert_last(process_list, (void*)pid_p);
  if (print) {
    printf("Pid+ %d\n", pid);
  }
}

static void add_background_process(pid_t pid) {
  add_to_process_list(&background_processes_list, pid, true);
}

static void add_piping_process(pid_t pid) {
  add_to_process_list(&piping_processes_list, pid, false);
}

static void wait_for_processes(list_t** process_list, bool print) {
  int processes = length(*process_list);
  int wstatus = 0;

  for (int i = 0; i < processes; i++) {
    pid_t* pid_p = (pid_t*)remove_first(process_list);
    pid_t pid = *pid_p;
    pid_t result = waitpid(pid, &wstatus, WNOHANG);
    if (result == pid || result == -1) {
      if (print) {
        printf("Pid- %d\n", pid);
      }
      // Free the memory hold by pid.
      free(pid_p);
    } else {
      /* Add back to the background process list. */
      insert_last(process_list, (void*)pid_p);
    }
  }
}

static void wait_for_background_processes() {
  wait_for_processes(&background_processes_list, true);
  wait_for_processes(&piping_processes_list, false);
}

static void exit_command() {
  free_list(&path_dir_list);
  free_list(&background_processes_list);
  free_list(&piping_processes_list);
  exit(0);
}

void cd_command(char **argv, int argc) {
  char current_dir[MAXBUF];
  bool changed = false;
  getcwd(current_dir, MAXBUF);

  if (argc > 2) {
    printf("sh: %s: to many arguments\n", argv[0]);
  } else if (argc == 1) {
    char* home_dir = getenv("HOME");
    if (home_dir && !chdir(home_dir)) {
      changed = true;
    }
  } else if (strcmp(argv[1], "-") == 0 && chdir(previous_dir) == 0) {
    printf("%s\n", previous_dir);
    changed = true;
  } else if (chdir(argv[1]) == 0) {
    changed = true;
  } else {
    printf("sh: %s: %s: No such file or directory\n", argv[0], argv[1]);
  }

  if (changed) {
    strcpy(previous_dir, current_dir);
  }
}

/* run_program: fork and exec a program. */
void run_program(char **argv, int argc, bool foreground, bool doing_pipe) {
  /* you need to fork, search for the command in argv[0],
   * setup stdin and stdout of the child process, execv it.
   * the parent should sometimes wait and sometimes not wait for
   * the child process (you must figure out when). if foreground
   * is true then basically you should wait but when we are
   * running a command in a pipe such as PROG1 | PROG2 you might
   * not want to wait for each of PROG1 and PROG2...
   *
   * hints:
   *  snprintf is useful for constructing strings.
   *  access is useful for checking wether a path refers to an
   *      executable program.
   *
   *
   */

  /* Check if exit command. */
  if (!strcmp(argv[0], "exit")) {
    exit_command();
  /* Check if cd command. */
  } else if (!strcmp(argv[0], "cd")) {
    cd_command(argv, argc);
  /* Other commands. */
  } else {
    /* Create a path of first argument, and test if it is runnable. */
    char pathname[2 * (MAXBUF + 1)];
    strcpy(pathname, argv[0]);
    list_t* dir = path_dir_list;
    unsigned paths = length(path_dir_list);

    while (true) {
      /* Check if file exists. */
      if ((pathname[0] == '/' || pathname[0] == '.') && access(pathname, F_OK) == 0) {
        /* Check if allowed to execute. */
        if (access(pathname, X_OK) == 0) {
          /* The pathname is executable.   */
          /* Fork to create a new process. */
          int write_pipe[2];
          if (doing_pipe && pipe(write_pipe) == -1) {
            error("error when creating pipe");
            break;
          }

          /* Fork to create a new process. */
          pid_t child_pid = fork();
          if (child_pid) {
            /* We are in the parent process.   */
            /* Add background process to list. */
            if (doing_pipe) {
              add_piping_process(child_pid);
              /* Close the write end of the pipe. */
              close(write_pipe[1]);
              read_from_pipe = true;
              read_pipe[0] = write_pipe[0];
              read_pipe[1] = write_pipe[1];
            } else {
              read_from_pipe = false;
              if (!foreground) {
                add_background_process(child_pid);
              }
            }

            if (foreground) {
              int wstatus = 0;
              pid_t pid = waitpid(child_pid, &wstatus, 0);
              if (pid != child_pid) {
                printf("%s terminated abnormally", argv[0]);
              }
            }
            break;
          } else {
            /* We are in child process. */
            if (input_fd) {
              close(STDIN_FILENO);
              dup2(input_fd, STDIN_FILENO);
            }
            if (output_fd) {
              close(STDOUT_FILENO);
              dup2(output_fd, STDOUT_FILENO);
            }
            if (doing_pipe) {
              /* Close the read end of the pipe. */
              close(write_pipe[0]);
              close(STDOUT_FILENO);
              dup2(write_pipe[1], STDOUT_FILENO);
            }
            if (read_from_pipe) {
              /* Close the write end of the pipe. */
              close(read_pipe[1]);
              close(STDIN_FILENO);
              dup2(read_pipe[0], STDIN_FILENO);
            }
            execv(pathname, argv);
          }
        } else {
          printf("sh: %s: Permission denied\n", argv[0]);
          break;
        }
      } else {
        if (paths > 0) {
          /* Create a new path. */
          int len = strlen(dir->data);
          strcpy(pathname, dir->data);
          pathname[len] = '/';
          strcpy(&pathname[len + 1], argv[0]);
          --paths;
          dir = dir->succ;
        } else {
          printf("%s: command not found\n", argv[0]);
          break;
        }
      }
    }
  }
}

void parse_line(void) {
  char *argv[MAX_ARG + 1];
  int argc;
  token_type_t type;
  bool foreground;
  bool doing_pipe;

  input_fd = 0;
  output_fd = 0;
  argc = 0;

  for (;;) {
    wait_for_background_processes();
    foreground = true;
    doing_pipe = false;

    type = gettoken(&argv[argc]);

    switch (type) {
    case NORMAL:
      argc += 1;
      break;

    case INPUT:
      type = gettoken(&argv[argc]);
      if (type != NORMAL) {
        error("expected file name: but found %s", argv[argc]);
        return;
      }

      input_fd = open(argv[argc], O_RDONLY);

      if (input_fd < 0)
        error("cannot read from %s", argv[argc]);

      break;

    case OUTPUT:
      type = gettoken(&argv[argc]);
      if (type != NORMAL) {
        error("expected file name: but found %s", argv[argc]);
        return;
      }

      output_fd = open(argv[argc], O_CREAT | O_WRONLY, PERM);

      if (output_fd < 0)
        error("cannot write to %s", argv[argc]);
      break;

    case PIPE:
      doing_pipe = true;
      /*FALLTHROUGH*/

    case AMPERSAND:
      foreground = false;
      /*FALLTHROUGH*/

    case NEWLINE:
    case SEMICOLON:
      if (argc == 0)
        return;

      argv[argc] = NULL;

      run_program(argv, argc, foreground, doing_pipe);

      input_fd = 0;
      output_fd = 0;
      argc = 0;

      if (type == NEWLINE)
        return;

      break;
    }
  }
}

/* init_search_path: make a list of directories to look for programs in. */
static void init_search_path(void) {
  char *dir_start;
  char *path;
  char *s;
  list_t *p;
  bool proceed;

  path = getenv("PATH");

  /* path may look like "/bin:/usr/bin:/usr/local/bin"
   * and this function makes a list with strings
   * "/bin" "usr/bin" "usr/local/bin"
   *
   */

  dir_start = malloc(1 + strlen(path));
  if (dir_start == NULL) {
    error("out of memory.");
    exit(1);
  }

  strcpy(dir_start, path);

  path_dir_list = NULL;

  if (path == NULL || *path == 0) {
    path_dir_list = new_list("");
    return;
  }

  proceed = true;

  while (proceed) {
    s = dir_start;
    while (*s != ':' && *s != 0)
      s++;
    if (*s == ':')
      *s = 0;
    else
      proceed = false;

    insert_last(&path_dir_list, dir_start);

    dir_start = s + 1;
  }

  p = path_dir_list;

  if (p == NULL)
    return;

#if 0
	do {
		printf("%s\n", (char*)p->data);
		p = p->succ;
	} while (p != path_dir_list);
#endif
}

static void init_directories() {
  if (!getcwd(previous_dir, MAXBUF)) {
    previous_dir[0] = 0;
  }
}

static void init_background_processes() {
  background_processes_list = NULL;
  piping_processes_list = NULL;
}

/* main: main program of simple shell. */
int main(int argc, char **argv) {
  char *prompt = (argc >= 2 && !strncmp(argv[1], "-n", 3)) ? "" : "% ";

  progname = argv[0];

  init_search_path();
  init_directories();
  init_background_processes();

  while (fetch_line(prompt) != EOF)
    parse_line();

  return 0;
}