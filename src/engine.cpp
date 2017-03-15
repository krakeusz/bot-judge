#include "engine.h"

#include <sstream>

namespace Engine
{

GameResult::GameResult() : type(EngineError), pretty_result("undefined error")
{
}

GameResult GameResult::createWin(
      const std::vector<PlayerData>& players,
      const PlayerData& winner,
      std::string result_details)
{
  GameResult result;
  result.type = ResultType::Win;
  for (int i = 0; i < static_cast<int>(players.size()); i++)
  {
    result.player_scores.push_back(i == winner.getPlayerId() ? 1.0 : 0.0);
  }
  std::ostringstream pretty;
  pretty << "Player #" << winner.getPlayerId() << " (" << winner.getProgramName()
         << ") won [" << result_details << "]";
  result.pretty_result = pretty.str();
  return result;
}

GameResult GameResult::createDraw(
      const std::vector<PlayerData>& players,
      std::string result_details)
{
  GameResult result;
  result.type = ResultType::Draw;
  result.player_scores = std::vector<double>(players.size(), 0.5);
  std::ostringstream pretty;
  pretty << "Draw [" << result_details << "]";
  result.pretty_result = pretty.str();
  return result;
}

GameResult GameResult::createError(
      const std::vector<PlayerData>& players,
      std::string error_details)
{
  GameResult result;
  result.type = ResultType::EngineError;
  result.player_scores = std::vector<double>(players.size(), 0.0);
  std::ostringstream pretty;
  pretty << "Match aborted due to engine error: " << error_details;
  result.pretty_result = pretty.str();
  return result;
}

PlayerData::PlayerData(filedesc_t read_fd, filedesc_t write_fd, filedesc_t error_fd,
             std::string p_name, int p_id)
  : program_name(std::move(p_name)), player_id(p_id)
{
  player_stream.reset(new playerstream(read_fd, write_fd));
  error_stream.reset(new oplayerstream(error_fd));
}

} // namespace Engine
