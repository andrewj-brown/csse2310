#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "pokemonGameConstants.h"

// returns an int that's a boolean
int in_array(int arr[], int comp, int arr_size);
int check_win(char* game_string, int* pokemon_locations, int num_pokemon);

// returns an int that's an int
int terminal_height(void);
int count_flags(char* game_string);
int parse_index(char* command, int grid_size);
int number_around_cell(int grid_size, int* pokemon_locations, int num_pokemon, int index);

// returns a pointer to an array of ints
int* generate_pokemon(int grid_size, int number_of_pokemon);
int* neighbour_indices(int grid_size, int index);
int* queue_reveal(int grid_size, char* cur_game_string, int* pokemon_locations, int num_pokemon, int index, int* sup_queue);

// returns a pointer to a string
char* read_string(void);
char* init_string(int length);
char* flag_cell(char* game_string, int index);
char* select_cell(int grid_size, char* game_string, int* pokemon_locations, int num_pokemon, int index, int* game_state);

// returns nothing
void display_game(char* game_string, int grid_size, int* game_state);


/*
 * main...
 */
int main(void) {
	int grid_size = 0;
	int num_pokemon = 0;

	srand(time(0));
	
	while (1) {
		// while loops on input ensure that input repeats until it gets a valid input
	
		printf(SIZE_INPUT);
		char* user_input = read_string();

		if (strlen(user_input) <= 3 && strlen(user_input) > 0) {
			grid_size = atoi(user_input);

			if (grid_size == 0) {
				printf(PROBLEM_GRIDSIZE);
				continue;
			}
		} else {
			printf(PROBLEM_GRIDSIZE);
			continue;
		}

		if (grid_size <= 26 && grid_size > 1) {
			break;
		} else {
			printf(PROBLEM_GRIDSIZE);
			continue;
		}

		free(user_input);
	}

	int game_string_len = grid_size * grid_size;

	while (1) {
		printf(NUM_POKEMON_INPUT);
		char* user_input = read_string();

		if (strlen(user_input) <= 4 && strlen(user_input) > 0) {
			num_pokemon = atoi(user_input);

			if (num_pokemon == 0) {
				printf(PROBLEM_NUMPOKE);
				continue;
			}
		} else {
			printf(PROBLEM_NUMPOKE);
			continue;
		}

		if (num_pokemon < game_string_len) {
			break;
		} else {
			printf(PROBLEM_NUMPOKE);
			continue;
		}

		free(user_input);
	}
 	
	char* game_string = init_string(game_string_len);
	memset(game_string, UNEXPOSED, game_string_len);
		
	int* pokemon_locations = generate_pokemon(grid_size, num_pokemon);
	int game_state = 0; // game_state represents three things: | 0: game in progress | -1: game lost | 1: game won |
	int move_count = 5;

	while (game_state == 0) {
		display_game(game_string, grid_size, &game_state);
		
		while (1) {
			printf(COMMAND);
			char* user_input = read_string();
			
			if (user_input[0] == 'f') {
				if (strlen(user_input) > 6 || strlen(user_input) < 5) {
					printf(INVALID);
				} else {	
					int cmd_index = parse_index(user_input, grid_size);
					if (cmd_index >= game_string_len || cmd_index < 0) {
						printf(INVALID);
					} else {
						game_string = flag_cell(game_string, cmd_index);

						free(user_input);
						break;
					}
				}
			} else if (user_input[0] == 's') {
				if (strlen(user_input) > 6 || strlen(user_input) < 5) {
					printf(INVALID);
				} else {
					int cmd_index = parse_index(user_input, grid_size);
					if (cmd_index >= game_string_len || cmd_index < 0) {
						printf(INVALID);
					} else {
						game_string = select_cell(grid_size, game_string, pokemon_locations, num_pokemon, cmd_index, &game_state);
						
						free(user_input);

						if (game_state != -1 && check_win(game_string, pokemon_locations, num_pokemon)) {
							game_state = 1;
						}

						break;
					}
				}
			} else if (user_input[0] == '-') {
				if (user_input[1] == 'q') {
					printf(CHECK_QUIT);
					char* confirm = read_string();
					
					if (confirm[0] == 'y') {
						free(user_input);
						free(confirm);
						free(game_string);
						free(pokemon_locations);
						
						puts(QUIT_TEXT);
						return 0;
					} else {
						free(confirm);
						puts(QUIT_CANCEL);
					}
				} else if (user_input[1] == 'h') {
					printf(HELP_TEXT);
				} else if (user_input[1] == 'r') {
					printf(CHECK_REST);
					char* confirm = read_string();
					
					if (confirm[0] == 'y') {
						memset(game_string, UNEXPOSED, game_string_len);
						pokemon_locations = generate_pokemon(grid_size, num_pokemon);

						free(confirm);
						free(user_input);
						puts(RESTART_TEXT);
						break;
					} else {
						free(confirm);
					}
				} else {
					printf(INVALID);
				}
			} else {
				printf(INVALID);
			}

			free(user_input);
		}
	}
	
	display_game(game_string, grid_size, &game_state);

	free(game_string);
	free(pokemon_locations);
	return 0;
}

