#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

// #define, const, function defs, and external libraries
#include "unjumbleDefs.h"

int main(int argc, char **argv)
{
    FILE *dictionaryFile;
    struct UserSuppliedInformation parsedArgs =
            create_user_info_storage(argc, argv);
    int openDictErr = open_dict_file(parsedArgs.dictAddr, &dictionaryFile);
    if (parsedArgs.errorCode == 0) {
        parsedArgs.errorCode = openDictErr;
    }
    
    // return early if any errors are encountered
    if (parsedArgs.errorCode != 0) {
        free(parsedArgs.letters);
        if (openDictErr == 0) {
            fclose(dictionaryFile);
        }
        if (parsedArgs.errorCode == BAD_COMMAND_ARGS_ERRC) {
            fprintf(stderr, BAD_COMMAND_ARGS);
            free(parsedArgs.dictAddr);
            return BAD_COMMAND_ARGS_ERRC;
        } else if (parsedArgs.errorCode == BAD_LEN_OF_LETTERS_ERRC) {
            fprintf(stderr, BAD_LEN_OF_LETTERS);
            free(parsedArgs.dictAddr);
            return BAD_LEN_OF_LETTERS_ERRC;
        } else if (parsedArgs.errorCode == BAD_DICT_FILENAME_ERRC) {
            fprintf(stderr, BAD_DICT_FILENAME, parsedArgs.dictAddr);
            free(parsedArgs.dictAddr);
            return BAD_DICT_FILENAME_ERRC;
        } else if (parsedArgs.errorCode == BAD_CHAR_IN_LETTERS_ERRC) {
            fprintf(stderr, BAD_CHAR_IN_LETTERS);
            free(parsedArgs.dictAddr);
            return BAD_CHAR_IN_LETTERS_ERRC;
        }
    }

    int dictionaryLength, numberOfMatches;
    char **dictionaryWords = read_dict_file(dictionaryFile, &dictionaryLength);
    free(parsedArgs.dictAddr);

    char **matchedWords =
            get_matches_as_array(dictionaryWords, &numberOfMatches, 
            dictionaryLength, parsedArgs.letters, parsedArgs.mustInclude);
    
    free(parsedArgs.letters);
    if (numberOfMatches == 0) {
        free(matchedWords);
        return NO_MATCHES_ERRC;
    }
    output_matches(matchedWords, numberOfMatches, parsedArgs.sortOrder);
    return 0;
}

/*
 * Frees a pointer-pointer by looping over the array and freeing each element
 *
 * char** memBlock: the pointer to an array of pointers
 * int length: the number of pointers in the array
 */
void free_array(char **memBlock, int length) {
    for (int i = 0; i < length; i++) {
        free(memBlock[i]);
    }
    
    free(memBlock);
}

/*
 * Prints the matches in the given order. Dictionary order is the default.
 * char** matchedWords: an array of strings containing all the matches
 * int numberOfMatches: the number of matches in the above array
 * int sortOrder: the order to sort the matches in
 */
void output_matches(char **matchedWords, int numberOfMatches, int sortOrder)
{
    if (sortOrder == NO_SORT) {
        for (int i = 0; i < numberOfMatches; i++) {
            printf("%s\n", matchedWords[i]);
            free(matchedWords[i]);
        }
    } else if (sortOrder == ALPHA_SORT) {
        qsort(matchedWords, (size_t)numberOfMatches, sizeof(char *),
                alpha_sort);
        for (int i = 0; i < numberOfMatches; i++) {
            printf("%s\n", matchedWords[i]);
            free(matchedWords[i]);
        }
    } else if (sortOrder == LEN_SORT) {
        qsort(matchedWords, (size_t)numberOfMatches, sizeof(char *), len_sort);
        for (int i = 0; i < numberOfMatches; i++) {
            printf("%s\n", matchedWords[i]);
            free(matchedWords[i]);
        }
    } else if (sortOrder == LONGEST_SORT) {
        // sort by length, print only if length matches the first (longest)
        qsort(matchedWords, (size_t)numberOfMatches, sizeof(char *), len_sort);
        int longestLen = strlen(matchedWords[0]);

        for (int i = 0; i < numberOfMatches; i++) {
            if (strlen(matchedWords[i]) == longestLen) {
                printf("%s\n", matchedWords[i]);
            }
            free(matchedWords[i]);
        }
    }
    
    free(matchedWords);
    return;
}

