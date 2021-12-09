#include <stdio.h>

#define BAD_COMMAND_ARGS "Usage: unjumble [-alpha|-len|-longest] \
[-include letter] letters [dictionary]\n"
#define BAD_COMMAND_ARGS_ERRC 1

#define BAD_DICT_FILENAME "unjumble: file \"\x25s\" can not be opened\n"
#define BAD_DICT_FILENAME_ERRC 2

#define BAD_LEN_OF_LETTERS "unjumble: must supply at least three letters\n"
#define BAD_LEN_OF_LETTERS_ERRC 3

#define BAD_CHAR_IN_LETTERS "unjumble: can only unjumble \
alphabetic characters\n"
#define BAD_CHAR_IN_LETTERS_ERRC 4

#define NO_MATCHES_ERRC 10

#define NULL_POINTER_ERRM "Call to memory allocation function returned null \
pointer. Failed to allocate memory!\n"

#define MAX_CMD_ARGS 6
#define MIN_CMD_ARGS 2

#define MIN_WORD_LEN 3
#define MAX_WORD_LEN 50

#define NO_SORT 0
#define NO_CHAR '\0'

#define ALPHA_SORT 11
#define LEN_SORT 12
#define LONGEST_SORT 13

#define FLAG_ID '-'

#define DEFAULT_DICT "/usr/share/dict/words"

// strcmp() takes pointers to strings for comparison, not just strings.
const char *alphaArg = "-alpha";

// Hence, these four consts need to be defined like so, instead of
const char *lenArg = "-len";

// just #define'ing them like the other constants above.
const char *longestArg = "-longest";

// Also, apparently they have to have a comment each, despite being related.
const char *includeArg = "-include";

// struct that stores the parsed command args
struct UserSuppliedInformation {
    int sortOrder;
    char mustInclude;
    char *dictAddr;
    char *letters;
    int errorCode;
};

/*
 * Takes the command-line args, initialises all the necessary stuff, 
 * then parses the command args into said stuff.
 *
 * int argc: the number of command-line arguments provided by the user
 * char** argc: array of strings of arguments provided by the user
 *
 * ret <struct>: all the information the program needs to run.
 */
struct UserSuppliedInformation create_user_info_storage(
        int argc, char **argv);

/*
 * Takes the command-line args and parses them for relevant information, 
 * including flags (i.e. -alpha), values (i.e. 'abc') and errors.
 * 
 * The if/elseif/else structure of this function takes 68 lines, plus another
 * ~40 for error checking and comments. Further segmentation of this function
 * (i.e. by moving arg parsing or error checking to a new function) would
 * significantly harm readability and add to the complexity of the code for no
 * real benefit. Keeping the command-arg-parsing all in one function has a
 * higher benefit to readability than keeping to the 50-line function maximum.
 * 
 * int* sortOrder: pointer to sortOrder, so we can set it if needed
 * char* mustInclude: pointer to mustInclude, so we can set it if needed
 * char** letters: pointer to letters... you get the point
 * char** dictAddr: pointer to dictAddr
 * int argc: number of command-line arguments the user provided
 * char** argv: array of strings of command-line arguments the user provided
 *
 * ret <int>: any relevant error codes encountered in parsing the command args
 */
void parse_cmd_args(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv);

void letters_in_pos_two(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv);
 	
void letters_in_pos_three(struct UserSuppliedInformation *initInfoSpace,
       int argc, char **argv);

void letters_in_pos_four(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv);

/*
 * Opens the dictionary file to be read, with provided or default file address
 * char* dictAddr: the file address of the dictionary file.
 * FILE** dictionaryFile: pointer to file pointer, so we can set file pointer
 *
 * ret <int>: any relevant error codes encountered in opening the file
 */
int open_dict_file(char *dictAddr, FILE **dictionaryFile);

/*
 * Reads a dictionary file into an array of strings.
 *
 * FILE *dictionaryFile: the file to be read
 * int *dictionaryLength: the length (in lines) of the dictionary file
 *
 * ret <char**>: array of strings read from the file.
 */
