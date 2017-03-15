#include <iostream>
#include <string>
#include <ctime>

using namespace std;

int main(int argc, char** argv)
{
  unsigned seed = time(NULL);
  if (argc >= 2)
    seed = atoi(argv[1]);
  srand(seed);

  while (true)
  {
    string command;
    getline(cin, command);
    if (command == "MOVE")
    {
      switch (rand() % 3)
      {
        case 0:
          cout << "SCISSORS" << endl;
          break;
        case 1:
          cout << "ROCK" << endl;
          break;
        case 2:
          cout << "PAPER" << endl;
          break;
      }
    }
    else
    {
      cerr << "unknown command" << endl;
    }
  }
  return 0;
}
