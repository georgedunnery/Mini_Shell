// George Dunnery
// CS 5007 - Assignment 3 - Mini Shell

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>

// Use char array[BUFFER_SIZE + 1] to get 80 characters plus null terminator
#define BUFFER_SIZE 80
#define COMMANDS 40

// ============================================================================
// FUNCTION DECLARATIONS:
int match(char** user_cmd);
void change_directory(char** user_cmd);
void current_working_directory();
void help_msg();
void linux_command(char** user_cmd, int arguments);
void nullify_cmd(char** user_cmd, int size);
int count_args(char** user_cmd, int max);
void guessing_game(int games);
int number_of_rounds();

// ============================================================================
// CORE FUNCTIONS AND UTILITIES:

// Static array keeping track of the built-in commands
static char* built_in_cmd[] = {"cd","help","exit","gg"};

// Create a signal handler
//   - int sig: The number identifying the signal to handle
void sigint_handler(int sig) {
    // Signal 2 is the command Ctrl+C
    if (sig == 2) {
        printf("\nmini-shell terminated\n");
    }
    else {
        printf("Signal Caught: %d\n", sig);
    }
    exit(0);
}

// Function to dump the rest of the input
// Useful for when the user enters too many commands
// Also stabilizes guessing game for bad input
void clear_input() {
    freopen(NULL,"r",stdin);
}

// Receive user input from the terminal
//   - char* buffer: String of max input length to store the user's input
void receive_commands(char* buffer) {
    // Provide the prompt prefix on the shell
    printf("mini-shell> ");
    // The fgets function will also force the shell to wait for user input
    fgets(buffer, BUFFER_SIZE, stdin);
    // 80+ characters would cause another while loop iteration with the rest
    // The next line clears the remainder, effectively truncating input
    // The return is ignored since we just want to clear stdin
}

// Parse input received from the user
//   - char* buffer: String already containing the user's input
//   - char* user_cmd: Empty array of strings to store parsed user commands
void parse(char* buffer, char** user_cmd) {
    // Clean the user command array of garbage data
    nullify_cmd(user_cmd, COMMANDS);
    // Read the input using the strtok() function
    char* token;
    token = strtok(buffer, " \t\n");
    // Split the input into an array based on " ", "\n", "\t"
    int line = 0;
    while (token != NULL) {
        user_cmd[line] = token + '\0';
        line++;
        token = strtok(NULL, " \t\n");
    }
}

// Check if a given command matches built-in functions
//   - char** user_cmd: Array of strings, first string denotes a function
// Return: Integer, to select a case in execute() switch statement
int match(char** user_cmd) {
    // Initialize variables to control while loop and switch statement
    int index = 0;
    int option = -1;
    // Compare all known commands to this one
    while (built_in_cmd[index] != NULL) {
        // If the command is known, set option accordingly for the switch
        if (strcmp(built_in_cmd[index], user_cmd[0]) == 0) {
            option = index;
            break;
        }
        index++;
    }
    return option;
}

// Execute a command
//   - char* user_cmd: Array of strings, the user commands
//   - int option: The value assigned by the match() function to control switch
void execute(char** user_cmd, int option) {
    // Known commands will run based on the corresponding case
    // An unknown command (option initialized as = -1) will be handled
    // by the default case and sent to the Linux shell
    switch(option) {
        case 0:
            change_directory(user_cmd);
            printf("Navigated To: ");
            current_working_directory();
            break;
        case 1:
            help_msg();
            break;
        case 2:
            exit(1);
            break;
        case 3:
            guessing_game(number_of_rounds());
            break;
        default:
            linux_command(user_cmd, count_args(user_cmd, COMMANDS));
    }
}

