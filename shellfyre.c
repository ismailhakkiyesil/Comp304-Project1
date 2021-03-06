//İsmail Hakkı Yeşil 72293 - Alkan Akısu 71455

#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>
#include <limits.h>

const char *sysname = "shellfyre";

void execute(char *commName, char **args);
void fileSearch(char *searchedWord, bool isRecursive, bool isOpen);
void take(char *dirName);
static void mkdirRecursive(const char *path, mode_t mode);
void rockPaperScissors();
char *whichFighter(char fighterInitial);
void coffeeSelection();
void DadJoke();
void pstraverse(int root, char* flag);

enum return_codes
{
	SUCCESS = 0,
	EXIT = 1,
	UNKNOWN = 2,
};

struct command_t
{
	char *name;
	bool background;
	bool auto_complete;
	int arg_count;
	char **args;
	char *redirects[3];		// in/out redirection
	struct command_t *next; // for piping
};

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command)
{
	int i = 0;
	printf("Command: <%s>\n", command->name);
	printf("\tIs Background: %s\n", command->background ? "yes" : "no");
	printf("\tNeeds Auto-complete: %s\n", command->auto_complete ? "yes" : "no");
	printf("\tRedirects:\n");
	for (i = 0; i < 3; i++)
		printf("\t\t%d: %s\n", i, command->redirects[i] ? command->redirects[i] : "N/A");
	printf("\tArguments (%d):\n", command->arg_count);
	for (i = 0; i < command->arg_count; ++i)
		printf("\t\tArg %d: %s\n", i, command->args[i]);
	if (command->next)
	{
		printf("\tPiped to:\n");
		print_command(command->next);
	}
}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command)
{
	if (command->arg_count)
	{
		for (int i = 0; i < command->arg_count; ++i)
			free(command->args[i]);
		free(command->args);
	}
	for (int i = 0; i < 3; ++i)
		if (command->redirects[i])
			free(command->redirects[i]);
	if (command->next)
	{
		free_command(command->next);
		command->next = NULL;
	}
	free(command->name);
	free(command);
	return 0;
}

/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt()
{
	char cwd[1024], hostname[1024];
	gethostname(hostname, sizeof(hostname));
	getcwd(cwd, sizeof(cwd));
	printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
	return 0;
}