/*
 * A function to 'sort' two strings alphabetically, to be used by qsort.
 * NB: qsort only sends const void *, so parameters need to match that type.
 * const void* comp1: a pointer to a string. See above for type info
 * const void* comp2: a pointer to a string. 
 *
 * ret <int>: -1 if str1 sorts > str2, 1 if str1 sorts < str2, and 0 otherwise
 */
int alpha_sort(const void *comp1, const void *comp2)
{
    const char *compVal1 = ((const char **)comp1)[0];
    const char *compVal2 = ((const char **)comp2)[0];

    int ret = strcasecmp(compVal1, compVal2);
    if (ret != 0) {
        return ret;
    } else {
        return strcmp(compVal1, compVal2);
    }
}

/*
 * A function to 'sort' two strings by length, to be used by qsort.
 * NB: qsort only sends const void *, so parameters need to match that type.
 * const void* comp1: a pointer to a string. See above for type info
 * const void* comp2: a pointer to a string. 
 *
 * ret <int>: -1 if str1 sorts > str2, 1 if str2 sorts < str2, and 0 otherwise
 */
int len_sort(const void *comp1, const void *comp2)
{
    const char *compVal1 = ((const char **)comp1)[0];
    const char *compVal2 = ((const char **)comp2)[0];

    if (strlen(compVal1) > strlen(compVal2)) {
        return -1;
    } else if (strlen(compVal1) < strlen(compVal2)) {
        return 1;
    } else {
        // if they're the same length, sort alphabetically
        return alpha_sort(comp1, comp2);
    }
}

/*
 * Constructs a string array of only the matches so we can free other stuff
 * Retains dictionary order in doing so.
 * int* matchedDictionaryIndices: the dictionary index of every valid match
 * char** dictionaryWords: an array of every word in the dictionary
 * int numberOfMatches: the number of matches
 *
 * ret <char**>: an array of only matched words, in dictionary order.
 */
char **get_matches_as_array(char **dictionaryWords, int *numberOfMatches,
        int dictionaryLength, char *letters, char mustInclude)
{
    int *matchedDictionaryIndices = find_dictionary_matches(dictionaryWords,
            dictionaryLength, letters, numberOfMatches, mustInclude);    

    char **wordMatches = 
            (char **)check_null_pointer(
            malloc((numberOfMatches[0] + 1) * sizeof(char *)));
    int wordLen;

    for (int i = 0; i < numberOfMatches[0]; i++) {
        wordLen = strlen(dictionaryWords[matchedDictionaryIndices[i]]);
        wordMatches[i] =
                (char *)check_null_pointer(
                calloc((wordLen + 1), sizeof(char)));
        strcpy(wordMatches[i], dictionaryWords[matchedDictionaryIndices[i]]);
    }
    
    free(matchedDictionaryIndices);
    free_array(dictionaryWords, dictionaryLength);
    return wordMatches;
}

/*
 * Finds all the dictionary words that can be made with the given letters,
 * and returns the index where they're stored in the dictionary.
 * char** dictionaryWords: the dictionary (an array of strings)
 * int dictionaryLength: the length of the dictionary
 * char* letters: the user-supplied letters to be matched with
 * int* numberOfMatches: pointer to the number of matches so we can change it
 * char includeChar: character that must be in a match, per -include flag
 *
 * ret <int*>: array of ints representing the dictionary index of every match
 */