// Send unknown commands to the linux shell
// Report to the user when they fail
//   - char** user_cmd: Array of strings, function followed by arguments
//   - int arguments: Number of arguments (including function name)
void linux_command(char** user_cmd, int arguments) {
    // List of arguments to execvp() must end with a NULL pointer
    user_cmd[arguments] = NULL;
    // Create a child process to execute the user commands
    pid_t child;
    child = fork();
    int success;
    // Child process runs inside the if statement
    if (child == 0) {
        success = execvp(user_cmd[0], user_cmd);
        // If the operation failed, report it to the user
        if (success < 0) {
            printf("Command not found, did you mean something else?\n");
            // Exit the child process to avoid orphaning
            exit(1);
        }
        // Exit normally when the operation was successful and is complete
        exit(0);
    }
    // Parent process waits for the child to end in the else statement
    // This way, child processes always run in the foreground until complete
    else {
        wait(&child);
    }
}

// Function to count the number of valid arguments in the list of user commands
//   - char** user_cmd: Array of strings containing the user commands
//   - int max: The maximum possible number of commands (e.g. size of array)
int count_args(char** user_cmd, int max) {
    int count = 0;
    while (count < max) {
        if (strcmp("\0", user_cmd[count]) == 0) {
            break;
        }
        count++;
    }
    return count;
}

// Function to clear an array of strings (intended for list of user commands)
// Sets all strings to "\0" to prevent viewing of garbage data
//   - char** user_cmd: Array of strings
//   - int size: Size of the array of strings
void nullify_cmd(char** user_cmd, int size) {
    int i;
    for(i = 0; i < size; i++) {
        user_cmd[i] = "\0";
    }
}

// ============================================================================
// PIPE SUPPORT:

// Function to count the number of pipes in the user input
// Use this function after parsing the input
//   - char** user_cmd: Array of strings containing the parsed user input
//   - int size: The maximum number of commands the array possibly holds
int count_pipes(char** user_cmd, int size) {
    int num_pipes = 0;
    int i;
    for (i = 0; i < size; i++) {
        if (strcmp("|", user_cmd[i]) == 0) {
            num_pipes++;
        }
    }
    return num_pipes;
}

// Function to extract the commands on either side of the pipe
// This function should only be called after existence of pipe verified
//   - char** user_cmd: Array of strings containing parsed user input
//   - char** base: Array of strings to store first command to pipe
//   - char** next: Array of strings to store second command to pipe
// Example of use: 
//   user_cmd = "ls" "-l" "|" "wc"
//   base     = "ls" "-l"
//   next     = "wc"
// Dev Note: To scale this, can recursively call and have next take the rest
void convert_to_piped(char** user_cmd, char** base, char** next) {
    // Clear the arrays of any potential garbage data
    nullify_cmd(base, COMMANDS);
    nullify_cmd(next, COMMANDS);
    // Two for loops populate base & next. Same variable i to track position
    int i;
    for (i = 0; i < COMMANDS - 1; i++) {
        // Pipe encountered: increment i to skip it, populate next
        if (strcmp("|", user_cmd[i]) == 0) {
            base[i] = NULL;
            i++;
            break;
        }
        base[i] = user_cmd[i];
    }
    // Variable j will keep track of next's index
    // We still use i since it already knows where we are in the user_cmd
    int j = 0;
    for (i; i < COMMANDS - 1; i++) {
        // Stop if we hit another pipe or the next string is null
        if ((strcmp("|", user_cmd[i]) == 0) || 
            (strcmp("\0", user_cmd[i]) == 0)) {
            next[j] = NULL;
            break;
        }
        next[j] = user_cmd[i];
        j++;
    }
}