/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command)
{
	const char *splitters = " \t"; // split at whitespace
	int index, len;
	len = strlen(buf);
	while (len > 0 && strchr(splitters, buf[0]) != NULL) // trim left whitespace
	{
		buf++;
		len--;
	}
	while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
		buf[--len] = 0; // trim right whitespace

	if (len > 0 && buf[len - 1] == '?') // auto-complete
		command->auto_complete = true;
	if (len > 0 && buf[len - 1] == '&') // background
		command->background = true;

	char *pch = strtok(buf, splitters);
	command->name = (char *)malloc(strlen(pch) + 1);
	if (pch == NULL)
		command->name[0] = 0;
	else
		strcpy(command->name, pch);

	command->args = (char **)malloc(sizeof(char *));

	int redirect_index;
	int arg_index = 0;
	char temp_buf[1024], *arg;

	while (1)
	{
		// tokenize input on splitters
		pch = strtok(NULL, splitters);
		if (!pch)
			break;
		arg = temp_buf;
		strcpy(arg, pch);
		len = strlen(arg);

		if (len == 0)
			continue;										 // empty arg, go for next
		while (len > 0 && strchr(splitters, arg[0]) != NULL) // trim left whitespace
		{
			arg++;
			len--;
		}
		while (len > 0 && strchr(splitters, arg[len - 1]) != NULL)
			arg[--len] = 0; // trim right whitespace
		if (len == 0)
			continue; // empty arg, go for next

		// piping to another command
		if (strcmp(arg, "|") == 0)
		{
			struct command_t *c = malloc(sizeof(struct command_t));
			int l = strlen(pch);
			pch[l] = splitters[0]; // restore strtok termination
			index = 1;
			while (pch[index] == ' ' || pch[index] == '\t')
				index++; // skip whitespaces

			parse_command(pch + index, c);
			pch[l] = 0; // put back strtok termination
			command->next = c;
			continue;
		}

		// background process
		if (strcmp(arg, "&") == 0)
			continue; // handled before

		// handle input redirection
		redirect_index = -1;
		if (arg[0] == '<')
			redirect_index = 0;
		if (arg[0] == '>')
		{
			if (len > 1 && arg[1] == '>')
			{
				redirect_index = 2;
				arg++;
				len--;
			}
			else
				redirect_index = 1;
		}
		if (redirect_index != -1)
		{
			command->redirects[redirect_index] = malloc(len);
			strcpy(command->redirects[redirect_index], arg + 1);
			continue;
		}

		// normal arguments
		if (len > 2 && ((arg[0] == '"' && arg[len - 1] == '"') || (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
		{
			arg[--len] = 0;
			arg++;
		}
		command->args = (char **)realloc(command->args, sizeof(char *) * (arg_index + 1));
		command->args[arg_index] = (char *)malloc(len + 1);
		strcpy(command->args[arg_index++], arg);
	}
	command->arg_count = arg_index;
	return 0;
}

void prompt_backspace()
{
	putchar(8);	  // go back 1
	putchar(' '); // write empty over
	putchar(8);	  // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command)
{
	int index = 0;
	char c;
	char buf[4096];
	static char oldbuf[4096];

	// tcgetattr gets the parameters of the current terminal
	// STDIN_FILENO will tell tcgetattr that it should write the settings
	// of stdin to oldt
	static struct termios backup_termios, new_termios;
	tcgetattr(STDIN_FILENO, &backup_termios);
	new_termios = backup_termios;
	// ICANON normally takes care that one line at a time will be processed
	// that means it will return if it sees a "\n" or an EOF or an EOL
	new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
	// Those new settings will be set to STDIN
	// TCSANOW tells tcsetattr to change attributes immediately.
	tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

	// FIXME: backspace is applied before printing chars
	show_prompt();
	int multicode_state = 0;
	buf[0] = 0;

	while (1)
	{
		c = getchar();
		// printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

		if (c == 9) // handle tab
		{
			buf[index++] = '?'; // autocomplete
			break;
		}

		if (c == 127) // handle backspace
		{
			if (index > 0)
			{
				prompt_backspace();
				index--;
			}
			continue;
		}
		if (c == 27 && multicode_state == 0) // handle multi-code keys
		{
			multicode_state = 1;
			continue;
		}
		if (c == 91 && multicode_state == 1)
		{
			multicode_state = 2;
			continue;
		}
		if (c == 65 && multicode_state == 2) // up arrow
		{
			int i;
			while (index > 0)
			{
				prompt_backspace();
				index--;
			}
			for (i = 0; oldbuf[i]; ++i)
			{
				putchar(oldbuf[i]);
				buf[i] = oldbuf[i];
			}
			index = i;
			continue;
		}
		else
			multicode_state = 0;

		putchar(c); // echo the character
		buf[index++] = c;
		if (index >= sizeof(buf) - 1)
			break;
		if (c == '\n') // enter key
			break;
		if (c == 4) // Ctrl+D
			return EXIT;
	}
	if (index > 0 && buf[index - 1] == '\n') // trim newline from the end
		index--;
	buf[index++] = 0; // null terminate string

	strcpy(oldbuf, buf);

	parse_command(buf, command);

	// print_command(command); // DEBUG: uncomment for debugging

	// restore the old settings
	tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
	return SUCCESS;
}

int process_command(struct command_t *command);

int main()
{
	while (1)
	{
		struct command_t *command = malloc(sizeof(struct command_t));
		memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

		int code;
		code = prompt(command);
		if (code == EXIT)
			break;

		code = process_command(command);
		if (code == EXIT)
			break;

		free_command(command);
	}

	printf("\n");
	return 0;
}

int process_command(struct command_t *command)
{
	int r;
	if (strcmp(command->name, "") == 0)
		return SUCCESS;

	if (strcmp(command->name, "exit") == 0)
		return EXIT;

	if (strcmp(command->name, "cd") == 0)
	{
		if (command->arg_count > 0)
		{
			r = chdir(command->args[0]);
			if (r == -1)
				printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
			return SUCCESS;
		}
	}

	// TODO: Implement your custom commands here

	if (strcmp(command->name, "filesearch") == 0 || strcmp(command->name, "fs") == 0)
	{
		// printf("filesearch working\n");
		if (command->arg_count == 1)
		{
			fileSearch(command->args[0], false, false);
			return SUCCESS;
		}
		else if (command->arg_count == 2)
		{

			if (strcmp(command->args[1], "-r") == 0)
				fileSearch(command->args[0], true, false);
			else if (strcmp(command->args[1], "-o") == 0)
				fileSearch(command->args[0], false, true);
			else
			{
				printf("Syntax must be like this --> fs filename [-r],[-o]\n");
				return UNKNOWN;
			}

			return SUCCESS;
		}
		else if (command->arg_count == 3)
		{
			fileSearch(command->args[0], true, true);
		}
		else
		{
			printf("Syntax must be like this --> fs filename [-r],[-o]\n");
			return UNKNOWN;
		}
	}

	if (strcmp(command->name, "rockpaperscissors") == 0 || strcmp(command->name, "rps") == 0)
	{
		rockPaperScissors();
		return SUCCESS;
	}

	if (strcmp(command->name, "coffeeselection") == 0 || strcmp(command->name, "coffee") == 0)
	{
		coffeeSelection();
		return SUCCESS;
	}

if (strcmp(command->name, "take")==0 || strcmp(command->name, "t") == 0)
	{
		take(command->args[0]);
		return SUCCESS;
 	}

	if (strcmp(command->name, "joker") == 0 || strcmp(command->name, "j") == 0)
	{

		// */15 * * * *
		DadJoke();
		return SUCCESS;
	}

	if (strcmp(command->name, "pstraverse")==0 || strcmp(command->name, "pst")==0)
	{
		pstraverse(atoi(command->args[0]), command->args[1]);
		return SUCCESS;
 	}

	pid_t pid = fork();

	if (pid == 0) // child
	{
		// increase args size by 2
		command->args = (char **)realloc(
			command->args, sizeof(char *) * (command->arg_count += 2));

		// shift everything forward by 1
		for (int i = command->arg_count - 2; i > 0; --i)
			command->args[i] = command->args[i - 1];

		// set args[0] as a copy of name
		command->args[0] = strdup(command->name);
		// set args[arg_count-1] (last) to NULL
		command->args[command->arg_count - 1] = NULL;

		/// TODO: do your own exec with path resolving using execv()
		execute(command->name, command->args);

		exit(0);
	}
	else
	{
		/// TODO: Wait for child to finish if command is not running in background
		bool isBG = command->background;
		if (!isBG)
			wait(0);
		return SUCCESS;
	}

	printf("-%s: %s: command not found\n", sysname, command->name);
	return UNKNOWN;
}

void execute(char *commName, char **args)
{
	bool isChDir = strcmp("cd", commName) == 0;
	bool isGCC = strcmp("cd", commName) == 0;
	bool isExceptions = isGCC || isChDir;

	if (isExceptions)
	{
		if (isChDir)
			chdir(args[0]);
		else
			execv("/usr/bin/gcc", args);
		return;
	}

	char *PATHenv = getenv("PATH");
	int size = (sizeof(PATHenv) / sizeof(PATHenv[0]));
	size++;

	char *pathArr[size];

	char *p = strtok(PATHenv, ":");

	for (int i = 0; i < size; i++)
	{
		pathArr[i] = p;
		p = strtok(NULL, ":");
	}

	for (int i = 0; i < sizeof(pathArr) / sizeof(pathArr[0]); i++)
	{
		char buffer[50] = "";
		strcat(buffer, pathArr[i]);
		strcat(buffer, "/");
		strcat(buffer, commName);

		if (access(buffer, X_OK) == 0)
			execv(buffer, args);
	}
}

void fileSearch(char *searchedWord, bool isRecursive, bool isOpen)
{
	char fileName[100];
	strcat(fileName, searchedWord);
	strcat(fileName, ".*");

	// -o is work in progress
	if (!isRecursive)
	{
		if (!isOpen)
			execlp("find", "find", ".", "-maxdepth", "1",
				   "-name",
				   fileName, NULL);
		else
			execlp("find", "find", ".", "-maxdepth", "1", "-name", fileName, "-exec", "cat", " '{}' ", "\\;", NULL);
	}
	else
	{
		if (!isOpen)
			execlp("find", "find", ".", "-name", fileName, NULL);
		else
			execlp("find", "find", ".", "-name", fileName, "-exec", "cat", " '{}' ", "\\;", NULL);
	}
}

void rockPaperScissors()
{
	int mode;
	printf("Welcome to Rock Paper Scissors\n");
	printf("Please Select A Mode\n");
	printf("1 For 1 round Game\n");
	printf("2 For Best of 3 Game\n");
	printf("3 For Best of 5 Game\n");
	scanf("%d", &mode);
	int count =
		mode == 1
			? 1
		: mode == 2
			? 3
		: mode == 3
			? 5
			: 0;
	char player;
	char bot;
	int playerScore = 0, botScore = 0;
	int round = 1;
	while (1)
	{
		printf("\nRound %d\n", round++);
		printf("Choose Your Fighter (r for rock, p for paper, s for scissors)\n");
		scanf(" %c", &player); // initall pre-blank
		int random = rand() % 3;
		bot =
			random == 0
				? 'r'
			: random == 1
				? 'p'
				: 's';

		bool playerWin = (player == 'r' && bot == 's') || (player == 'p' && bot == 'r') || (player == 's' && bot == 'p');
		bool botWin = (player == 's' && bot == 'r') || (player == 'r' && bot == 'p') || (player == 'p' && bot == 's');
		bool isTie = player == bot;

		playerScore += playerWin ? 1 : 0;
		botScore += botWin ? 1 : 0;
		if (playerWin)
			printf("You win the round %s beats %s\n", whichFighter(player), whichFighter(bot));
		if (botWin)
			printf("You lose the round %s beaten by %s\n", whichFighter(player), whichFighter(bot));
		if (isTie)
			printf("It is tie %s cannot beat %s\n", whichFighter(player), whichFighter(bot));

		printf("Scores are --> You: %d Computer: %d\n", playerScore, botScore);

		if (playerScore > count / 2 || botScore > count / 2)
			break;
	}
	printf("\nMatch Finished with scores --> You: %d Computer: %d\n", playerScore, botScore);
	if (playerScore > botScore)
		printf("\nCongrats!!\n\n");
	else
		printf("\nBig L for you\n\n");
}

char *whichFighter(char fighterInitial)
{
	if (fighterInitial == 'r')
		return "Rock";
	if (fighterInitial == 'p')
		return "Paper";
	if (fighterInitial == 's')
		return "Scissors";
	return "";
}

void coffeeSelection()
{
	int temperature;
	printf("Welcome to KU Bucks\n");
	printf("Please select your coffee temperature\n");
	printf("1 For Hot\n");
	printf("2 For Iced\n");
	scanf("%d", &temperature);

	int coffee_selection;
	int milk_selection; 

	if(temperature == 1){
	
		printf("Our Hot coffee options are either cappucino or filtered coffee \n");
		printf("<1 for americano, 2 for filtered coffee> \n");
		scanf("%d", &coffee_selection);

		printf("<You saw the milk behind the counter, if you want milk type 1 else type 0>\n");
		scanf("%d", &milk_selection);
		if(milk_selection==1){
			if(coffee_selection ==1){
				printf("Oh, I forgot to mention milk. Here, your Hot americano with milk \n");
			}
			else{
				printf("Oh, I forgot to mention milk. Here, your Hot filtered coffee with milk \n");
			}
		}
		else{
			if(coffee_selection ==1){
				printf("Oh, I forgot to mention milk. Here, your Hot americano \n");
			}
			else{
				printf("Oh, I forgot to mention milk. Here, your Hot filtered coffee \n");
			}
		}
	}
	else {
		printf("Our Iced coffee options are either americano or filtered coffee \n");
		printf("<1 for americano, 2 for filtered coffee> \n");
		scanf("%d", &coffee_selection);

		printf("<You saw the milk behind the counter, if you want milk type 1 else type 0> \n");
		scanf("%d", &milk_selection);
		if(milk_selection==1){
			if(coffee_selection ==1){
				printf("Oh, I forgot to mention milk. Here, your Iced americano with milk \n");
			}
			else{
				printf("Oh, I forgot to mention milk. Here, your Iced filtered coffee with milk \n");
			}
		}
		else{
			if(coffee_selection ==1){
				printf("Oh, I forgot to mention milk. Here, your Iced americano \n");
			}
			else{
				printf("Oh, I forgot to mention milk. Here, your Iced filtered coffee \n");
			}
		}
	}
}
void take(char* dirName){
	mkdirRecursive(dirName, 0777);
	chdir(dirName);
}

static void mkdirRecursive(const char *path, mode_t mode) {
    char opath[PATH_MAX];
    char *p;
    size_t len;

    strncpy(opath, path, sizeof(opath));
    opath[sizeof(opath) - 1] = '\0';
    len = strlen(opath);
    if (len == 0)
        return;
    else if (opath[len - 1] == '/')
        opath[len - 1] = '\0';
    for(p = opath; *p; p++)
        if (*p == '/') {
            *p = '\0';
            if (access(opath, F_OK))
                mkdir(opath, mode);
            *p = '/';
        }
    if (access(opath, F_OK))         /* if path is not terminated with / */
        mkdir(opath, mode);
}

void DadJoke()
{
	// curl - H "Accept: text/plain" https://icanhazdadjoke.com/
	char *URL = "https://icanhazdadjoke.com";

	execlp("curl", "curl", "-H", "\"Accept: text/plain\"", URL, NULL);
}
void pstraverse(int root, char *flag) {
	//printf("Im inside pst\n");
	
    char pid_number[15];
	sprintf(pid_number, "%d", root);

	char root_pid[15] = "pid=";
	char dfs_or_bfs[15] = "flag=";
	strcat(root_pid, pid_number);
	strcat(dfs_or_bfs, flag);

	/*
	printf("%s",root_pid);
	printf("\n");
	printf("%s",dfs_or_bfs);
	*/

	int child;
	if((child = fork()) == 0){
		//printf("im inside child \n");
		char *traverse_args[] = {"/usr/bin/sudo","insmod","pstraverse.ko",root_pid,dfs_or_bfs,0};
    	execv(traverse_args[0], traverse_args);
	}else{
		//printf("im inside parent \n");
		wait(NULL);
		char *remove_args[] = {"/usr/bin/sudo","rmmod","pstraverse.ko",0};
    	execv(remove_args[0], remove_args);
	}
}