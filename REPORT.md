# ECS150-Proj1-sshell
sshell project

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
This is a extra feature Appending, which is simply add some features based on the redirection. In the redirection, we use open() function to open the file, with O_TRUNC mode. So we add this feature by changing the mode of opening the file from O_TRUNC mode to O_APPEND.

### Background

