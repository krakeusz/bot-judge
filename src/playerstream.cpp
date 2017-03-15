#include "playerstream.h"

#include <cassert>
#include <cstring>
#include <signal.h> // sigaction
#include <sys/select.h> // select, timeval
#include <unistd.h> // read

playerbuf::playerbuf(int input_fd, int output_fd)
  : input_fd_(input_fd),
    output_fd_(output_fd),
    readbuf_(nullptr),
    writebuf_(nullptr),
    timeout_(),
    last_error_(0)
{
  if (input_fd >= 0)
    readbuf_ = new char[BUF_SIZE];
  if (output_fd >= 0)
  {
    writebuf_ = new char[BUF_SIZE];
    setp(writebuf_, writebuf_ + BUF_SIZE);
  }
}

void playerbuf::set_timeout_ms(int timeout_ms)
{
  if (timeout_ == nullptr)
  {
    timeout_.reset(new timeval);
  }
  timeout_->tv_sec = timeout_ms / 1000;
  timeout_->tv_usec = 1000 * (timeout_ms % 1000);
}

playerbuf::~playerbuf()
{
  delete readbuf_;
  delete writebuf_;
}

void playerbuf::throw_last_error(const playerbuf& /*sender*/, int errnum)
{
  throw playerbuf_error(strerror(errnum), std::error_code(errnum, std::system_category()));
}

int playerbuf::underflow()
{
  if (gptr() == egptr())
  {
    // No more data in internal buffer.
    // Fetch more data from file descriptor.
    fd_set set;
    FD_ZERO(&set); // clear the set
    FD_SET(input_fd_, &set); // add our file descriptor to the set
    // TODO: this works only on Linux. See final paragraphs of man 2 select.
    int rv = select(input_fd_ + 1, &set, nullptr, nullptr, timeout_.get());
    if (rv == -1)
    {
      last_error_ = errno;
      call_on_error();
      return traits_type::eof();
    }
    else if (rv == 0)
    {
      // timeout
      last_error_ = ETIME;
      call_on_error();
      return traits_type::eof();
    }
    else
    {
      rv = read(input_fd_, readbuf_, BUF_SIZE);
      if (rv == -1)
      {
        last_error_ = errno;
        call_on_error();
        return traits_type::eof();
      }
      else if (rv == 0)
      {
        // No characters were read.
        // In that case, this is the real EOF.
        return traits_type::eof();
      }
      // Set the interfal streambuf pointers to our readbuf_ data.
      setg(readbuf_, readbuf_, readbuf_ + rv);
    }
  }
  assert(gptr() != egptr());
  return traits_type::to_int_type(*gptr());
}

int playerbuf::overflow(int c)
{
  // No more place in internal buffer for written data.
  // Or, pptr == epptr == nullptr.
  // Flush the buffer, then start writing at the beginning.
  if (sync() != 0)
    return traits_type::eof();
  assert(pptr() != epptr());

  if (c == EOF)
    return traits_type::eof();

  int rv = traits_type::to_int_type(c);
  *pptr() = c;
  pbump(1);
  return rv;
}

int playerbuf::sync()
{
  // Flush the buffer.
  int chars_left = pptr() - pbase();
  while (chars_left > 0)
  {
    // It is possible that write(2) doesn't write everything at once.
    // Write in loop, then.
    int rv = write(output_fd_, pptr() - chars_left, chars_left);
    if (rv <= 0)
    {
      last_error_ = errno;
      setp(pbase() - chars_left, writebuf_ + BUF_SIZE);
      call_on_error();
      return -1;
    }
    chars_left -= rv;
  }
  setp(writebuf_, writebuf_ + BUF_SIZE);
  return 0;
}

void playerbuf::call_on_error() const
{
  if (on_error_)
    on_error_(*this, last_error_);
}

std::string playerbuf::get_last_strerror() const
{
  std::string result = "EOF";
  if (last_error_ != 0)
  {
    result += ": ";
    result += strerror(last_error_);
  }
  return result;
}

void playerstream_base::ignore_sigpipe()
{
  struct sigaction action;
  action.sa_handler = SIG_IGN;
  sigemptyset(&(action.sa_mask));
  action.sa_flags = 0;
  sigaction(SIGPIPE, &action, nullptr);
}

playerbuf_error::playerbuf_error(std::string msg, std::error_code err_code)
  : what_(err_code.message() + ": " + msg), err_code_(err_code)
{
}

const char* playerbuf_error::what() const noexcept
{
  return what_.c_str();
}

std::error_code playerbuf_error::get_error_code() const
{
  return err_code_;
}

