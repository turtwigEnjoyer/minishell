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
#define HISTORY_SIZE 20

// files in case of redirection
char filev[3][64];

// to store the execvp second parameter
char *argv_execvp[8];
// to store command history

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

int digits(int num) {
  if (num < 10)
    return 1;
  return 1 + digits(num / 10);
}

int min(int a, int b) {
  if (a > b)
    return a;
  return b;
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

void my_print_cmd(struct command cmd) {
  if (cmd.argvv[0] == NULL) {
    fprintf( stderr, "NULL\n");
    return;
  }
  if (cmd.argvv[0][0] == NULL) {
    fprintf( stderr, "NULL\n");
    return;
  }
  int argument = 0;
  while (cmd.argvv[0][argument] != NULL) {
    if (argument != 0) {
      fprintf( stderr, " ");
    }
    fprintf( stderr, "%s", cmd.argvv[0][argument]);
    argument++;
  }
  for (int i = 1; i < cmd.num_commands; i++) {
    fprintf( stderr, " | %s", cmd.argvv[i][0]);
    argument = 1;
    while (cmd.argvv[i][argument] != NULL) {
      fprintf( stderr, " %s", cmd.argvv[i][argument]);
      argument++;
    }
  }
  if (strcmp(cmd.filev[0], "0") != 0) {
    fprintf( stderr, " < %s", filev[0]);
  }
  if (strcmp(cmd.filev[1], "0") != 0) {
    fprintf( stderr, " > %s", filev[1]);
  }
  if (strcmp(cmd.filev[2], "0") != 0) {
    fprintf( stderr, " !> %s", filev[2]);
  }
  if (cmd.in_background == 1) {
    fprintf( stderr, " &");
  }
  fprintf( stderr, "\n");
  return;
}

struct command history[HISTORY_SIZE];
int history_iterator = 0;
int counter = 0;          // to know the number of sequences executed

int head = 0;
int tail = 0;
int n_elem = 0;

void siginthandler(int param) {
  printf("\n****  Exiting MSH **** \n");
  for (int i = 0; i < min(HISTORY_SIZE,counter); i++) {
    
    free_command(&history[i]);
  }
  // signal(SIGINT, siginthandler);
  exit(0);
}

// Checks for correct number of commands
int is_valid_counter(int command_counter) {
  if (command_counter == 0) {
    // No commands -> ask user again
    return -1;
  }
  if (command_counter > MAX_COMMANDS) {
    printf("Error: Maximum number of commands is %d \n", MAX_COMMANDS);
    return -1;
  }
  return 1;
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
// Checks possible command redirection errors
void run_my_history(char ***argvv, int counter, int history_it);
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

void is_valid_open_file(char ***argvv, char filev[3][64], int degree,
                        int num_command) {
  if (strcmp(filev[0], "0") != 0 && degree == num_command - 1) {
    if (close(STDIN_FILENO) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
    if (open(filev[0], O_RDONLY) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
  }
  if (strcmp(filev[1], "0") != 0 && degree == 0) {
    if (close(STDOUT_FILENO) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
    if (open(filev[1], O_RDWR | O_TRUNC | O_CREAT, 0664) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
  }
  if (strcmp(filev[2], "0") != 0 && degree == 0) {
    if (close(STDERR_FILENO) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
    if (open(filev[2], O_RDWR | O_TRUNC | O_CREAT, 0664) == -1) {
      perror("Error opening file\n");
      exit(-1);
    }
  }
  if (execvp(argvv[num_command - degree - 1][0],
             argvv[num_command - degree - 1]) == -1) {
    perror("Error executing command\n");
    exit(-1);
  }
}

void executeCommand(char ***argvv, int in_background, int num_command,
                    char filev[3][64]) {
  int pid, degree = 0, status;
  for (int i = 0; i < num_command - 1; i++) {
    int p[2];
    pipe(p);
    pid = fork();
    if (pid == -1) {
      perror("Error in fork\n");
      exit(-1);
    } else if (pid == 0) { // child
      degree++; // this degree is independent and is only increased by child
      if (close(p[0]) == -1 || close(1) == -1 || dup(p[1]) == -1) {
        perror("Error connecting pipes\n");
        exit(-1);
      }

    } else { // parent

      waitpid(pid, &status, 0);
      if (status != 0) {
        exit(-1);
      }
      if (close(p[1]) == -1 || close(0) == -1 || dup(p[0]) == -1) {
        perror("Error connecting pipes\n");
        exit(-1);
      }
      break;
    }
  }

  is_valid_open_file(argvv, filev, degree, num_command);
  exit(0); // nothing failed
}

void start_command(char ***argvv, int in_background, char filev[3][64], int command_counter) {

  int shell_fork_id = fork();
  if (shell_fork_id == -1) {
    perror("Error in fork\n");
  } else if (shell_fork_id == 0) {
    executeCommand(argvv, in_background, command_counter, filev);
  } else if (in_background == 1) {
    printf("Background process id: %d\n", shell_fork_id);
    return;
  }
  waitpid(shell_fork_id, NULL, 0);
}

void valid_command( char ***argvv, int in_background, char filev[3][64], int command_counter){
  if (is_valid_counter(command_counter) < 0)
      return;

  if (strcmp(argvv[0][0], "myhistory") == 0) {
    run_my_history(argvv, counter, history_iterator);
  } else if (strcmp(argvv[0][0], "mycalc") == 0) {
    if (command_counter != 1) {
      printf("[ERROR] Command mycalc does not allow redirections\n");
    } else {
      mycalc(argvv);
    }
  } else {
    start_command(argvv, in_background, filev, command_counter);
  }
  // printf("storing command\n");

  store_command(argvv, filev, in_background, &(history[history_iterator]));
  counter++;
  history_iterator = (history_iterator + 1) % HISTORY_SIZE;
}


void myhist_no_args(int counter, int history_it) {
  // Should only print the last commands into STDERR

  int begin = 0;

  if (counter >= HISTORY_SIZE) {
    // If we have 20 commands in history, the 20th ago command is stored one
    // after our iterator (We have an array size 20, which will overwrite
    // commands as it fills up)
    begin = (history_it + 1) % HISTORY_SIZE;
    int count = 0;
    fprintf(stderr, "%d ", count);
    my_print_cmd(history[history_it]);
    for (int i = begin; i != history_it; i = (i + 1) % HISTORY_SIZE) {
      count++;
      fprintf(stderr, "%d ", count);
      my_print_cmd(history[i]);
    }
  } else {
    // if we haven't reached the maximum we print from the first untill the last
    // stored command
    for (int i = 0; i < counter; i++) {
      fprintf(stderr, "%d ", i);
      my_print_cmd(history[i]);
    }
  }
}

void run_my_history(char ***argvv, int counter, int history_it){

  if ( argvv[0][1] == NULL){
    myhist_no_args(counter, history_it);
    return;
  }
  
  // Run command at inputted number
  // Command 19 is prev, command 0 is 20 commands ago
  int wrong = 0;
  int num = myatoi(argvv[0][1], &wrong);
  if (wrong == 1 || argvv[0][2] != NULL) {
    printf("ERROR: Command structure is: \n myhistory <Num (optional) > \n");
    return;
  }
  if (num < 0 || num > HISTORY_SIZE ||
      num >= counter) {
    printf("ERROR: Command not found\n");
    return;
  }
  //Get position of command in history[]
  int begin = 0;
  if( counter > HISTORY_SIZE){
    begin = (history_it+1)%HISTORY_SIZE;
  }
  int pos = (begin + num)%HISTORY_SIZE;
  char newFilev[3][64];
  fprintf( stderr, "Running command %d\n", num);
  valid_command(history[pos].argvv, history[pos].in_background, history[pos].filev, history[pos].num_commands);
}



/**
 * Main sheell  Loop
 */
int main(int argc, char *argv[]) {
  setenv("Acc", "0", 0);
  int shell_fork_id; // for queue access
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

  int run_history = 0;

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

    valid_command( argvv, in_background, filev, command_counter);
  }

  return 0;
}
