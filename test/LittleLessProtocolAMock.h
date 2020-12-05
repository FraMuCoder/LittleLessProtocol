/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSPROTOCOLAMOCK_H_
#define _LITTLELESSPROTOCOLAMOCK_H_

#include <LittleLessProtocol.h>
#include "gmock/gmock.h"


class LittleLessProtocolAMock : public LittleLessProtocolA {
public:
LittleLessProtocolAMock(Stream &stream)
    : LittleLessProtocolA(stream)
{}

  MOCK_METHOD3(canHandleMsg, bool(llp_MsgType , uint8_t, llp_RxStruct &));
  MOCK_METHOD3(handleMsgData, void(llp_MsgType , uint8_t, llp_RxStruct &));
  MOCK_METHOD3(handleMsgFinish, void(llp_MsgType , uint8_t, bool));
  MOCK_METHOD1(getCmdId, uint8_t(const char cmd[3]));
  MOCK_METHOD2(getCmdStr, bool(uint8_t cmdId, char cmd[3]));
};

#endif // _LITTLELESSPROTOCOLAMOCK_H_
