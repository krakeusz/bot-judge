/*
 * Assumptions:
 * 1. The judge is not resistant to engine bugs/hack attempts.
 *    It is not safe to run the judge against 3rd-party engines.
 *
 */

#include "common.h"
#include "engine.h"
#include "err.h"

#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <cassert>
#include <cstdio>
#include <ctime>
#include <string>
#include <sstream>
#include <vector>

using namespace std;
using Engine::play_game;
using Engine::GameResult;
using Engine::PlayerData;

// TODO: use C++11 random numbers

constexpr int NUM_PROGRAMS = 2;

const char * LOG_FOLDER = "logs/";

static void command(const char* cmd)
{
  int retcode = system(cmd);
  if (retcode != 0)
  {
    fprintf(stderr, "command '%s' failed", cmd);
    exit(retcode);
  }
}

static void remove_folder(const char* path)
{
  ostringstream cmd;
  cmd << "rm -rf " << path;
  command(cmd.str().c_str());
}

static void make_folder(const char* path)
{
  ostringstream cmd;
  cmd << "mkdir -p " << path;
  command(cmd.str().c_str());
}

static string get_battle_folder_path(int battle_id)
{
  ostringstream folder;
  folder << LOG_FOLDER << battle_id << '/';
  return folder.str();
}

static string get_filename(const string& path)
{
  size_t startPos = path.find_last_of('/');
  if (startPos == string::npos)
    startPos = 0;
  else
    startPos++;
  return path.substr(startPos);
}

static string get_battle_stderr_path(int battle_id, int program_id, const string& process_name)
{
  ostringstream path;
  path << get_battle_folder_path(battle_id) << program_id << "."
       << get_filename(process_name) << ".err";
  return path.str();
}

static void make_battle_folder(int battle_id)
{
  make_folder(get_battle_folder_path(battle_id).c_str());
}

static vector<double> play_match(vector<string> programs, int battle_id)
{
  assert(programs.size() == NUM_PROGRAMS);
  make_battle_folder(battle_id);
  int read_pipes[NUM_PROGRAMS][2];
  int write_pipes[NUM_PROGRAMS][2];
  int err_files[NUM_PROGRAMS];
  vector<pid_t> children_pids;

  for (int i = 0; i < NUM_PROGRAMS; i++)
  {
    SYSCALL_WITH_CHECK(pipe(read_pipes[i]));
    SYSCALL_WITH_CHECK(pipe(write_pipes[i]));
    string err_file_path = get_battle_stderr_path(battle_id, i, programs[i]);
    cerr << "Creating error file " << err_file_path << endl;
    SYSCALL_WITH_CHECK(err_files[i] = open(err_file_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0640));

    pid_t child_pid;
    unsigned seed = rand();
    switch ((child_pid = fork())) {
      case -1:
        syserr("Error in fork\n");

      case 0:
        // Child process

        // Close unused pipes (these ends are used by parent).
        // Close also pipes of our older brothers, they were "inherited".
        for (int j = 0; j <= i; j++)
        {
          SYSCALL_WITH_CHECK(close(read_pipes[j][PIPE_READ_END]));
          SYSCALL_WITH_CHECK(close(write_pipes[j][PIPE_WRITE_END]));
        }
        // Close err files of our older brothers.
        for (int j = 0; j <= i - 1; j++)
        {
          SYSCALL_WITH_CHECK(close(err_files[j]));
        }

        // Redirect write_pipe to child's stdin
        SYSCALL_WITH_CHECK(close(fileno(stdin)));
        SYSCALL_WITH_CHECK(dup(write_pipes[i][PIPE_READ_END]));

        // Redirect child's stdout to read_pipe
        SYSCALL_WITH_CHECK(close(fileno(stdout)));
        SYSCALL_WITH_CHECK(dup(read_pipes[i][PIPE_WRITE_END]));

        // Redirect child's stderr to err_pipe
        SYSCALL_WITH_CHECK(close(fileno(stderr)));
        SYSCALL_WITH_CHECK(dup(err_files[i]));

        // Transmute into target executable.
        execlp(programs[i].c_str(), programs[i].c_str(), to_string(seed).c_str(), nullptr);

        syserr("Cannot use/find the program binary on $PATH: %s\n", programs[i].c_str());

      default:
        // Parent process
        children_pids.push_back(child_pid);
        // Close the pipes that are used only by the youngest child.
        SYSCALL_WITH_CHECK(close(write_pipes[i][PIPE_READ_END]));
        SYSCALL_WITH_CHECK(close(read_pipes[i][PIPE_WRITE_END]));

    } /* switch */

  }

  vector<Engine::PlayerData> players;
  for (int i = 0; i < NUM_PROGRAMS; i++)
  {
    players.emplace_back(
        read_pipes[i][PIPE_READ_END],
        write_pipes[i][PIPE_WRITE_END],
        err_files[i],
        programs[i], i);
  }

  GameResult result = play_game(players);
  cout << result.pretty_result << endl;

  // Terminate programs and clean the resources
  for (pid_t child_pid: children_pids)
    SYSCALL_WITH_CHECK(kill(child_pid, SIGKILL));

  for (size_t i = 0; i < children_pids.size(); i++)
    wait(nullptr);

  return result.player_scores;
}

template <class T>
vector<T>& operator+=(vector<T>& v1, const vector<T>& v2)
{
  for (size_t i = 0; i < min(v1.size(), v2.size()); i++)
  {
    v1[i] += v2[i];
  }
  return v1;
}

int main(int argc, char* argv[])
{
  srand(time(NULL));
  if (argc != NUM_PROGRAMS + 1)
  {
    fprintf(stderr, "USAGE: %s <program1> <program2>\n", argv[0]);
    return 1;
  }
  // Make sure that a write to broken pipe will not terminate the program.
  playerstream_base::ignore_sigpipe();
  remove_folder(LOG_FOLDER);
  vector<double> match_scores(NUM_PROGRAMS);
  const int reps = 10;
  for (int i = 0; i < reps; i++)
    match_scores += play_match({ argv[1], argv[2] }, i);
  cout << "Final scores:" << endl;
  for (int i = 0; i < NUM_PROGRAMS; i++)
    cout << "Bot #" << i << "(" << argv[i+1] << ") has total score " << match_scores[i] << endl;
}
