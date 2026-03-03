Simple Shell Interpreter (C in VisualStudioCode)
Implemented a basic Unix shell interpreter that supports command parsing, execution, environment variable management, as well as input/output redirection and piping functionalities.

Command Parsing:
Functionality: Parses the user-entered command line text, including commands and their parameters, while handling special characters and quotes.
Implementation Method: Utilizes string manipulation functions to split user input, identifying commands and parameters. Implements a state machine to handle quotes and escape characters, ensuring complex command lines are parsed accurately.
Command Execution:
Functionality: Executes parsed commands, including both built-in and system commands.
Implementation Method: For built-in commands, directly executes corresponding functions within the Shell program. For system commands, uses fork() to create a child process, employs exec() family functions to replace the current process image in the child, and the parent process waits for completion with wait().
Environment Management:
Functionality: Allows users to set and query environment variables.
Implementation Method: Employs a linked list to store key-value pairs of environment variables. Provides setenv and getenv commands to enable users to add, modify, or retrieve environment variables at runtime.
Input/Output Redirection:
Functionality: Supports redirecting command standard input and output to files.
Implementation Method: Parses command line for redirection symbols (e.g., > and <). Before executing commands, uses freopen() to reopen standard input or output file descriptors, redirecting data streams to specified files.
Pipe Support:
Functionality: Allows users to connect multiple commands via pipes (| symbol), where the output of one command serves as the input to another.
Implementation Method: For each pair of commands connected by a pipe, uses pipe() to create a pipe, fork() to spawn child processes, and dup2() in the child processes to redirect standard input or output to the pipe's read or write end. The main process controls the execution order and resource release of all child processes.