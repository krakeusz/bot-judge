#include <iostream>
#include <string>

using namespace std;

int main()
{
  while (true)
  {
    string command;
    getline(cin, command);
    if (command == "MOVE")
    {
      cout << "SCISSORS" << endl;
    }
    else
    {
      cerr << "unknown command" << endl;
    }
  }
  return 0;
}
