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
int exec_rec(Pgm*, int, int, int);

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
	int fdin, fdout;

	fdin = cmd->rstdin == NULL ? STDIN_FILENO : open(cmd->rstdin, O_RDONLY);
	fdout = cmd->rstdout == NULL ? STDOUT_FILENO : open(cmd->rstdout, O_WRONLY|O_CREAT);

	int size = exec_rec(cmd->pgm, fdin, fdout, 0);
	for( size; size>-2; size-- )
	{
		wait(NULL);
	}
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
exec_rec(Pgm* pgm, int fdin, int fdout, int size)
{
	const char* bin = pgm->pgmlist[0];
	const char* arg = pgm->pgmlist[1];
	
	int fd[2];
	pipe(fd);
		
	if( pgm->next != NULL )
		exec_rec(pgm->next, fdin, fd[1], size);
		close(fd[1]);
	
	if( fork() == 0 )
	{	
		dup2(fd[0], STDIN_FILENO);
		dup2(fdout, STDOUT_FILENO);
		close(fd[0]);
		execlp(bin, bin, arg, (char*)NULL);
	}

	return size++;
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
