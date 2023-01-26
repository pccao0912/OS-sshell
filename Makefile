sshell: sshell.c
	gcc -Wall -Werror -Wextra sshell.c -o sshell

clean:
	rm -f sshell