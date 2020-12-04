/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include <LittleLessProtocol.h>
#include "StreamMock.h"
#include "LittleLessProtocolAMock.h"

#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

// test parameter char[3]
MATCHER_P(IsCMD, cmd, "") { return (arg[0] == cmd[0]) && (arg[1] == cmd[1]) && (arg[2] == cmd[2]); }

// test parameter llp_RxStruct
MATCHER_P(IsMsgTotalSize, size, "") { return arg.msgTotalSize == size; }
MATCHER_P5(IsRxStruct, msgTotalSize, buf, bufTotalSize, bufOffset, bufSize, "") {
  return (arg.msgTotalSize == msgTotalSize)
          && (arg.buf == buf)
          && (arg.bufTotalSize == bufTotalSize)
          && (arg.bufOffset == bufOffset)
          && (arg.bufSize == bufSize);
}

// action for canHandleMsg(...)
ACTION_P2(setBuf, buf, size) {
  arg2.buf = buf;
  arg2.bufTotalSize = size;
  return true;
}

// action for getCmdStr(...)
ACTION_P(setCmdStr, str) {
  arg1[0] = str[0];
  arg1[1] = str[1];
  arg1[2] = str[2];
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// Valid rx frames
////////////////////////////////////////////////////////////////////////////////

TEST(LittleLessProtocolA, rxValidRequestFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame(">abc:01:00:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[1] = {0xFF};

  EXPECT_CALL(testObj, getCmdId(IsCMD("abc")))
      .WillOnce(Return(1));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::request, 1, IsMsgTotalSize(1)))
      .WillOnce(setBuf(buf, 1));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::request, 1, IsRxStruct(1, buf, 1, 0, 1)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::request, 1, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0x00);
}

TEST(LittleLessProtocolA, rxValidResponseFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("<DEF:02:0102:FF\r\n");
  sm.putInput(frame);

  uint8_t buf[2] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("DEF")))
      .WillOnce(Return(5));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::response, 5, IsMsgTotalSize(2)))
      .WillOnce(setBuf(buf, 2));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::response, 5, IsRxStruct(2, buf, 2, 0, 2)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::response, 5, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0x01);
  EXPECT_EQ(buf[1], 0x02);
}

TEST(LittleLessProtocolA, rxValidErrorFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("!xYz:03:abcdef:FF\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("xYz")))
      .WillOnce(Return(7));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::error, 7, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::error, 7, IsRxStruct(3, buf, 5, 0, 3)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::error, 7, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0xab);
  EXPECT_EQ(buf[1], 0xcd);
  EXPECT_EQ(buf[2], 0xef);
}

TEST(LittleLessProtocolA, rxValidEmptyErrorFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("!xYz:00::FF\r\n");
  sm.putInput(frame);

  EXPECT_CALL(testObj, getCmdId(IsCMD("xYz")))
      .WillOnce(Return(7));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::error, 7, IsMsgTotalSize(0)))
      .WillOnce(setBuf(NULL, 0));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::error, 7, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxValidUpdateFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:CAFFEE:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 254, IsRxStruct(3, buf, 5, 0, 3)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0xca);
  EXPECT_EQ(buf[1], 0xff);
  EXPECT_EQ(buf[2], 0xee);
}

TEST(LittleLessProtocolA, rxValidASCIIFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:06:\"CAFFEE\":ff\r\n");
  sm.putInput(frame);

  uint8_t buf[10] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(128));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 128, IsMsgTotalSize(6)))
      .WillOnce(setBuf(buf, sizeof(buf)));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 128, IsRxStruct(6, buf, sizeof(buf), 0, 6)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 128, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 'C');
  EXPECT_EQ(buf[1], 'A');
  EXPECT_EQ(buf[2], 'F');
  EXPECT_EQ(buf[3], 'F');
  EXPECT_EQ(buf[4], 'E');
  EXPECT_EQ(buf[5], 'E');
}

TEST(LittleLessProtocolA, rxValidMixedFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:05:CA\"FFEE\":ff\r\n");
  sm.putInput(frame);

  uint8_t buf[10] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(128));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 128, IsMsgTotalSize(5)))
      .WillOnce(setBuf(buf, sizeof(buf)));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 128, IsRxStruct(5, buf, sizeof(buf), 0, 5)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 128, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0xca);
  EXPECT_EQ(buf[1], 'F');
  EXPECT_EQ(buf[2], 'F');
  EXPECT_EQ(buf[3], 'E');
  EXPECT_EQ(buf[4], 'E');
}

TEST(LittleLessProtocolA, rxValidFrameWithEmptyASCII) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:CA\"\"FFEE\"\":ff\r\n");
  sm.putInput(frame);

  uint8_t buf[10] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(128));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 128, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, sizeof(buf)));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 128, IsRxStruct(3, buf, sizeof(buf), 0, 3)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 128, 0xFF, true))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(buf[0], 0xca);
  EXPECT_EQ(buf[1], 0xff);
  EXPECT_EQ(buf[2], 0xee);
}

////////////////////////////////////////////////////////////////////////////////
// Invalid rx frames
////////////////////////////////////////////////////////////////////////////////