/*
 * flags or unflags (as necessary) the cell at index <index>.
 */
char* flag_cell(char* cur_game_string, int index) {
	char* new_game_string = init_string(strlen(cur_game_string));

	new_game_string = strcpy(new_game_string, cur_game_string);

	if (cur_game_string[index] == FLAG) {
		new_game_string[index] = UNEXPOSED;
	} else {
		new_game_string[index] = FLAG;
	}
	
	free(cur_game_string);
	return new_game_string;
}

/*
 * handles selecting the cell at index <index>. Also handles loss scenario.
 *
 * Uses recursion to find nearby "zero" cells.
 */
char* select_cell(int grid_size, char* cur_game_string, int* pokemon_locations, int num_pokemon, int index, int* game_state) {
	char* new_game_string = init_string(strlen(cur_game_string));

	if (cur_game_string[index] == FLAG) {
		free(new_game_string);
		return cur_game_string;
	}

	new_game_string = strcpy(new_game_string, cur_game_string);

	if (in_array(pokemon_locations, index, num_pokemon)) {
		// reveal all pokemon and cell numbers to the user. Ignores flags completely; if the user has placed a flag it will be overwritten by the true "value" of the cell.
				
		for (int i = 0; i < strlen(cur_game_string); i++) {
			if (in_array(pokemon_locations, i, num_pokemon)) {
				new_game_string[i] = POKEMON;	
			} else {
				new_game_string[i] = number_around_cell(grid_size, pokemon_locations, num_pokemon, i) + '0';
			}
		}
		*game_state = -1;
	} else {
		// reveal the current cell, and if it's zero, then "select" all the nearby cells recursively.
		new_game_string[index] = number_around_cell(grid_size, pokemon_locations, num_pokemon, index) + '0';

		if (number_around_cell(grid_size, pokemon_locations, num_pokemon, index) == 0) {
			int* neighbours = neighbour_indices(grid_size, index);
			
			for (int i = 0; i < 8; i++) {
				if (new_game_string[neighbours[i]] == UNEXPOSED) {
					new_game_string = select_cell(grid_size, new_game_string, pokemon_locations, num_pokemon, neighbours[i], game_state);
				}
			}

			free(neighbours);
		}
	}

	free(cur_game_string);
	return new_game_string;
}

/*
 * checks the number of pokemon that are around any given cell.
 */
int number_around_cell(int grid_size, int* pokemon_locations, int num_pokemon, int index) {
	int* neighbours = neighbour_indices(grid_size, index);
	int out = 0;
	
	for (int i = 0; i < 8; i++) {
		if (neighbours[i] != -1) {
			if (in_array(pokemon_locations, neighbours[i], num_pokemon)) {
				out += 1;
			}
		}
	}

	free(neighbours);
	return out;
}

/*
 * creates an array of <int>s representing all the valid "neighbours" that a cell has.
 */