int *find_dictionary_matches(char **dictionaryWords, int dictionaryLength,
        char *letters, int *numberOfMatches, char mustInclude)
{
    int *matchedDictionaryIndices =
            (int *)check_null_pointer(malloc(1 * sizeof(int)));

    int match;
    *numberOfMatches = 0;
    
    char **cmpRemoveLetters = check_null_pointer(malloc(1 * sizeof(char *)));
    cmpRemoveLetters[0] = check_null_pointer(
            malloc((strlen(letters) + 1) * sizeof(char)));
    strcpy(cmpRemoveLetters[0], letters);

    for (int i = 0; i < dictionaryLength - 1; i++) {
        // check that the potential match has the include-flag character
        if (mustInclude) {
            if (first_index_of(dictionaryWords[i], mustInclude,
                    strlen(dictionaryWords[i])) == -1) {
                continue;
            }
        }
        
         
        match = str_match(letters, dictionaryWords[i], cmpRemoveLetters);
        if (match) {
            matchedDictionaryIndices[*numberOfMatches] = i;
            *numberOfMatches = *numberOfMatches + 1;
            matchedDictionaryIndices =
                    (int *)check_null_pointer(
                    realloc(matchedDictionaryIndices,
                    (*numberOfMatches + 1) * sizeof(int)));
        }
         
        strcpy(cmpRemoveLetters[0], letters);
    }
    
    free(cmpRemoveLetters[0]);
    free(cmpRemoveLetters);
    return matchedDictionaryIndices;
}

/*
 * Checks for a match between the given letters and the word. 
 * char* letters: the letters that can be used
 * char* dictionaryWord: the word we try to construct
 *
 * ret <int>: a boolean value, 1 if the word is a match, otherwise 0
 */
int str_match(char *letters, char *dictionaryWord, char **cmpRemoveLetters)
{
    if (strlen(dictionaryWord) > strlen(letters)) {
        return 0;
    } else if (strlen(dictionaryWord) < MIN_WORD_LEN) {
        return 0;
    }
    
    
    for (int i = 0; i < strlen(dictionaryWord); i++) {
        int indexOfDictChar =
                first_index_of(cmpRemoveLetters[0], dictionaryWord[i],
                strlen(letters));

        if (indexOfDictChar == -1) {
            return 0;
        } else {
            // removed characters are spaces because null would end the string
            cmpRemoveLetters[0][indexOfDictChar] = ' ';
        }
    }

    return 1;
}

/*
 * Reads the number of lines in a file.
 * FILE *dictionaryFile: the file to be read
 *
 * ret <int>: the number of lines in the file.
 */
int count_lines(FILE *dictionaryFile) {
    int lineCount = 0;
    char lineCounter;
    fpos_t fileStartReadPosition;
    fgetpos(dictionaryFile, &fileStartReadPosition);

    while (1) {
        lineCounter = fgetc(dictionaryFile);
        if (lineCounter == '\n') {
            lineCount++;
        } else if (lineCounter == EOF) {
            break;
        }
    }
    
    fsetpos(dictionaryFile, &fileStartReadPosition);
    return lineCount;
}

/*
 * Reads a dictionary file into an array of strings.
 * FILE *dictionaryFile: the file to be read
 * int *dictionaryLength: pointer to the length (in lines) of the dict file
 *
 * ret <char**>: array of strings read from the file.
 */
char **read_dict_file(FILE *dictionaryFile, int *dictionaryLength)
{
    int lineCount = count_lines(dictionaryFile);
    
    char **dictionaryWords =
            (char **)check_null_pointer(
            malloc((lineCount + 1) * sizeof(char *)));
    for (int i = 0; i <= lineCount; i++) {
        dictionaryWords[i] = 
                (char *)check_null_pointer(
                calloc(MAX_WORD_LEN + 1, sizeof(char)));
    }

    char nextRead;
    int wordCounter = 0;
    int charCounter = 0;

    // now actually read the file into the pointer-pointer.
    nextRead = fgetc(dictionaryFile);
    while (1) {
        if (nextRead == EOF) {
            break;
        } else if (nextRead == '\n') {
            dictionaryWords[wordCounter][charCounter] = '\0';
            wordCounter += 1;
            charCounter = 0;
        } else if (charCounter >= MAX_WORD_LEN) {
            // line too long, throw it out
            memset(dictionaryWords[wordCounter], '\0', charCounter);
            wordCounter += 1;
            charCounter = 0;
            while (nextRead != '\n') {
                nextRead = fgetc(dictionaryFile);
            }
        } else {
            dictionaryWords[wordCounter][charCounter] = nextRead;
            charCounter += 1;
        }
        nextRead = fgetc(dictionaryFile);
    }

    *dictionaryLength = wordCounter + 1;
    fclose(dictionaryFile);
    return dictionaryWords;
}

