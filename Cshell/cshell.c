

#include <stdio.h>
#include "command.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <fcntl.h>
#include <err.h>

static int shell_aux(struct structure *t, int p_in, int p_out);

int shell(struct structure *t) {
	
	/*check to see if tree == NULL */
	if(t == NULL){
		return 0;
	}else{
		return shell_aux(t, STDIN_FILENO, STDOUT_FILENO);
	}
	return 0;
}

static int shell_aux(struct structure *t, int p_in, int p_out) {
	pid_t child;
	int status, pipe_succ, dup_res, execute_node, pipe_fd[2];
	/*CASE: conjunction is NONE*/
	if(t->conjunction == NONE){
		
		/*direct processing commands: exit and cd*/
		
		/*check for exit command*/
		if(strcmp(t->argv[0], "exit") == 0){
			exit(0);
			/*check for cd command*/
		}else if(strcmp(t->argv[0], "cd") == 0){
			
			if(t->argv[1] == NULL){/*check if there is a location following cd*/
				chdir(getenv("HOME"));
				perror(t->argv[1]);
			}else{
				chdir(t->argv[1]);
				perror(t->argv[1]);
			}
			return 0;
			
		}else{
			child = fork();
			/*check if fork worked*/
			if(child < 0){
				err(EX_OSERR, "Fork Error");
			}else{
				/*------child code-------*/
				if(child != 0){ /*wait for child*/
					wait(&status);
					return status;
				}
				/* If t->input is present (value other than NULL),
				 you will  use the file descriptor associated with opening the file t->input.*/
				if(child == 0){
					if(t->input != NULL){ /*check for input redirection*/
						
						p_in = open(t->input, O_RDONLY);
						dup_res = dup2(p_in, STDIN_FILENO);
						
						if(p_in < 0){
							perror("Error: Input File can't be opened\n");
						}
						if(dup_res < 0){
							perror("Error: dup2 function failed\n");
						}
						close(p_in);
					}
					
					if(t->output != NULL){ /*check for output redirection*/
						p_out = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
						dup_res = dup2(p_out, STDIN_FILENO);
						
						if(p_out < 0){
							perror("Error: output File can't be opened\n");
						}
						
						if(dup_res < 0){
							perror("Error: dup2 function failed\n");
						}
						close(p_out);
					}
					execvp(t -> argv[0], t -> argv);
					/*printf("Failed to execute %s\n", t -> argv[0]);*/
					exit(1);
				}/*finish child process*/
			}
		}
	} /*end of NONE NODE if statement*/
	
	/*CASE: conjunction is AND*/
	else if(t->conjunction == AND){
		/* STEP A: Process the left subtree (t->left) using the parent input/output
		 file descriptors (p_in, p_out) that the node received.*/
	
		
		/* If the left subtree execution is successful, the right subtree
		 (t->right) will be processed using the parent's input/output
		 file descriptors (p_in, p_out).  In this case the
		 value returned by the AND node processing will be  */
		execute_node = execute_aux(t->left, p_in, p_out);
		if(execute_node == 0){
			execute_node = execute_aux(t->right, p_in, p_out);
			
		}
		
	} /*end of AND NODE if statement*/
	
	/*CASE: conjunction is PIPE*/
	else if(t->conjunction == PIPE){
		/*STEP ONE, Check for ambiguous cases*/
		if(t->right->input != NULL) {
			printf("Ambiguous output redirect.\n");
		}else{
			if(t->left->output != NULL){
				printf("Ambiguous output redirect.\n");
			}else{
				if(t->input) {
					p_in = open(t->input, O_RDONLY);
					if(p_in < 0){
						perror("PIPE Error: Input File can't be opened\n");
					}
				}
				
				if(t->output) {
					p_out = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
					if(p_out < 0){
						perror("PIPE Error: Output File can't be opened\n");
					}
				}
				
				/*STEP TWO, Create PIPE*/
				pipe_succ = pipe(pipe_fd);
				if(pipe_succ < 0){
					perror("ERROR: Pipe did not work");
				}
				
				child = fork();
				if(child < 0){
					perror("ERROR: Fork did not work");
				}
				
				
				if(child == 0){
					close(pipe_fd[0]);
					dup_res = dup2(pipe_fd[1], STDOUT_FILENO);
					if(dup_res < 0){
						perror("ERROR dup2 failed.\n");
					}
					execute_aux(t->left, p_in, pipe_fd[1]);
					close(pipe_fd[1]);
					exit(0);
					
				}
				
				if(child != 0){
					close(pipe_fd[1]);
					if (dup2(pipe_fd[0], STDIN_FILENO) < 0) {
						perror("dup2 error");
					}
					execute_aux(t->right, pipe_fd[0], p_out);
					close(pipe_fd[0]);
					wait(NULL);
				}
			}
	
		}
	}/*end of PIPE NODE if statement*/
	
	/*CASE: conjunction is SUBSHELL*/
	else if(t->conjunction == SUBSHELL){
		
		if(t->input){
			p_in = open(t->input, O_RDONLY);
			if(p_in < 0){
				perror("SUBSHELL Error: Input File can't be opened\n");
			}
		}
		
		if(t->output) {
			p_in = open(t->output, O_WRONLY | O_CREAT | O_TRUNC, 0664);
			if(p_in < 0){
				perror("SUBSHELL Error: Output File can't be opened\n");
			}
		}
		
		child = fork();
		if(child < 0){
			perror("Fork did not work");
		}else{
			if(child == 0){
				execute_aux(t->left, p_in, p_out);
				exit(0);
			}
			
			if(child != 0){
				wait(NULL);
			}
		}
	}/*end of SUBSHELL NODE if statement*/
	return 0;
}