int* neighbour_indices(int grid_size, int index) {
	int* out = (int*)malloc(8*sizeof(int));

	if (out == NULL) {
		fprintf(stderr, "Failure to allocate memory for finding neighbour cells!\n");
		raise(SIGABRT);
	}

	int top = (index < grid_size);
	int left = ((index % grid_size) == 0);
	int right = (((index - grid_size + 1) % grid_size) == 0);
	int bottom = (index > (grid_size * (grid_size - 1)));
	
	if (!top && !left) {
		out[0] = index - grid_size - 1;
	} else {
		out[0] = -1;
	}

	if (!top) {
		out[1] = index - grid_size;
	} else {
		out[1] = -1;
	}

	if (!top && !right) {
		out[2] = index - grid_size + 1;
	} else {
		out[2] = -1;
	}

	if (!right) {
		out[3] = index + 1;
	} else {
		out[3] = -1;
	}

	if (!bottom && !right) {
		out[4] = index + grid_size + 1;
	} else {
		out[4] = -1;
	}

	if (!bottom) {
		out[5] = index + grid_size;
	} else {
		out[5] = -1;
	}

	if (!bottom && !left) {
		out[6] = index + grid_size - 1;
	} else {
		out[6] = -1;
	}

	if (!left) {
		out[7] = index - 1;
	} else {
		out[7] = -1;
	}

	return out;
}

/*
 * turns a command position (a1) into a game string index (0)
 */
int parse_index(char* command, int grid_size) {
	int row = -1;
	int column = -1;
	
	if (command[2] >= 'A' && command[2] <= 'Z') {
		row = command[2] - 'A'; 
	} else {
		row = command[2] - 'a';
	}

	if (command[4] != '\n') {
		char two_digit[] = {command[3], command[4]};
		column = atoi(two_digit) - 1;
	} else {
		column = command[3] - '1';
	}

	if (row != -1 && column != -1) {
		return (row * grid_size) + column;
	} else {
		return -1;
	}
}

/*
 * counts the number of placed flags in the game string
 */
int count_flags(char* game_string) {
	int count = 0;
	for (int i = 0; i < strlen(game_string); i++) {
		if (game_string[i] == FLAG) {
			count += 1;
		}
	}

	return count;
}

/*
 * printf's the game board for the user to look at.
 */
void display_game(char* game_string, int grid_size, int* game_state) {
	int write_addr = 0;
	char* game_board = init_string(19*(grid_size*grid_size) + 18*(grid_size) + 10);

	write_addr += sprintf(game_board+write_addr, "   |");
	for (int i = 0; i < grid_size; i++) {
		if (i < 9) {
			write_addr += sprintf(game_board+write_addr, " %i |", i+1);
		} else {
			write_addr += sprintf(game_board+write_addr, " %i|", i+1);
		}
	}
	write_addr += sprintf(game_board+write_addr, "\n");

	for (int i = 0; i <= grid_size; i++) {
		write_addr += sprintf(game_board+write_addr, "----");
	}
	write_addr += sprintf(game_board+write_addr, "\n");

	for (int i = 0; i < grid_size; i++) {
		write_addr += sprintf(game_board+write_addr, " %c |", i+'A');
	
		for (int j = 0; j < grid_size; j++) {
			char cell_char = game_string[j + (i*grid_size)];

			switch(cell_char) {
				case '0':
					// blank cell
					write_addr += sprintf(game_board + write_addr, "   |");
					break;
				case '1':
					// cyan
					write_addr += sprintf(game_board + write_addr, " \x1B[1;36m%c\x1B[0m |", cell_char);
					break;
				case '2':
					// blue
					write_addr += sprintf(game_board + write_addr, " \x1B[1;34m%c\x1B[0m |", cell_char);
					break;
				case '3':
					// green
					write_addr += sprintf(game_board + write_addr, " \x1B[1;32m%c\x1B[0m |", cell_char);
					break;
				case '4':
					// red
					write_addr += sprintf(game_board + write_addr, " \x1B[1;31m%c\x1B[0m |", cell_char);
					break;
				case '5':
					// yellow
					write_addr += sprintf(game_board + write_addr, " \x1B[1;33m%c\x1B[0m |", cell_char);
					break;
				case '6':
					// magenta
					write_addr += sprintf(game_board + write_addr, " \x1B[1;35m%c\x1B[0m |", cell_char);
					break;
				case '7':
					// white
					write_addr += sprintf(game_board + write_addr, " \x1B[1;37m%c\x1B[0m |", cell_char);
					break;
				case '8':
					// dark grey
					write_addr += sprintf(game_board + write_addr, " \x1B[1;30m%c\x1B[0m |", cell_char);
					break;
				case FLAG:
					// flags have magenta background
					write_addr += sprintf(game_board + write_addr, " \x1B[45m%c\x1B[0m |", cell_char);
					break;
				case POKEMON:
					// pokemon have red background
					write_addr += sprintf(game_board + write_addr, " \x1B[41m%c\x1B[0m |", cell_char);
					break;
				default:
					// anything else (should only ever be '~') has default color
					write_addr += sprintf(game_board+write_addr, " %c |", game_string[j + (i*grid_size)]);
					break;
			}
		}

		write_addr += sprintf(game_board + write_addr, "\n");

		for (int j = 0; j <= grid_size; j++) {
			write_addr += sprintf(game_board + write_addr, "----");
		}
		write_addr += sprintf(game_board+write_addr, "\n");
	}
	puts("");
	printf("%s", game_board);
	free(game_board);

	int numret = terminal_height() - 2*(grid_size+1) - 5;

	if (*game_state < 0) {
		puts(LOSS_TEXT);
	} else if (*game_state > 0) {
		puts(WIN_TEXT);
	} else {
		printf("\nYou have placed %i flags.\n", count_flags(game_string));
	}

	for (int i = 0; i < numret; i++) {
		puts("");
	}
}