/*
 * Opens the dictionary file to be read, with provided or default file address
 * char* dictAddr: the file address of the dictionary file.
 * FILE** dictionaryFile: pointer to file pointer, so we can set file pointer
 *
 * ret <int>: any relevant error codes encountered in opening the file
 */
int open_dict_file(char *dictAddr, FILE **dictionaryFile)
{
    if (strlen(dictAddr) > 0 && dictAddr[0] != ' ') {
        *dictionaryFile = fopen(dictAddr, "r");
    } else {
        *dictionaryFile = fopen(DEFAULT_DICT, "r");
    }
     
    if (*dictionaryFile == NULL) {
        return BAD_DICT_FILENAME_ERRC;
    } else {
        return 0;
    }
}

/*
 * Finds the "letters" argument as the first argument with no preceding -
 * that is also not the value of an "include".
 *
 * int argc: number of command args
 * char **argv: command args
 *
 * ret <int>: the index of the "letters" arg in argv, or a negative error code
 */
int find_letters(int argc, char **argv) {
    int index = -1;
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != FLAG_ID && strcmp(argv[i - 1], includeArg) != 0) {
            index = i;
            break;
        }
    }
   
    if (index == -1) {
        return -BAD_COMMAND_ARGS_ERRC;
    } else if (strlen(argv[index]) < MIN_WORD_LEN) {
        return -BAD_LEN_OF_LETTERS_ERRC;
    }
    
    for (int i = 0; i < strlen(argv[index]); i++) {
        if (!isalpha(argv[index][i])) {
            return -BAD_CHAR_IN_LETTERS_ERRC;
        }
    }
    
    return index;
}

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
        int argc, char **argv)
{
    if (argc > MAX_CMD_ARGS || argc < MIN_CMD_ARGS) {
        initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    }
    
    int lettersIndex = find_letters(argc, argv);
    if (lettersIndex > 0) {
        strcpy(initInfoSpace->letters, argv[lettersIndex]);
    } else {
        initInfoSpace->errorCode = -lettersIndex;
        return;
    }

    // >1 arg after letters: error; 1 arg after: set dictAddr
    if (lettersIndex < (argc - 2)) {
        initInfoSpace[0].errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    } else if (lettersIndex == (argc - 2)) {
        strcpy(initInfoSpace[0].dictAddr, argv[argc - 1]);
    }
    
    switch (lettersIndex) {
        case 2:
            letters_in_pos_two(initInfoSpace, argc, argv);
            break;
        case 3:
            letters_in_pos_three(initInfoSpace, argc, argv);
            break;
        case 4:
            letters_in_pos_four(initInfoSpace, argc, argv);
            break;
    }
    
    return;
}

/*
 * handles the case for if letters is the second argument provided
 *
 * struct UserSuppliedInformation *initInfoSpace: struct for parsed args
 * int argc: number of args
 * char **argv: command args
 */
void letters_in_pos_two(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv) 
{
    if (strcmp(argv[1], alphaArg) == 0) {
        initInfoSpace[0].sortOrder = ALPHA_SORT;
    } else if (strcmp(argv[1], lenArg) == 0) {
        initInfoSpace[0].sortOrder = LEN_SORT;
    } else if (strcmp(argv[1], longestArg) == 0) {
        initInfoSpace[0].sortOrder = LONGEST_SORT;
    } else {
        initInfoSpace[0].errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    }

    return;
}

/*
 * handles the case for if letters is the third argument provided
 *
 * struct UserSuppliedInformation *initInfoSpace: struct for parsed args
 * int argc: number of args
 * char **argv: command args
 */
void letters_in_pos_three(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv)
{
    if (strcmp(argv[1], includeArg) == 0) {
        if (strlen(argv[2]) == 1) {
            initInfoSpace->mustInclude = argv[2][0];
        } else {
            initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
            return;
        }
    } else {
        initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    }

    return;
}

