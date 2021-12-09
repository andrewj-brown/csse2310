# csse2310
The four assignments (+1 bonus project) from my time studying CSSE2310 - Computer Systems Principles & Programming.

## a1 - unjumble
unjumble is a program that can tell us all the anagrams and shorter words that we can make from a given set of letters. It does this by opening and reading either a given or default dictionary and comparing each word to our set of letters. Additional control is given to the user by sorting output according to length, alphabetical order, or only outputting the longest (or tied longest) words. unjumble is fully functional with no memory leaks. Builds with `make` and tested on AlmaLinux 8.4.

## a2 - bomb
The bomb is not a coding assessment task, rather, a debugging and reverse-engineering one. The bomb connects to a pre-set logging database, inaccessible to students (and rendering the bomb unusable outside of the student testing environment). It provides a series of increasingly complex functions to debug and determine the output of in order to "defuse" the bomb, with marks deducted for failed attempts. A writeup of how the challenges were completed is provided.

## a3 - jobrunner
jobrunner is a program to handle large-scale, multiprocess running and handling of terminal utilities. jobrunner reads terminal program "jobs" (like ls, cat, or echo) from a file and sets up pipes between jobs, inputs from files, and outputs to files as specified by the user, before forking and executing the given jobs. The program is fully functional with no memory leaks or errors, though some bugs are present in determining which problem should be reported to the user. Builds with `make` and tested on AlmaLinux 8.4.

## a4 - intsuite
intsuite is a pair of programs (intclient and intserver) that support remote, multithreaded calculation of integrals. intclient reads integration "jobs" from stdin or other file and makes HTTP requests to intserver for validation and calculation, while intserver awaits incoming connections and spawns threads to handle and compute requests. The pair of programs are fully functional, albeit with one known memory bug in each program. Builds with `make` and tested on AlmaLinux 8.4.

## misc - pokemongame
pokemongame is a duplicate of an assignment from another course (CSSE1001) that I decided to recreate early in the semester (before the first assignment) as an introduction to C programming and memory management. It's a terminal-based minesweeper clone, where you supply cells to "dig" or "flag" using a battleships-like grid system. The game is fully functional and playable, and builds with `make`. Tested on AlmaLinux 8.4.
