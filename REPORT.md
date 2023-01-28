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

## Extra feature
### Appending

### Background

## Error Management