/*
 * handles the case for if letters is the fourth argument provided
 *
 * struct UserSUppliedInformation *initInfoSpace: struct for parsed args
 * int argc: number of args
 * char **argv: command args
 */
void letters_in_pos_four(struct UserSuppliedInformation *initInfoSpace,
        int argc, char **argv)
{
    if (strcmp(argv[1], alphaArg) == 0) {
        initInfoSpace->sortOrder = ALPHA_SORT;
    } else if (strcmp(argv[1], lenArg) == 0) {
        initInfoSpace->sortOrder = LEN_SORT;
    } else if (strcmp(argv[1], longestArg) == 0) {
        initInfoSpace->sortOrder = LONGEST_SORT;
    } else if (strcmp(argv[1], includeArg) == 0) {
        if (strlen(argv[2]) == 1) {
            initInfoSpace->mustInclude = argv[2][0];
        } else {
            initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
            return;
        }
    } else {
        initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    }
	
    if (strcmp(argv[2], includeArg) == 0) {
        if (strlen(argv[3]) == 1) {
            initInfoSpace->mustInclude = argv[3][0];
        } else {
            initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
            return;
        }
    } else if (strcmp(argv[3], alphaArg) == 0) {
        initInfoSpace->sortOrder = ALPHA_SORT;
    } else if (strcmp(argv[3], lenArg) == 0) {
        initInfoSpace->sortOrder = LEN_SORT;
    } else if (strcmp(argv[3], longestArg) == 0) {
        initInfoSpace->sortOrder = LONGEST_SORT;
    } else {
        initInfoSpace->errorCode = BAD_COMMAND_ARGS_ERRC;
        return;
    }

    return;
}

struct UserSuppliedInformation create_user_info_storage(int argc, char **argv)
{
    struct UserSuppliedInformation parsedArgs;
    
    int longestArgLen = 0;
    for (int i = 0; i < argc; i++) {
        if (strlen(argv[i]) > longestArgLen) {
            longestArgLen = strlen(argv[i]);
        }
    }
    
    parsedArgs.dictAddr = 
            (char *)check_null_pointer(malloc(
            (longestArgLen + 1) * sizeof(char)));
    
    memset(parsedArgs.dictAddr, ' ', (size_t)(longestArgLen + 1));
    parsedArgs.dictAddr[longestArgLen] = '\0';
    
    parsedArgs.letters = 
            (char *)check_null_pointer(malloc(
            (longestArgLen + 1) * sizeof(char)));
    memset(parsedArgs.dictAddr, ' ', (size_t)(longestArgLen + 1));
    parsedArgs.dictAddr[longestArgLen] = '\0';
    
    parsedArgs.sortOrder = NO_SORT;
    parsedArgs.mustInclude = '\0';
    parsedArgs.errorCode = 0;
    
    parse_cmd_args(&parsedArgs, argc, argv);

    if (parsedArgs.mustInclude != '\0' && !isalpha(parsedArgs.mustInclude)) {
        parsedArgs.errorCode = BAD_COMMAND_ARGS_ERRC;
    }
    
    return parsedArgs;
}

/*
 * Returns the index of a given item in a string.
 * char *check: the string to be checked
 * char comp: the comparison character
 * int checkLen: the length of the above array
 *
 * ret <int>: the index of the first occurrence of <comp> in <check>.
 */
int first_index_of(char *check, char comp, int checkLen)
{
    for (int i = 0; i < checkLen; i++) {
        if (tolower(check[i]) == tolower(comp)) {
            return i;
        }
    }
    return -1;
}

/*
 * Checks a given pointer for null and potentially raises errors.
 * void to void so that it can be directly used on malloc/calloc/realloc.
 * void* newMemAlloc: a pointer to a freshly-allocated block of memory
 *
 * ret <void*>: the same pointer it was given, but guaranteed to not be null.
 */
void *check_null_pointer(void *newMemAlloc)
{
    if (!newMemAlloc) {
        fprintf(stderr, NULL_POINTER_ERRM);
        raise(SIGABRT);
    }

    return newMemAlloc;
}