TEST(LittleLessProtocolA, rxInvalidFrameToLongChecksum) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:CAFFEE:fff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 254, IsRxStruct(3, buf, 5, 0, 3)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameChecksumBadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:CAFFEE:f_\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgData(llp_MsgType::update, 254, IsRxStruct(3, buf, 5, 0, 3)))
      .Times(1);
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameColon3BadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:CAFFEE_ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameDataToLong) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:02:CAFFEE:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(2)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameDataToShort) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:04:CAFFEE:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(4)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameDataBadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:03:qwertz:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameColon2BadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:030CAFFEE:ff\r\n");
  sm.putInput(frame);

  uint8_t buf[5] = {0};

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));
  EXPECT_CALL(testObj, canHandleMsg(llp_MsgType::update, 254, IsMsgTotalSize(3)))
      .WillOnce(setBuf(buf, 5));
  EXPECT_CALL(testObj, handleMsgFinish(llp_MsgType::update, 254, _, false))
      .Times(1);

  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameLenBadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#upd:nn:CAFFEE:ff\r\n");
  sm.putInput(frame);

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));

  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameColon1BadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("#updA03:CAFFEE:ff\r\n");
  sm.putInput(frame);

  EXPECT_CALL(testObj, getCmdId(IsCMD("upd")))
      .WillOnce(Return(254));

  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

TEST(LittleLessProtocolA, rxInvalidFrameTypeBadChar) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  std::string frame("_upd:03:CAFFEE:ff\r\n");
  sm.putInput(frame);

  EXPECT_CALL(testObj, getCmdId(_)).Times(0);
  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }
}

////////////////////////////////////////////////////////////////////////////////
// Valid tx frames
////////////////////////////////////////////////////////////////////////////////

TEST(LittleLessProtocolA, txValidRequestFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  EXPECT_CALL(testObj, getCmdId(_)).Times(0);
  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  EXPECT_TRUE(testObj.canSend());

  EXPECT_CALL(testObj, getCmdStr(1, _))
      .WillOnce(setCmdStr("abc"));;

  testObj.startFrame(llp_MsgType::request, 1, 3);

  EXPECT_FALSE(testObj.canSend());

  EXPECT_EQ(">abc:03:", sm.getOutputStr());

  testObj.sendByte(0);
  testObj.sendByte(1);
  testObj.sendByte(2);

  EXPECT_EQ("000102", sm.getOutputStr()); 

  testObj.endFrame();

  EXPECT_EQ(":FF\r\n", sm.getOutputStr()); 

  EXPECT_TRUE(testObj.canSend());
}

TEST(LittleLessProtocolA, txValidResponseFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  EXPECT_CALL(testObj, getCmdId(_)).Times(0);
  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  EXPECT_TRUE(testObj.canSend());

  EXPECT_CALL(testObj, getCmdStr(4, _))
      .WillOnce(setCmdStr("XyZ"));;

  testObj.startFrame(llp_MsgType::response, 4, 5);

  EXPECT_FALSE(testObj.canSend());

  testObj.sendByte(0xAA);
  testObj.sendChar('a');
  testObj.sendChar('b');
  testObj.sendChar('c');
  testObj.sendByte(0xBB);

  testObj.endFrame();

  EXPECT_EQ("<XyZ:05:AA\"abc\"BB:FF\r\n", sm.getOutputStr()); 

  EXPECT_TRUE(testObj.canSend());
}

TEST(LittleLessProtocolA, txValidEmptyErrorFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  EXPECT_CALL(testObj, getCmdId(_)).Times(0);
  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  EXPECT_TRUE(testObj.canSend());

  EXPECT_CALL(testObj, getCmdStr(4, _))
      .WillOnce(setCmdStr("XyZ"));;

  testObj.startFrame(llp_MsgType::error, 4, 0);

  EXPECT_FALSE(testObj.canSend());

  testObj.endFrame();

  EXPECT_EQ("!XyZ:00::FF\r\n", sm.getOutputStr()); 

  EXPECT_TRUE(testObj.canSend());
}

TEST(LittleLessProtocolA, txValidUpdateFrame) {
  StreamMock sm;
  LittleLessProtocolAMock testObj(sm);

  EXPECT_CALL(testObj, getCmdId(_)).Times(0);
  EXPECT_CALL(testObj, canHandleMsg(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgData(_, _, _)).Times(0);
  EXPECT_CALL(testObj, handleMsgFinish(_, _, _, _)).Times(0);
  EXPECT_CALL(testObj, getCmdStr(_, _)).Times(0);

  EXPECT_TRUE(testObj.canSend());

  EXPECT_CALL(testObj, getCmdStr(4, _))
      .WillOnce(setCmdStr("ECH"));;

  testObj.startFrame(llp_MsgType::update, 4, 5);

  EXPECT_FALSE(testObj.canSend());

  testObj.sendStr(5, "Hello World");

  testObj.endFrame();

  EXPECT_EQ("#ECH:05:\"Hello\":FF\r\n", sm.getOutputStr()); 

  EXPECT_TRUE(testObj.canSend());
}
