#ifndef PLAYERSTREAM_INLINES_H
#define PLAYERSTREAM_INLINES_H

#include <cstring>

void playerbuf::on_error_call(error_fun_t error_fun)
{
  on_error_ = error_fun;
}

void playerbuf::on_error_throw()
{
  on_error_ = throw_last_error;
}

void playerbuf::on_error_no_op()
{
  on_error_ = error_fun_t();
}

int playerbuf::get_last_error() const
{
  return last_error_;
}

void playerstream_base::set_timeout_ms(int timeout_ms)
{
  pbuf_.set_timeout_ms(timeout_ms);
}

void playerstream_base::on_error_call(playerbuf::error_fun_t error_fun)
{
  pbuf_.on_error_call(error_fun);
}

void playerstream_base::on_error_throw()
{
  pbuf_.on_error_throw();
}

void playerstream_base::on_error_no_op()
{
  pbuf_.on_error_no_op();
}

int playerstream_base::get_last_error() const
{
  return pbuf_.get_last_error();
}

inline std::string playerstream_base::get_last_strerror() const
{
  return pbuf_.get_last_strerror();
}

#endif /* PLAYERSTREAM_INLINES_H */
