#include <string>

#include <gtest/gtest.h>

#include "common.h"
#include "err.h"
#include "playerstream.h"

namespace {

class PlayerStreamTestBase : public ::testing::Test {

protected:
  PlayerStreamTestBase()
  {
    // Create an unnamed pipe.
    // Its read end will be used as playerstream input.
    // Its write end will be used in tests for sending crafted messages.
    SYSCALL_WITH_CHECK(pipe(pipes));
    playerstream_base::ignore_sigpipe();
  }

  virtual ~PlayerStreamTestBase()
  {
    CloseReadPipe();
    CloseWritePipe();
  }

  virtual int GetReadPipe()
  {
    return pipes[PIPE_READ_END];
  }

  virtual int GetWritePipe()
  {
    return pipes[PIPE_WRITE_END];
  }

  virtual void CloseWritePipe()
  {
    if (writePipeClosed)
      return;

    writePipeClosed = true;
    SYSCALL_WITH_CHECK(close(pipes[PIPE_WRITE_END]));
  }

  virtual void CloseReadPipe()
  {
    if (readPipeClosed)
      return;

    readPipeClosed = true;
    SYSCALL_WITH_CHECK(close(pipes[PIPE_READ_END]));
  }

  static constexpr int TIMEOUT_MS_SHORT = 5;
  // Unnamed pipe with 2 ends: 1 for reading, 1 for writing.
  int pipes[2];
  bool writePipeClosed = false;
  bool readPipeClosed = false;
};

class InputPlayerStreamTest : public PlayerStreamTestBase
{
protected:
  InputPlayerStreamTest()
    : testedStream(GetReadPipe())
  {
  }

  iplayerstream testedStream;
};

TEST_F(InputPlayerStreamTest, TestThrowingExceptionOnReadWithInvalidFileDescriptor)
{
  iplayerstream testedBadStream{GetReadPipe() + 2};
  testedBadStream.on_error_throw();
  std::string readMsg;
  try
  {
    testedBadStream >> readMsg;
    FAIL() << "The previous line should have thrown an exception.";
  }
  catch (const playerbuf_error& e)
  {
    EXPECT_EQ(EBADF, e.get_error_code().value());
  }
}

TEST_F(InputPlayerStreamTest, TestReadingStringWithNewline)
{
  const std::string expectedReadMsg = "Hello, world!";
  const std::string sentMsg = expectedReadMsg + '\n';
  int returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  std::string readMsg;
  std::getline(testedStream, readMsg);
  EXPECT_EQ(expectedReadMsg, readMsg);
  EXPECT_EQ(0, testedStream.get_last_error());
}

TEST_F(InputPlayerStreamTest, TestReadingStringAfterClosingPipe)
{
  const std::string sentMsg = "Hello, world!";
  int returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  CloseWritePipe();

  std::string readMsg;
  std::getline(testedStream, readMsg);
  EXPECT_EQ(sentMsg, readMsg);
  EXPECT_TRUE(testedStream.eof());
  EXPECT_EQ(0, testedStream.get_last_error());
}

TEST_F(InputPlayerStreamTest, TestReadingTwoWords)
{
  // The trailing space is needed to mark the end of 2nd word
  const std::string sentMsg = "Hello world ";
  int returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  std::string readMsg;
  testedStream >> readMsg;
  EXPECT_EQ("Hello", readMsg);

  testedStream >> readMsg;
  EXPECT_EQ("world", readMsg);
}

TEST_F(InputPlayerStreamTest, TestReadingTwoWordsWithTwoPipeWrites)
{
  std::string sentMsg = "Hello ";
  int returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  std::string readMsg;
  testedStream >> readMsg;
  EXPECT_EQ("Hello", readMsg);

  sentMsg = "world ";
  returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  testedStream >> readMsg;
  EXPECT_EQ("world", readMsg);
}

TEST_F(InputPlayerStreamTest, TestTimeoutAfterNoDataRead)
{
  testedStream.set_timeout_ms(TIMEOUT_MS_SHORT);
  std::string readMsg;

  testedStream >> readMsg;
  EXPECT_TRUE(testedStream.eof());
  EXPECT_TRUE(testedStream.fail()); // no data read at all
  EXPECT_EQ(ETIME, testedStream.get_last_error());
}

TEST_F(InputPlayerStreamTest, TestTimeoutAfterIncompleteLineRead)
{
  const std::string sentMsg = "Hello";
  int returnValue = write(GetWritePipe(), sentMsg.c_str(), sentMsg.size());
  ASSERT_EQ(sentMsg.size(), returnValue);

  testedStream.set_timeout_ms(TIMEOUT_MS_SHORT);

  std::string readMsg;
  testedStream >> readMsg;
  EXPECT_EQ(sentMsg, readMsg);
  EXPECT_TRUE(testedStream.eof());
  EXPECT_EQ(ETIME, testedStream.get_last_error());
}

TEST_F(InputPlayerStreamTest, TestCallbackCalledAfterTimeout)
{
  testedStream.set_timeout_ms(TIMEOUT_MS_SHORT);
  int callCount = 0;
  auto callback = [this, &callCount]
    (const playerbuf& sender, int errnum)
  {
    EXPECT_EQ(testedStream.rdbuf(), &sender);
    EXPECT_EQ(ETIME, errnum);
    callCount++;
  };
  testedStream.on_error_throw();
  // Override the error behaviour
  testedStream.on_error_call(callback);

  std::string readMsg;
  std::getline(testedStream, readMsg);
  EXPECT_TRUE(testedStream.eof());
  EXPECT_EQ(ETIME, testedStream.get_last_error());
  EXPECT_EQ(1, callCount);
}

TEST_F(InputPlayerStreamTest, TestReadingMoreThanBufferSizeAtOnce)
{
  constexpr int WORD_SIZE = playerbuf::BUF_SIZE + 5;
  const std::string word(WORD_SIZE, 'a');
  int returnValue = write(GetWritePipe(), word.c_str(), word.size());
  ASSERT_EQ(word.size(), returnValue);

  CloseWritePipe();

  std::string readMsg;
  testedStream >> readMsg;
  EXPECT_EQ(word, readMsg);
  EXPECT_TRUE(testedStream.eof());
}

TEST_F(InputPlayerStreamTest, TestNoCallbackCalledOnTimeoutAfterCallbackWasCleared)
{
  auto callback = [](const playerbuf& /*sender*/, int /*errnum*/)
  {
    FAIL() << "The callback function should not have been called.";
  };
  testedStream.set_timeout_ms(TIMEOUT_MS_SHORT);
  testedStream.on_error_call(callback);
  testedStream.on_error_no_op();

  int readChar = testedStream.get();
  EXPECT_EQ(EOF, readChar);
  EXPECT_EQ(ETIME, testedStream.get_last_error());
  EXPECT_TRUE(testedStream.eof());
}

class OutputPlayerStreamTest : public PlayerStreamTestBase
{
protected:
  OutputPlayerStreamTest()
    : testedStream(new oplayerstream(GetWritePipe()))
  {
  }

