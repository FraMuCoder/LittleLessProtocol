/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSBASEAMOCK_H_
#define _LITTLELESSBASEAMOCK_H_

#include <LittleLessProtocol.h>
#include "gmock/gmock.h"


class LittleLessBaseAMock : public LittleLessBaseA {
public:
LittleLessBaseAMock(Stream &stream, uint8_t version)
    : LittleLessBaseA(stream, version)
{}

  //MOCK_METHOD3(canHandleMsg, bool(llp_MsgType , uint8_t, llp_RxStruct &));
  //MOCK_METHOD3(handleMsgData, void(llp_MsgType , uint8_t, llp_RxStruct &));
  //MOCK_METHOD4(handleMsgFinish, void(llp_MsgType , uint8_t, uint8_t, bool));
  //MOCK_METHOD1(getCmdId, uint8_t(const char cmd[3]));
  //MOCK_METHOD2(getCmdStr, bool(uint8_t cmdId, char cmd[3]));

  MOCK_METHOD2(getAppName, void(uint8_t &, const char **));
  MOCK_METHOD2(getAppExtra, void(uint8_t &, const char **));
  MOCK_METHOD1(handleConStateChanged, void(bool ));
};

#endif // _LITTLELESSBASEAMOCK_H_