/*
 * reads a string input from stdin using getline();
 */
char* read_string(void) {
	size_t size = sizeof(char);
	char* out = (char*)malloc(1 * sizeof(char));

	if (out == NULL) {
		fprintf(stderr, "Failure to allocate memory for reading input string!\n");
		raise(SIGABRT);
	}

	getline(&out, &size, stdin);

	return out;
}

/*
 * checks if <comp> is in <arr>; returns 1 if it is and 0 if it isn't.
 */
int in_array(int arr[], int comp, int arr_size) {
	for (int i = 0; i < arr_size; i++) {
		if (arr[i] == comp) {
			return 1;
		}
	}

	return 0;
}


/*
 * checks if the game has been won; returns 1 if it has and 0 if it hasn't.
 */
int check_win(char* game_string, int* pokemon_locations, int num_pokemon) {
	for (int i = 0; i < strlen(game_string); i++) {
		if (!in_array(pokemon_locations, i, num_pokemon)) {
			if (!isdigit(game_string[i])) {
				return 0;
			}
		}
	}

	return 1;
}

/*
 * generates an array of <int>s representing the indexes in the "game string" where pokemon are hidden.
 */
int* generate_pokemon(int grid_size, int number_of_pokemon) {
	int cell_count = grid_size * grid_size;
	int* pokemon_locations = (int*)malloc(number_of_pokemon*sizeof(int));

	if (pokemon_locations == NULL) {
		fprintf(stderr, "Failure to allocate memory for generating pokemon locations!\n");
		raise(SIGABRT);
	}

	for (int i = 0; i < number_of_pokemon; i++) {
		pokemon_locations[i] = -1;
	}

	for (int i = 0; i < number_of_pokemon; i++) {
		if (i >= cell_count-1) {
			break;
		}
			
		int next_index = rand() / (RAND_MAX / cell_count);

		while (in_array(pokemon_locations, next_index, number_of_pokemon)) {
			next_index = rand() / (RAND_MAX / cell_count);
		}

		pokemon_locations[i] = next_index;
	}

	return pokemon_locations;
}

/*
 * initialises a new string of given length; handles malloc() failures; and sets the string to just be all spaces.
 */
char* init_string(int length) {
	char* out = (char*)malloc((length + 1)* sizeof(char));

	if (out == NULL) {
		fprintf(stderr, "Failure to allocate memory for new string!");
		raise(SIGABRT);
	}

	memset(out, ' ', length);

	return out;
}

/*
 * gets the height of the terminal
 */
int terminal_height(void) {
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);

	return w.ws_row;
}