  virtual void TearDown() override
  {
    // check that no data is left in pipe
    testedStream.reset();
    CloseWritePipe();
    if (!readPipeClosed)
    {
      // make sure that the next read() will not block indefinitely
      char c;
      int readCount = read(GetReadPipe(), &c, sizeof(char));
      EXPECT_EQ(0, readCount);
    }
  }

  std::string readNChars(int n)
  {
    char* buf = new char[n+1];
    int readCount = read(GetReadPipe(), buf, n);
    EXPECT_EQ(n, readCount);
    buf[readCount] = '\0';
    std::string result(buf);
    delete buf;
    return result;
  }

  std::unique_ptr<oplayerstream> testedStream;
};

TEST_F(OutputPlayerStreamTest, TestWritingHelloWorld)
{
  std::string expectedMsg = "Hello, world!";
  *testedStream << expectedMsg << std::flush;
  std::string readMsg = readNChars(expectedMsg.size());
  EXPECT_EQ(expectedMsg, readMsg);
}

TEST_F(OutputPlayerStreamTest, TestFlushAsFirstOperation)
{
  *testedStream << std::flush;
  // The stream should be empty at TearDown.
}

TEST_F(OutputPlayerStreamTest, TestDoubleFlushAfterWritingOneChar)
{
  std::string expectedMsg = "a";
  *testedStream << expectedMsg;
  *testedStream << std::flush;
  *testedStream << std::flush;
  std::string readMsg = readNChars(expectedMsg.size());
  EXPECT_EQ(expectedMsg, readMsg);
}

TEST_F(OutputPlayerStreamTest, TestWriteFlushMultipleTimes)
{
  constexpr int REPS = 10;
  for (int i = 0; i < REPS; i++)
  {
    SCOPED_TRACE("Looping with i=" + std::to_string(i));
    std::string expectedMsg = "Hello ";
    *testedStream << expectedMsg;
    *testedStream << std::flush;
    std::string readMsg = readNChars(expectedMsg.size());
  }
}

TEST_F(OutputPlayerStreamTest, TestCallbackCalledAfterWritingToClosedPipe)
{
  constexpr int EXPECTED_ERRNUM = EBADF;
  int callCount = 0;
  auto callback = [this, &callCount]
    (const playerbuf& sender, int errnum)
  {
    EXPECT_EQ(testedStream->rdbuf(), &sender);
    EXPECT_EQ(EXPECTED_ERRNUM, errnum);
    callCount++;
  };
  testedStream->on_error_call(callback);

  CloseWritePipe();
  std::string writtenMsg = "abc";
  *testedStream << writtenMsg;
  *testedStream << std::flush;
  EXPECT_TRUE(testedStream->bad());
  EXPECT_EQ(EXPECTED_ERRNUM, testedStream->get_last_error());
  EXPECT_EQ(1, callCount);
}

TEST_F(OutputPlayerStreamTest, TestThrowingExceptionOnWriteWithInvalidFileDescriptor)
{
  oplayerstream testedBadStream{GetWritePipe() + 2};
  testedBadStream.on_error_throw();
  std::string writtenMsg = "abc";
  testedBadStream << writtenMsg << std::flush;
  EXPECT_TRUE(testedBadStream.bad());
  EXPECT_EQ(EBADF, testedBadStream.get_last_error());
}

TEST_F(OutputPlayerStreamTest, TestBadBitOnWriteWithInvalidFileDescriptor)
{
  oplayerstream testedBadStream{GetWritePipe() + 2};
  std::string writtenMsg = "abc";
  testedBadStream << writtenMsg << std::flush;
  EXPECT_TRUE(testedBadStream.bad());
  EXPECT_EQ(EBADF, testedBadStream.get_last_error());
}

TEST_F(OutputPlayerStreamTest, TestWritingAndFlushingPipeWithClosedReadEnd)
{
  CloseReadPipe();
  *testedStream << "abc" << std::flush;
  EXPECT_EQ(EPIPE, testedStream->get_last_error());
}

// TODO: add tests for i/o class (playerstream)

}  // namespace


