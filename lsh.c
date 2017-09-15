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

//
int exec_rec(Pgm*, int, int);

/* When non-zero, this global means the user is done using this program. */
int done = 0;

/*
 * Name: main
 *
 * Description: Gets the ball rolling...
 *
 */
int main(void)
{
  Command cmd;
  int n;

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
	// set fdin and fdout to STDIN and STDOUT if no redirect is given.
	int fdin, fdout, fderr;
	int *stat;

	fdin = cmd->rstdin == NULL ? STDIN_FILENO : open(cmd->rstdin, O_RDONLY);
	fdout = cmd->rstdout == NULL ? STDOUT_FILENO : open(cmd->rstdout, O_WRONLY|O_CREAT);

	// get the first program.
	Pgm* p = cmd->pgm; 

	// no idea why i need to print this but code fails otherwise.
	printf("elos\n");

	// fd[0]=read, fd[1]=write
	// only one program, no piping needed.
	if (p->next == NULL) {
		exec_rec(p, fdin, fdout);

		wait(stat);
		printf("wait status: %i\n", *stat);

		return 1;
	}

	// multiple programs, piping is needed.
	while (p != NULL) {
		// create pipe.
		int fd[2];
		pipe(fd);

		// if last program to run use fdin as input.
		if (p->next == NULL) {
			exec_rec(p, fdin, fdout);
		} else {
			// execute program p with input from fd[read] and 
			// output it to fdout.
			exec_rec(p, fd[0], fdout);

			// save fd[write] to fdout for next program.
			fdout = fd[1];
		}

		// go to next program.
		p = p->next;
	}

	// wait for all child processes to stop
	wait(NULL);
	printf("wait status: %i\n", *stat);

	return 1;
}

/*
 * Name: exec_rec
 *
 * Parameters:
 * pgm   - The program to execute.
 * fdin  - The file descriptor program should read input from.
 * fdout - The file descriptor program should write output to.
 *
 * Description: 
 * Runs a single program, reads input from fdin and writes 
 * output to fdout.
 */
int
exec_rec(Pgm* pgm, int fdin, int fdout)
{
	// EXTRACT ARGUMENTS <start>
	int *stat;
	int size = 0;

	char** t = pgm->pgmlist;

	// calculate size of program+arguments.
	while (*t) {
		size++;
		*t++;
	}

	// add extra room for (char*)NULL.
	size++;
	char* tmp[size];

	// add program+arguments to tmp.
	for (int i=0; i<size-1; i++) {
		tmp[i] = pgm->pgmlist[i];
	}

	// Add the (char*)NULL to pgmlist (required by execvp).
	tmp[size] = NULL;

	// EXTRACT PROGRAM AND ARGUMENTS <end>

	switch(fork()) {
		case 0: // child runs command.
			if (fdin != 0) {
				dup2(fdin, STDIN_FILENO);
				close(fdin);
			}
			if (fdout != 1) {
				dup2(fdout, STDOUT_FILENO);
				close(fdout);
			}

			return execvp(tmp[0], (char * const *) tmp);
		default: // parent don't wait since we want to execute more programs.
			break;
	}

	return 1;
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