char **read_dict_file(FILE *dictionaryFile, int *dictionaryLength);

/*
 * Checks for a match between the given letters and the word.
 * If the word can be constructed from the letters, returns 1
 *
 * char* letters: the letters that can be used
 * char* dictionaryWord: the word we try to construct
 *
 * ret <int>: 1 if the word can be constructed, otherwise 0
 */
int str_match(char *letters, char *dictionaryWord, char **cmpRemoveLetters);

/*
 * Returns the index of a given item in an array.
 *
 * int check[]: the array to be checked
 * int comp: the comparison item
 * int checkLen: the length of the above array
 *
 * ret <int>: index of the first ocurrence of <comp> in <check>.
 */
int first_index_of(char *check, char comp, int checkLen);

/*
 * Checks if a given char* matches one of the three sorting arguments.
 *
 * char *arg: the argument to check
 *
 * ret <int>: the sort style specified by the argument, or else 0.
 */
int is_sort_arg(char *arg);

/*
 * Checks a given pointer for null and potentially raises errors.
 * void* to void* so that it can be directly used on any alloc-family call.
 *
 * void* newMemAlloc: a pointer to a freshly-allocated block of memory
 *
 * ret <void*>: the same pointer it was given, but guaranteed to not be null.
 */
void *check_null_pointer(void *newMemAlloc);

/*
 * Finds all the words from dictionary that can be made with the given letters
 * and returns the index where they're stored in the dictionary.
 *
 * char** dictionaryWords: the dictionary (an array of strings)
 * int dictionaryLength: the length of the dictionary
 * char* letters: the user-supplied letters to be matched with
 * int* numberOfMatches: a pointer to number of matches, for main to use
 * char includeChar: char to be included in all matches, per the -include flag
 *
 * ret <int*>: an array of ints representing dict indices of all matches
 */
int *find_dictionary_matches(char **dictionaryWords, int dictionaryLength,
        char *letters, int *numberOfMatches, char mustInclude);

/*
 * Constructs a pointer-pointer to only matched words, thus removing the
 * need for storing indices and dictionary words. Retains dictionary order.
 *
 * int* matchedDictionaryIndices: dictionary indices of all valid matches
 * char** dictionaryWords: an array of every word in the dictionary
 * int numberOfMatches: the number of matches
 *
 * ret <char**>: an array of only matched words, in dictionary order.
 */
char **get_matches_as_array(char **dictionaryWords, int *numberOfMatches,
        int dictionaryLength, char *letters, char mustInclude);

/*
 * Prints the matches in the given order. Matches start in dict order.
 *
 * char** matchedWords: an array of strings containing all the matches
 * int numberOfMatches: the number of matches in the above array
 * int sortOrder: the order to sort the matches in
 */
output_matches(char **matchedWords, int numberOfMatches, int sortOrder);

/*
 * Reads the number of lines in a file.
 * FILE *dictionaryFile: the file to be read
 *
 * ret <int>: the number of lines in the file.
 */
int count_lines(FILE *dictionaryFile);

/*
 * A function to 'sort' two strings alphabetically, to be used by qsort.
 * Note that qsort() only sends void pointers, so passed args have to be void
 * despite immediately casting to char.
 *
 * void* str1: a pointer to a string.
 * void* str2: a pointer to a string
 *
 * ret <int>: 1 if str1 > str2, -1 if str1 < str2, and 0 otherwise.
 */
int alpha_sort(const void *str1, const void *str2);

/*
 * A function to 'sort' two strings by length, to be used by qsort.
 * See above function comment with regard to void* type.
 *
 * const void* comp1: a pointer to a string. 
 * const void* comp2: a pointer to a string.
 *
 * ret <int>: 1 if str1 > str2, -1 if str1 < str2, and 0 otherwise.
 */
int len_sort(const void *comp1, const void *comp2);

/*
 * Frees a pointer-pointer with a loop, so it's all guaranteed free'd.
 *
 * char** array: the pointer to an array of pointers
 * int length: the number of pointers in the array
 */
void free_array(char **memBlock, int length);