// Function to execute two commands connected by a single pipe
//   - char** base: Array of strings containing first command (before pipe)
//   - char** next: Array of strings containing second command (after pipe)
// Use convert_to_piped(...) function to format these arrays of strings
void piped_linux_cmds(char** base, char** next) {
    // Create an integer array (of file descriptors) to use as the pipe
    int file_descriptor[2];
    pipe(file_descriptor);

    // 1. Child Process: Executes the base command and writes output to pipe
    pid_t base_child = fork();
    if (base_child == 0) {
        // Close the reading end of the pipe, it's not needed in this child
        close(file_descriptor[0]);
        // Duplicate writing end as standard output
        dup2(file_descriptor[1], STDOUT_FILENO);
        // Close the old writing end of the pipe
        close(file_descriptor[1]);
        // Execute base command, which will be written to standard out
        // Child process ends here, either by execvp or exit(1)
        if (execvp(base[0], base) < 0) {
            printf("Execution Failure: Base Command\n");
            exit(1);
        }
    }

    // 2. Child Process: Executes the next command using base command's output
    pid_t next_child = fork();
    if (next_child == 0) {
        // Close the writing end of the pipe, it's not needed in this child
        close(file_descriptor[1]);
        // Duplicate the reading end as standard input
        dup2(file_descriptor[0], STDIN_FILENO);
        // Close the old reading end of the pipe
        close(file_descriptor[0]);
        // Execute the next command. Output displayed on shell as expected.
        // Child process ends here, either by execvp or exit(1)
        if (execvp(next[0], next) < 0) {
            printf("Execution Failure: Next Command\n");
            exit(1);
        }
    }

    // 3. Parent Process: Wait for the children to execute piped commands
    else {
        // Close both ends of the pipe, the parent does not need them
        close(file_descriptor[0]);
        close(file_descriptor[1]);
        // Wait for both the children to finish their processes
        wait(NULL);
        wait(NULL);
    }
    // PIPE COMPLETE: Both commands executed and results printed on shell.
}

// ============================================================================
// BUILT-IN COMMANDS:

// "cd": Change Directory
//   - char** user_cmd: Array of strings of the user's commands
void change_directory(char** user_cmd) {
    // Initialize a variable to track operation success or failure
    int success;
    if (user_cmd[1] == NULL) {
        success = chdir(getenv("HOME"));
    }
    else {
        success = chdir(user_cmd[1]);
    }
    // Report failure to the user
    // Otherwise, the shell will show the change in directories
    if (success == -1) {
        printf("Unable to change directories.\n");
    }
}

// "help": Print Documentation for Built-in Commands
void help_msg() {
    printf("==============================================================\n");
    printf("                           HELP                               \n");
    printf("==============================================================\n");
    printf("\n>>> PREFACE:\n");
    printf("  The dash characters '-' are used for neatness in this manual.");
    printf("\n  You should omit them when typing actual commands!\n");
    printf("\n>>> BUILT-IN FUNCTIONS:\n");
    printf("- cd [directory]: Navigate to a new directory.\n");
    printf("- help: Print a brief help manual with available commands.\n");
    printf("- exit: Exit the current shell process.\n");
    printf("- gg: Play the guessing game. User will be prompted to select\n");
    printf("      a number of rounds to play.\n");
    printf("\n>>> NOTES:\n");
    printf("- User Input Buffer: Maximum of %d characters is accepted.\n",
                                                                  BUFFER_SIZE);
    printf("                     Additional characters are truncated!\n");
    printf("- Linux Functions: Unknown commands are passed to the Linux\n");
    printf("                   terminal to be carried out.\n");
    printf("- Terminate Shell: Ctrl+C.\n");
    printf("- Piped Commands: The mini-shell currently only supports up\n");
    printf("                  to a single pipe. All input including and\n");
    printf("                  after the second pipe is truncated!\n");
    printf("\n");
    printf("==============================================================\n");
}

// Display Current Working Directory
// Can be called by passing "pwd" to linux terminal as well
// Automatically called after changing directories for convenience
void current_working_directory() {
    char this_directory[250];
    printf("%s\n", getcwd(this_directory, 250));
}

// Helper function to choose a number of rounds to play in the guessing game
// Returns an integer, the number of rounds selected
int number_of_rounds() {
    // Default number of rounds, used when bad input is given
    int rounds = 0;
    printf("\n      GUESSING GAME SETUP     \n");
    printf("==============================\n");
    printf(" Enter Number of Rounds: ");
    scanf("%d", &rounds);
    printf(" You chose %d rounds!\n", rounds);
    // Clear the input in case the user provided bad input
    clear_input();
    return rounds;
}


