#ifndef PLAYERSTREAM_H
#define PLAYERSTREAM_H

#include <functional>
#include <iostream>
#include <memory>
#include <streambuf>
#include <system_error>

/// \brief A streambuf that reads data from a file descriptor and allows setting timeouts on reads.
class playerbuf : public std::streambuf
{
public:
  /// \brief Create a streambuf reading/writing data from/to the given file descriptor(s).
  ///
  /// By default, the playerbuf will not have set any timeout of error handlers.
  /// If one of the file descriptors is < 0, then the corresponding operations
  ///  (reading/writing) are undefined. No internal buffers will be allocated
  ///  for that operation.
  playerbuf(int input_fd, int output_fd);

  ~playerbuf();

  static constexpr int BUF_SIZE = 1024;

  /// \brief Sets the timeout for next read operations.
  ///
  /// The timeout will be active until it is set again, cleared or times out.
  /// Only the time waiting for file descriptor to contain data will be measured.
  /// Note that if some data has already been buffered and not consumed,
  ///  then no time will be subtracted.
  /// When it times out, the current and subsequent read operations will return EOF.
  /// It is possible that the timeout will be exhausted only after several blocking read operations.
  void        set_timeout_ms(int timeout_ms);

  /// \brief The type for error callbacks.
  ///
  /// The first parameter is *this.
  /// The second parameter is error number (see errno).
  using error_fun_t = std::function<void(const playerbuf& sender, int errnum)>;

  /// \brief Set the error handling method.
  ///
  /// The given function will be called whenever there is an error.
  /// Errors include: a timeout while waiting for next characters, or system call errors (read(2), select(2) etc) in input operation only.
  /// No exceptions will be thrown on write operations, but badbit flag will be set instead.
  /// After the call, the previous error handler will be detached (along with exception-throwing handler).
  /// Note that simple end-of-file caused by, for instance, closing a pipe, will not trigger an error.
  inline void on_error_call(error_fun_t error_fun);

  /// \brief Set the error handling method to throw playerbuf_error exception.
  ///
  /// Errors include: a timeout while waiting for next characters, or system call errors (read(2), select(2) etc).
  /// After the call, the previous error handler will be detached (along with callback-invoking handler).
  /// Note that a simple end-of-file caused by, for instance, closing a pipe, will not trigger an error.
  /// If an exception is thrown after an internal error, then the appriopriate ios_base flags (eof, fail, bad)
  ///  will NOT be set.
  inline void on_error_throw();

  /// \brief Detach the previous error handler, if exists (exception-throwing / callback-calling).
  inline void on_error_no_op();

  /// \brief Return the last error that occured in this buffer.
  ///
  /// No error is marked by the value of 0. For example, a simple EOF with no errors will result in 0.
  inline int  get_last_error() const;

  std::string get_last_strerror() const;

  playerbuf(const playerbuf&) = delete;
  playerbuf& operator=(const playerbuf&) = delete;

protected:
  int underflow() override;
  int overflow(int c) override;
  int sync() override;

private:
  static void throw_last_error(const playerbuf& sender, int errnum);
  void        call_on_error() const;

  int                      input_fd_;   ///< the input file descriptor to read from
  int                      output_fd_;  ///< the output file descriptor to read from
  char*                    readbuf_;    ///< the local buffer of characters read from input_fd
  char*                    writebuf_;   ///< the local buffer of characters to be written to output_fd
  std::unique_ptr<timeval> timeout_;    ///< the remaining timeout. If nullptr, then no timeout is set.
  error_fun_t              on_error_;   ///< the function that will be called on next error. Can be set to none.
  int                      last_error_; ///< the last error number that occured in this buffer.
};

class playerstream_base
{
public:
  /// Calls playerbuf::set_timeout_ms.
  inline void set_timeout_ms(int timeout_ms);

  /// Calls playerbuf::on_error_call.
  inline void on_error_call(playerbuf::error_fun_t error_fun);

  /// Calls playerbuf::on_error_throw.
  inline void on_error_throw();

  /// Calls playerbuf::on_error_no_op.
  inline void on_error_no_op();

  /// Calls playerbuf::get_last_error.
  inline  int get_last_error() const;

  /// Calls playerbuf::get_last_strerror.
  inline std::string get_last_strerror() const;

  /// \brief Makes sure that the SIGPIPE signal is ignored.
  ///
  /// This is a helper function. It will ignore the signal until
  ///  the current process ends or the signal is un-ignored by other means.
  /// This signal can appear when we invoke write
  ///  on a pipe with closed read end (see man 2 write).
  static void ignore_sigpipe();

protected:
  playerbuf pbuf_;
  playerstream_base(int input_fd, int output_fd): pbuf_(input_fd, output_fd) {}
};

class iplayerstream : public virtual playerstream_base, public std::istream
{
public:
  iplayerstream(int input_fd)
    : playerstream_base(input_fd, -1), std::ios(&pbuf_), std::istream(&pbuf_) {}
};

class oplayerstream : public virtual playerstream_base, public std::ostream
{
public:
  oplayerstream(int output_fd)
    : playerstream_base(-1, output_fd), std::ios(&pbuf_), std::ostream(&pbuf_) {}
};

class playerstream: public virtual playerstream_base,
                    public std::istream,
                    public std::ostream
{
public:
  playerstream(int input_fd, int output_fd)
    : playerstream_base(input_fd, output_fd),
      std::ios(&pbuf_),
      std::istream(&pbuf_),
      std::ostream (&pbuf_) {}
};

class playerbuf_error : public std::system_error
{
public:
  playerbuf_error(std::string msg, std::error_code err_code);
  const char* what() const noexcept override;
  std::error_code get_error_code() const;

private:
  const std::string what_;
  const std::error_code err_code_;
};

#include "playerstream_inlines.h"

#endif /* PLAYERSTREAM_H */
