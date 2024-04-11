// P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

// #include "parser.h"
#include <fcntl.h>
#include <signal.h>
#include <stddef.h> /* NULL */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wait.h>

#define MAX_COMMANDS 8

// files in case of redirection
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];

void siginthandler(int param) {
  printf("\n****  Exiting MSH **** \n");

  // signal(SIGINT, siginthandler);
  exit(0);
}

/* myhistory */

/* myhistory */

struct command {
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 5;
struct command *history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd) {
  if ((*cmd).argvv != NULL) {
    char **argv;
    for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++) {
      for (argv = *(*cmd).argvv; argv && *argv; argv++) {
        if (*argv) {
          free(*argv);
          *argv = NULL;
        }
      }
    }
  }
  free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background,
                   struct command *cmd) {
  int num_commands = 0;
  while (argvv[num_commands] != NULL) {
    num_commands++;
  }

  for (int f = 0; f < 3; f++) {
    if (strcmp(filev[f], "0") != 0) {
      strcpy((*cmd).filev[f], filev[f]);
    } else {
      strcpy((*cmd).filev[f], "0");
    }
  }

  (*cmd).in_background = in_background;
  (*cmd).num_commands = num_commands - 1;
  (*cmd).argvv = (char ***)calloc((num_commands), sizeof(char **));
  (*cmd).args = (int *)calloc(num_commands, sizeof(int));

  for (int i = 0; i < num_commands; i++) {
    int args = 0;
    while (argvv[i][args] != NULL) {
      args++;
    }
    (*cmd).args[i] = args;
    (*cmd).argvv[i] = (char **)calloc((args + 1), sizeof(char *));
    int j;
    for (j = 0; j < args; j++) {
      (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]), sizeof(char));
      strcpy((*cmd).argvv[i][j], argvv[i][j]);
    }
  }
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char ***argvv, int num_command) {
  // reset first
  for (int j = 0; j < 8; j++)
    argv_execvp[j] = NULL;

  int i = 0;
  for (i = 0; argvv[num_command][i] != NULL; i++)
    argv_execvp[i] = argvv[num_command][i];
}

//  STUDENT FUNCTIONS
void executeCommand(char ***argvv, int in_background, int num_command,
                    char filev[3][64]) {
  int pid, degree = 0, status;
  for (int i = 0; i < num_command - 1; i++) {
    int p[2];
    pipe(p);
    pid = fork();
    if (pid == -1) {
      perror("Error in fork");
      exit(-1);
    } else if (pid == 0) {
      degree++; // this degree is independent and is only increased by child
      close(p[0]);
      close(1);
      dup(p[1]);
    } else {
      close(p[1]);
      close(0);
      dup(p[0]);
      wait(NULL);
      break;
    }
  }

  if (strcmp(filev[0], "0") != 0 && degree == num_command - 1) {
    close(STDIN_FILENO);
    open(filev[0], O_RDONLY);
  }
  if (strcmp(filev[1], "0") != 0 && degree == 0) {
    close(STDOUT_FILENO);
    open(filev[1], O_RDWR | O_TRUNC | O_CREAT, 0664);
  }
  if (strcmp(filev[2], "0") != 0 && degree == 0) {
    close(STDERR_FILENO);
    open(filev[2], O_RDWR | O_TRUNC | O_CREAT, 0664);
  }
  execvp(argvv[num_command - degree - 1][0], argvv[num_command - degree - 1]);
  exit(0);
}

int digits(int num) {
  if (num < 10)
    return 1;
  return 1 + digits(num / 10);
}

