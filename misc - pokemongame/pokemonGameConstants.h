#define POKEMON '@'
#define FLAG '!'
#define UNEXPOSED '~'

#define COMMAND "\nPlease input a command: "
#define INVALID "Invalid action! Enter '-h' command for help.\n"
#define HELP_TEXT "'-h' - Help.\n's <Uppercase Letter><number>' - Selecting a cell (e.g. 's A1')\n'f <Uppercase Letter><number>' - Placing flag at cell (e.g. 'f A1')\n'-r' - Restart game.\n'-q' - Quit.\n"

#define SIZE_INPUT "Please input the size of the grid (at least 2, and sizes >26 will be treated as 26): "
#define NUM_POKEMON_INPUT "Please input the number of pokemons (at least one cell must be without a pokemon): "

#define CHECK_QUIT "Are you sure you want to quit? (y/n): "
#define CHECK_REST "Are you sure you want to restart? (y/n): "

#define QUIT_TEXT "Catch you on the flip side.\n"
#define QUIT_CANCEL "Let's keep going.\n"
#define RESTART_TEXT "It's rewind time.\n"
#define LOSS_TEXT "\nYou have scared away all the pokemon!\n"
#define WIN_TEXT "\nYou win!\n"

#define PROBLEM_GRIDSIZE "Huh? I need a number between 2 and 26!\n"
#define PROBLEM_NUMPOKE "Huh? I need a proper number!\n"
