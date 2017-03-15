#ifndef ENGINE_H
#define ENGINE_H

#include "common.h"
#include "playerstream.h"

#include <vector>
#include <string>

namespace Engine
{

/// This struct represents a player program to the engine.
class PlayerData
{
public:
  ///
  /// \param read_fd A pipe to read from program (connected to program's stdout).
  /// \param write_fd A pipe to write to program (connected to program's stdin).
  /// \param err_fd A file that the program will write stderr to. The engine can also write there.
  /// \param program_name A string with program name (doesn't need to be unique).
  /// \param player_id 0-based index of described player. The same player order is used in play_game(players).
  PlayerData(filedesc_t read_fd, filedesc_t write_fd, filedesc_t err_fd,
             std::string program_name, int player_id);

  const std::string& getProgramName() const { return program_name; }
  int getPlayerId() const { return player_id; }

  playerstream& playerStream() { return *player_stream; }
  std::ostream& errorStream() { return *error_stream; }

  PlayerData& operator=(const PlayerData&) = delete;
  PlayerData& operator=(PlayerData&&) = default;
  PlayerData(const PlayerData&) = delete;
  PlayerData(PlayerData&&) = default;

private:
  std::string                   program_name;
  int                           player_id;
  std::unique_ptr<playerstream> player_stream;
  std::unique_ptr<std::ostream> error_stream;
};

/// This struct holds the results of a single battle.
struct GameResult
{
  enum ResultType
  {
    Win,
    Draw,
    EngineError
  };

  ResultType type;
  std::vector<double> player_scores; ///< values in [0;1]. As many values as there are players.
  std::string pretty_result;

  static GameResult createWin(
      const std::vector<PlayerData>& players,
      const PlayerData& winner,
      std::string result_details = "");

  static GameResult createDraw(
      const std::vector<PlayerData>& players,
      std::string result_details = "");

  static GameResult createError(
      const std::vector<PlayerData>& players,
      std::string error_details);

private:
  GameResult();
};

/// This is the main engine's function.
///
/// It should play the game with the programs that
///   are already instantiated as child processes.
/// The pipes are the way to communicate with the programs.
GameResult play_game(std::vector<PlayerData>& players) noexcept;

} // end namespace Engine

#endif /* ENGINE_H */
