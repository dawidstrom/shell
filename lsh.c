/* 
 * Main source code file for lsh shell program
 *
 * You are free to add functions to this file.
 * If you want to add functions in a separate file 
 * you will need to modify Makefile to compile
 * your additional functions.
 *
 * Add appropriate comments in your code to make it
 * easier for us while grading your assignment.
 *
 * Submit the entire lab1 folder as a tar archive (.tgz).
 * Command to create submission archive: 
      $> tar cvf lab1.tgz lab1/
 *
 * All the best 
 */


#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "parse.h"

//Custom includes
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/errno.h>


/*
 * Function declarations
 */

void PrintCommand(int, Command *);
void PrintPgm(Pgm *);
void stripwhite(char *);

int execute(Command *);
void exec_rec(Pgm*, int, int, int*, int);
void sigint_handler(int);
void sigchld_handler(int);

int pids[256];
int pi = 0;

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Description:
 * Kills all running foreground processes.
 */
void
sigint_handler(int sig)
{
	pi--;
	for ( pi; pi >= 0; pi-- )
	{
		kill(SIGTERM, pids[pi]);
	}
	pi++;
}

/*
 * Description:
 * Reaps background process so it doesn't become a zombie.
 */
void
sigchld_handler(int sig)
{
	waitpid(-1, NULL, WNOHANG);
}

/*
 * Name: main
 * 
 * Description:
 * Gets the ball rolling...
 */
int main(void)
{
  Command cmd;
  int n;

  // Setup handlers for CTRL+C and background processes.
  signal(SIGINT, sigint_handler);
  signal(SIGCHLD, sigchld_handler);

  while (!done) {

    char *line;
    line = readline("> ");

    if (!line) {
      /* Encountered EOF at top level */
      done = 1;
    }
    else {
      /*
       * Remove leading and trailing whitespace from the line
       * Then, if there is anything left, add it to the history list
       * and execute it.
       */
      stripwhite(line);

      if(*line) {
        add_history(line);
        /* execute it */
        n = parse(line, &cmd);
		if (n != -1)
			execute(&cmd);
        PrintCommand(n, &cmd);
      }
    }
    
    if(line) {
      free(line);
    }
  }
  return 0;
}

/* 
 * Name: execute
 *
 * Parameters:
 * cmd - Pointer to the command being executed.
 * 
 * Description:
 * Executes a single command. (see Command)
 */
int
execute(Command* cmd)
{
	int fdin, fdout;

	// set fdin and fdout to STDIN and STDOUT if no redirect is given.
	fdin = cmd->rstdin == NULL ? STDIN_FILENO : open(cmd->rstdin, O_RDONLY);
	fdout = cmd->rstdout == NULL ? STDOUT_FILENO : open(cmd->rstdout, O_WRONLY|O_CREAT);

	// Keep track of the number of processes running in foreground.
	int zero = 0;
	int* size = &zero; 

	// Recursivly execute all programs.
	exec_rec(cmd->pgm, fdin, fdout, size, cmd->bakground);

	// Block untill all foreground processes have returned.
	for ( int i = *size; i>0; i-- )
	{
		wait(NULL);
	}

	// Set number of processes running in foreground to 0. (All have exited)
	pi = 0;
	return 1;
}

/*
 * Name: exec_rec
 *
 * Parameters:
 * pgm   - The program to execute.
 * fdin  - The file descriptor program should read input from.
 * fdout - The file descriptor program should write output to.
 * size  - Counter for number of foreground processes spawned.
 * bg    - Flag used to indicate processes should be spawned in background.
 *
 * Description: 
 * Recursivly spawns all the program pointed to by pgm, reads input from 
 * fdin and writes output to fdout.
 */
void
exec_rec(Pgm* pgm, int fdin, int fdout, int* size, int bg)
{
	// Only count foreground processes.
	*size = bg ? 0 : (*size)+1;

	// Handle built-in commands.
	if ( strcmp(pgm->pgmlist[0], "exit") == 0 )
	{
		exit(0);
	}
	else if ( strcmp(pgm->pgmlist[0], "cd") == 0 )
	{
		if ( chdir(pgm->pgmlist[1]) == -1 )
			perror(pgm->pgmlist[1]);
		return;
	}

	int fd[2];
	pipe(fd);
		
	// Recursivly spawn programs if not the last program.
	if ( pgm->next != NULL )
	{
		exec_rec(pgm->next, fdin, fd[1], size, bg);
		close(fd[1]);
	}

	int pid = fork();
	
	// Spawn process which executes the given command.
	if ( pid == 0 )
	{
		// Do not propagate signals to baCkground processes.
		if ( bg )
			setpgid(0,0);

		// Connect input/output properly of spawned processes.
		if ( pgm->next != NULL )
			dup2(fd[0], STDIN_FILENO);
		else
			dup2(fdin, STDIN_FILENO);
		dup2(fdout, STDOUT_FILENO);
		close(fd[0]);

		// Execute program and handle errors.
		if ( execvp(pgm->pgmlist[0], pgm->pgmlist) )
		{
			perror(pgm->pgmlist[0]);
			exit(0);	
		}
	}

	// Only store pid of foreground processes.
	if ( !bg )
	{
		pids[pi] = pid;
		pi++;
	}
}

/*
 * Name: PrintCommand
 *
 * Description: Prints a Command structure as returned by parse on stdout.
 *
 */
void
PrintCommand (int n, Command *cmd)
{
  printf("Parse returned %d:\n", n);
  printf("   stdin : %s\n", cmd->rstdin  ? cmd->rstdin  : "<none>" );
  printf("   stdout: %s\n", cmd->rstdout ? cmd->rstdout : "<none>" );
  printf("   bg    : %s\n", cmd->bakground ? "yes" : "no");
  PrintPgm(cmd->pgm);
}

/*
 * Name: PrintPgm
 *
 * Description: Prints a list of Pgm:s
 *
 */
void
PrintPgm (Pgm *p)
{
  if (p == NULL) {
    return;
  }
  else {
    char **pl = p->pgmlist;

    /* The list is in reversed order so print
     * it reversed to get right
     */
    PrintPgm(p->next);
    printf("    [");
    while (*pl) {
      printf("%s ", *pl++);
    }
    printf("]\n");
  }
}

/*
 * Name: stripwhite
 *
 * Description: Strip whitespace from the start and end of STRING.
 */
void
stripwhite (char *string)
{
  register int i = 0;

  while (isspace( string[i] )) {
    i++;
  }
  
  if (i) {
    strcpy (string, string + i);
  }

  i = strlen( string ) - 1;
  while (i> 0 && isspace (string[i])) {
    i--;
  }

  string [++i] = '\0';
}
