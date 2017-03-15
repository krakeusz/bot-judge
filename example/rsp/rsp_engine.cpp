// Rock, Scissors, Paper engine

#include "engine.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>

namespace Engine
{

using std::array;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

class Choice
{
public:
  virtual bool losesAgainstPaper() const = 0;
  virtual bool losesAgainstRock() const = 0;
  virtual bool losesAgainstScissors() const = 0;
  virtual bool beats(const Choice& choice) const = 0;
};

class Rock : public Choice
{
public:
  virtual bool losesAgainstPaper() const override { return true; }
  virtual bool losesAgainstRock() const override { return false; }
  virtual bool losesAgainstScissors() const override { return false; }
  virtual bool beats(const Choice& choice) const override { return choice.losesAgainstRock(); }
};

class Paper : public Choice
{
public:
  virtual bool losesAgainstPaper() const override { return false; }
  virtual bool losesAgainstRock() const override { return false; }
  virtual bool losesAgainstScissors() const override { return true; }
  virtual bool beats(const Choice& choice) const override { return choice.losesAgainstPaper(); }
};

class Scissors : public Choice
{
public:
  virtual bool losesAgainstPaper() const override { return false; }
  virtual bool losesAgainstRock() const override { return true; }
  virtual bool losesAgainstScissors() const override { return false; }
  virtual bool beats(const Choice& choice) const override { return choice.losesAgainstScissors(); }
};

void ignoreLine(std::istream& istr)
{
  istr.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

unique_ptr<Choice> choiceFromString(const string& str)
{
  string upperStr = str;
  if (str == "ROCK")
  {
    return unique_ptr<Choice>(new Rock);
  }
  else if (str == "PAPER")
  {
    return unique_ptr<Choice>(new Paper);
  }
  else if (str == "SCISSORS")
  {
    return unique_ptr<Choice>(new Scissors);
  }
  return nullptr;
}

constexpr int PLAYERS = 2;

GameResult play_game(vector<PlayerData>& players) noexcept
{
  try {
    if (players.size() != PLAYERS)
    {
      return GameResult::createError(players,
          "This game is meant for " + to_string(PLAYERS) + " players only");
    }
    array<int, PLAYERS> winCount = { 0, 0 };
    constexpr int ROUNDS = 10;
    for (int i = 0; i < ROUNDS; i++)
    {
      vector<unique_ptr<Choice>> choices;
      for (auto& player: players)
      {
        auto& stream = player.playerStream();
        string response;
        stream.set_timeout_ms(100);
        stream << "MOVE" << std::endl;
        stream >> response;
        ignoreLine(stream);
        if (stream.eof())
        {
          string details =
            string("Win by opponent error: ") + stream.get_last_strerror();
          return GameResult::createWin(players, players[1-player.getPlayerId()], details);
        }
        auto choiceP = choiceFromString(response);
        if (!choiceP)
        {
          string details =
            "Win by opponent error: move not recognized: '"
            + response + "'";
          return GameResult::createWin(players, players[1-player.getPlayerId()], details);
        }
        choices.push_back(std::move(choiceP));
      }
      if (choices[0]->beats(*choices[1]))
        winCount[0]++;
      if (choices[1]->beats(*choices[0]))
        winCount[1]++;
    }
    int winnerId = (winCount[0] > winCount[1] ? 0 : 1);
    string details =
      to_string(winCount[winnerId]) + "-" + to_string(winCount[1-winnerId]);
    if (winCount[0] == winCount[1])
    {
      return GameResult::createDraw(players, details);
    }
    return GameResult::createWin(players, players[winnerId], details);
  } catch (std::exception& e)
  {
    return GameResult::createError(players, e.what());
  }
}

} // namespace Engine