int myatoi(char *str, int *wrong) {
  int num, length;
  num = atoi(str);
  length = digits(abs(num));
  char *strnum = malloc(16);
  sprintf(strnum, "%d", num);
  if (strcmp(strnum, str) != 0) {
    if (strlen(str) != length) {
      if (str[0] != '0' && str[0] != '-') {
        *wrong = 1;
        return num;
      } else {
        for (int i = 1; i < strlen(str) - length; i++) {
          if (str[i] != '0') {
            *wrong = 1;
            return num;
          }
        }
      }
    }
    if (num == 0) {
      if (str[0] != '0' && str[0] != '-') {
        *wrong = 1;
        return num;
      } else {
        for (int i = 1; i < strlen(str); i++) {
          if (str[i] != '0') {
            *wrong = 1;
            return num;
          }
        }
      }
    }
  }
  free(strnum);
  return num;
}

void mycalc(char ***argvv) {
  // check we have the right amount of args
  if (argvv[0][1] == NULL || argvv[0][2] == NULL || argvv[0][3] == 0 ||
      argvv[0][4] != NULL) {
    printf("[ERROR] The structure of the command is mycalc <operand_1> "
           "<add/mul/div> <operand_2>\n");
    return;
  }
  // check the operation
  if ((strcmp("add", argvv[0][2]) != 0) && (strcmp("mul", argvv[0][2]) != 0) &&
      (strcmp("div", argvv[0][2]) != 0)) {
    printf("[ERROR] The structure of the command is mycalc <operand_1> "
           "<add/mul/div> <operand_2>\n");
    return;
  }

  int operand1, operand2, wrong = 0;
  operand1 = myatoi(argvv[0][1], &wrong);
  operand2 = myatoi(argvv[0][3], &wrong);
  char *str = malloc(96);
  if (wrong == 1) {
    printf("[ERROR] The structure of the command is mycalc <operand_1> "
           "<add/mul/div> <operand_2>\n");
    return;
  }
  if (strcmp("mul", argvv[0][2]) == 0) {
    sprintf(str, "[OK] %d * %d = %d\n", operand1, operand2,
            operand1 * operand2);
    write(STDERR_FILENO, str, strlen(str));
  }
  if (strcmp("div", argvv[0][2]) == 0) {
    if (operand2 == 0) {
      printf("[ERROR] Division by 0 not allowed\n");
      wrong = 1;
    } else {
      sprintf(str, "[OK] %d / %d = %d; Remainder %d\n", operand1, operand2,
              operand1 / operand2, operand1 % operand2);
      write(STDERR_FILENO, str, strlen(str));
    }
  }
  if (strcmp("add", argvv[0][2]) == 0) {
    int acc;
    acc = atoi(getenv("Acc"));
    acc += operand1 + operand2;
    sprintf(str, "[OK] %d + %d = %d; Acc %d\n", operand1, operand2,
            operand1 + operand2, acc);
    write(STDERR_FILENO, str, strlen(str));
    char *s_acc = malloc(16);
    sprintf(s_acc, "%d", acc);
    setenv("Acc", s_acc, 1);
    free(s_acc);
  }
  free(str);
  return;
}

void my_print_cmd(struct command cmd) {
  if (cmd.argvv[0] == NULL) {
    printf("NULL\n");
    return;
  }
  if (cmd.argvv[0][0] == NULL) {
    printf("NULL\n");
    return;
  }
  int argument = 0;
  while (cmd.argvv[0][argument] != NULL) {
    if (argument != 0) {
      printf(" ");
    }
    printf("%s", cmd.argvv[0][argument]);
    argument++;
  }
  for (int i = 1; i < cmd.num_commands; i++) {
    printf(" | %s", cmd.argvv[i][0]);
    argument = 1;
    while (cmd.argvv[i][argument] != NULL) {
      printf(" %s", cmd.argvv[i][argument]);
      argument++;
    }
  }
  if (strcmp(cmd.filev[0], "0") != 0) {
    printf(" < %s", filev[0]);
  }
  if (strcmp(cmd.filev[1], "0") != 0) {
    printf(" > %s", filev[1]);
  }
  if (strcmp(cmd.filev[2], "0") != 0) {
    printf(" !> %s", filev[2]);
  }
  if (cmd.in_background == 1) {
    printf(" &");
  }
  printf("\n");
  return;
}

