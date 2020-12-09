/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include <LittleLessProtocol.h>
#include "StreamMock.h"
#include "LittleLessBaseAMock.h"
#include <cstring>

#include "gtest/gtest.h"

using ::testing::_;
using ::testing::Return;

// action for getAppName(...) and getAppExtra(...)
ACTION_P(setName, name) {
  arg0 = std::strlen(name);
  *arg1 = name;
}

////////////////////////////////////////////////////////////////////////////////
// Valid rx frames
////////////////////////////////////////////////////////////////////////////////

TEST(LittleLessBaseA, rxValidVersionRequest) {
  StreamMock sm;
  LittleLessBaseAMock testObj(sm, 0xF0);

  std::string frame(">ver:0B:10101043\"Test\"\"App\":ff\r\n");
  sm.putInput(frame);

  EXPECT_CALL(testObj, getAppName(_, _))
      .WillRepeatedly(setName("Test"));
  EXPECT_CALL(testObj, getAppExtra(_, _))
      .WillRepeatedly(setName("Mock"));
  EXPECT_CALL(testObj, handleConStateChanged(true))
      .Times(1);;

  for (int i = 0; i < frame.size(); ++i) {
    testObj.loop();
  }

  EXPECT_EQ(1, testObj.getEfectiveRxVersion());
  EXPECT_EQ(1, testObj.getEfectiveTxVersion());

  EXPECT_EQ("<ver:0C:F0F01044\"Test\"\"Mock\":FF\r\n", sm.getOutputStr()); 

  EXPECT_TRUE(testObj.canSend());
}
