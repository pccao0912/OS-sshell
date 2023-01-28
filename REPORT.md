# ECS150-Proj1-sshell
sshell project

## Introduction

## Parse
The first part we did is the Parsing secion. We write that function for case of
none piping parsing. The thing we did in the Parse() function is use strtok() to
divide those words for the case there's only one command. So by using strtok(),
the advantage of this method is it can eliminate any useless whitespace, and
only pass several word(command and arguments) into a struct that have a string
array to store. Also, in the struct, there are several flags for error checking.

## Execution without piping
After getting the parsed string array, next thing we did is to extract the
command we want, and pass it to executing along with the rest arguments in the
same struct.

## Execution with piping
A different parse method is needed for the piping since we introduce a new
character “|”. we use a similar method to the one in the parsing function by
using strtok(). count records pipes number and used to create pipes. Since all commands in pipes are executed cocurrently. We use for loop to execute each commands. We again parse each command separately by space and store them into an executable format(Ended with NULL).
After that fork to create child and parent processes. In the child process, we basically build a bridge between each command by connecting the stdin and stdout to each file descriptor's corresponding port and also closing the unused port. After doing that we simply let the child execute the command. In the parent process, we close the previously used descriptor's port and store the WEXITSTATUS(status) in a list. In the case which is supposed to be the last command to be executed. we use waitpid to wait for the child to be done. Then print the complete code with a list of statuses we just stored.


## Redirection
We write two functions for redirection. First we check if there's redirection
symbol before the execution and setting a flag if there is one. The reason for
that is we want to make sure we could identify there's redirection need to
handle before executing. The advantage of that is we could clearly know where's
the error(if any) in the testing process by checking the flag. And if the flag
indicates there's redirection, our program would goes into another function
Redirection() to do the execution, which is setting the output from terminal
STDOUT to input filename.

## Extra feature
### Appending
This is a extra feature Appending, which is simply add some features based on
the redirection. In the redirection, we use open() function to open the file,
with O_TRUNC mode. So we add this feature by changing the mode of opening the
file from O_TRUNC mode to O_APPEND.

### Background

## Summary
The hard part is to combine the redirection with the piping, which requires a
brand new parse since the function we write before that unfortunately didn't
have that function so we come up with a new one, which increase the redundancy.
For the error management part, we fix and add those function accoring to only
the instruction, so these part might not cover all corner cases even though we
created some and tested. We spent a lot of time on doing the background work,
because the part of combining background work with the piping is quite
complicated. We finally achive the version of one background process if piping.