void deadChildHandler(int sig) {
  // When childs finish their execution, we reap them as to not leave zombies
  // printf(" A child has finished, reaping...\n");
  wait(NULL);
}
/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[]) {
  setenv("Acc", "0", 0);
  int shell_fork_id;
  int history_iterator = 0; // for queue access
  int counter = 0;          // to know the number of sequences executed
  int wrong = 0;            // to communicate errors between calls
  /**** Do not delete this code.****/
  int end = 0;
  int executed_cmd_lines = -1;
  char *cmd_line = NULL;
  char *cmd_lines[10];

  if (!isatty(STDIN_FILENO)) {
    cmd_line = (char *)malloc(100);
    while (scanf(" %[^\n]", cmd_line) != EOF) {
      if (strlen(cmd_line) <= 0)
        return 0;
      cmd_lines[end] = (char *)malloc(strlen(cmd_line) + 1);
      strcpy(cmd_lines[end], cmd_line);
      end++;
      fflush(stdin);
      fflush(stdout);
    }
  }

  /*********************************/

  char ***argvv = NULL;
  int num_commands;

  history = (struct command *)malloc(history_size * sizeof(struct command));
  int run_history = 0;

  /* STUDENT CODE: handling children finishing their processes */
  struct sigaction sa;
  sa.sa_handler = deadChildHandler;
  sa.sa_flags = 0;
  sigemptyset(&(sa.sa_mask));
  sigaction(SIGCHLD, &sa, NULL);

  while (1) {
    int status = 0;
    int command_counter = 0;
    int in_background = 0;
    signal(SIGINT, siginthandler);

    if (run_history) {
      run_history = 0;
    } else {
      // Prompt
      write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

      // Get command
      //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN
      // NORMAL/CORRECTION MODE***************
      executed_cmd_lines++;
      if (end != 0 && executed_cmd_lines < end) {
        command_counter = read_command_correction(
            &argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
      } else if (end != 0 && executed_cmd_lines == end)
        return 0;
      else
        command_counter =
            read_command(&argvv, filev, &in_background); // NORMAL MODE
    }
    //************************************************************************************************

    /************************ STUDENTS CODE ********************************/
    if (command_counter > 0) {
      if (command_counter > MAX_COMMANDS) {
        printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
      } else {
        // Print command
        // print_command(argvv, filev);

        if (strcmp(argvv[0][0], "myhistory") == 0 && argvv[0][1] == NULL) {
          if (counter < history_size) {
            for (int i = 0; i < counter; i++) {
              // struct command cmd;
              // store_command(history[i].argvv, history[i].filev,
              //               history[i].in_background, &cmd);
              printf("%d \n", i);
              // my_print_cmd(cmd);
              // free_command(&cmd);
            }
          } else {
            for (int i = 0; i < history_size; i++) {
              // struct command cmd;
              // store_command(
              //     history[(history_iterator + i) % history_size].argvv,
              //     history[(history_iterator + i) % history_size].filev,
              //     history[(history_iterator + i) %
              //     history_size].in_background, &cmd);
              printf("%d \n", i);
              // my_print_cmd(cmd);
              // free_command(&cmd);
            }
          }
        } else {
          counter++;
          if (strcmp(argvv[0][0], "myhistory") == 0) {
            // change argvv to command
          }
          if (strcmp(argvv[0][0], "mycalc") == 0) {
            if (command_counter != 1) {
              printf("[ERROR] Command mycalc does not allow redirections\n");
            } else {
              mycalc(argvv);
            }
          } else {
            shell_fork_id = fork();
            if (shell_fork_id == 0) {
              executeCommand(argvv, in_background, command_counter, filev);
            } else if (in_background == 0) {
              wait(NULL);
            }
          }
          // printf("storing command\n");
          // free_command(&history[history_iterator]);
          // store_command(argvv, filev, in_background,
          //               &(history[history_iterator]));
          // history_iterator = (history_iterator + 1) % history_size;
          // printf("command stored\n");
        }
      }
    }
  }

  return 0;
}
