# bot-judge
A library for creating judge programs, for use in bot programming contests.

In bot programming contests, you usually need to write a program that reads
instructions (usually current game state) from standard input and writes the
selected move to the standard output. See https://www.codingame.com as an example website that holds
such contests.

If you want to actually conduct a game between bots, you need to create a "judge" for that specific game.
The judge source code will require reading/writing messages in text format to the competing game bots as well as
implementing game rules and calculating current game state.

This project aims to make the task of writing judges easy. The program will fork twice,
and each fork will transmute (exec) into a bot (which will be judged). Also, the standard input/output
of the bots will be redirected to your judge. The judge will see that text data using the standard C++ stream abstractions.
Internally, the programs are communicating using unnamed Linux pipes.

The judge can be especially useful when you're participating in a contest.
You can easily run different versions of your bot against each other
and check which one performs better.

The judge needs to be written in C++11 (at least).
On the other hand, the bots which will be judged can be written in any
language as long as the programs can be run from command-line.

# Targeted environments

- Linux only.

# Features

- Redirect stdin/stdout to/from 2 bot programs
- Use a convenient C++ stream abstraction to communicate from judge
- Allow returning errors when reading from a bot takes too much time (configurable, run-time parameter)
- Run the bots supplying a pseudo-random value in their command-line parameters
(useful if the bots use that value as a seed for their RNG,
in the sense that running the bots multiple times will result in different playthroughs)

# Usage

How to implement a judge for a given game:

- Implement the following function (in namespace Engine):
```c++
#include "engine.h"
GameResult 	play_game (std::vector< PlayerData > &players) noexcept
```
This function should play one game between two bots.
The bots' data (their names and associated streams) are given as the 'players' parameter.
- Compile your code. You will need to add inc/ folder to your include path
(in case of g++/clang, add -I<path_to_project_root>/inc as a compiler flag).
- (once only) Compile the bot-judge project:
```bash
cd src/
make
```
- Link your judge against build/libengine_main.a
- Run your judge like this:
```bash
./judge ./program1 ./program2
```
The judge will run 10 games between program1 and program2, assuming that their binaries
are located in current directory.

# Example

In the example/ folder you can find a sample judge for a Rock-Paper-Scissors game.
Run the example with the following commands:
```bash
cd src/
make
cd ..
cd example/rsp
make
./rsp_engine ./random ./rock
# or any other combination of bots, their binaries are in current folder
```

# Documentation
Can be automatically generated.
```bash
cd doc
doxygen
```
And then run doc/html/index.html in your favourite browser.

# For bot-judge developers
If you want to run unit tests, make sure that you have installed Google Test, and then:
```bash
cd test
make
```

