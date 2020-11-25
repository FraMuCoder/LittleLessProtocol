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


using ::testing::StrictMock;
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