// Built-in Command Choice: Guessing Game
// A simple guessing game where the player guesses a number between 1 and 25
//   - int games: The number of games you want to play
void guessing_game(int games) {
    printf("==============================\n");
    printf("         GUESSING GAME        \n");
    printf("           Rounds: %d         \n", games);
    printf("==============================\n");
    if (games < 1) {
        return;
    }
    // Create array to track user scores and random number seed
    int scores[games];
    srand(time(NULL));
    int i;
    for (i = 0; i < games; i++) {
        // Message to mark the beginning of each round
        printf("\n    BEGIN ROUND %d of %d !    \n", i + 1, games);
        printf("==============================\n");
        printf(" Pick a number between 1 - 25 \n");
        printf("==============================\n");
        // Generate the secret number & create variable to track user's attemps
        int secretNumber = rand() % 25 + 1;
        int attempts = 0;
        // Run a while loop until the user guesses correctly
        while(1) {
            // Default guess, used when the user provides bad input
            int guess = -1;
            attempts++;
            printf(" Make a guess: ");
            scanf("%d", &guess);
            clear_input();
            // Check if the guess was correct & give a hint if it wasn't
            if (guess > secretNumber) {
                printf(" Too high!\n");
            }
            else if (guess < secretNumber) {
                printf(" Too low!\n");
            }
            else if (guess == secretNumber) {
                printf(" You got it in %d attempts!\n", attempts);
                break;
            }
        }
        // Record the score for the round that just ended
        scores[i] = attempts;
    }
    // Display the user's scores for all the rounds before exiting
    printf("\n          SCOREBOARD          \n");
    printf("==============================\n");
    for (i = 0; i < games; i++) {
        printf(" Round %d: %d attempts!\n", i + 1, scores[i]);
    }
}

// ============================================================================
// MAIN: MINI SHELL

// Hosts and coordinates the mini shell
// Runs continuously until exit is called on every open shell, or Ctrl+C
int main() {
    
    // ==== SETUP ===================================================
    // Install the signal handler
    signal(SIGINT, sigint_handler);
    // Initialize a string to store the user input (plus 1 for ending '\0')
    char buffer[BUFFER_SIZE + 1];
    // Initialize an array of strings to hold the parsed commands
    char* user_cmd[COMMANDS];
    // Initialize an integer to keep track of the number of pipes
    int pipes;
    // Initialize two more arrays of strings to store piped commands
    // Base holds the first command; Next holds the second command
    char* base[COMMANDS];
    char* next[COMMANDS];

    // Announce the starting directory and mention help command
    printf("Starting Shell In: ");
    current_working_directory();
    printf("Type 'help' for a brief user manual.\n");

    // ==== SHELL ===================================================
    // Run the shell in an infinite loop. Terminate with Ctrl+C or exit.
    while(1) {
        // Reset the number of pipes to zero every iteration
        pipes = 0;
        // Wait for user input, then store it in string 'buffer'
        receive_commands(buffer);
        // Allow the user to mindlessly press enter without a seg fault
        // Condition: If there is no input, don't try to do anything
        if (strcmp("\n", buffer) != 0) {
            // Split the buffer by spaces to get the individual commands
            parse(buffer, user_cmd);
            // Check if the user inut contains any pipes
            pipes = count_pipes(user_cmd, COMMANDS);

            // No pipes are present in the input
            if (pipes == 0) {
                // Match to known commands using static array of strings
                // Execute commands after matching determines an option value
                execute(user_cmd, match(user_cmd));
            }

            // Pipe is detected: mini shell currently supports max 1 pipe!
            else {
                // Split user input around pipe into two arrays of strings
                // Everything including and after the second pipe truncated
                convert_to_piped(user_cmd, base, next);
                // Execute the piped commands (only one pipe supported)
                piped_linux_cmds(base, next);
            }
        }
    }
    return 0;
}
