/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSPROTOCOLTYPES_H_
#define _LITTLELESSPROTOCOLTYPES_H_

#include <stdint.h>

enum class llp_MsgType {
  request   = 0,
  response  = 1,
  error     = 2,
  update    = 3,
  // 4 .. 7 for future use
  none      = 0xF
};

struct llp_RxStruct {
  bool    handleSingleBytes;///< if true handleByte() and handleByteFinish() will be called for ever received byte else handleMsg() will be called once.
  uint8_t *buf;             ///< Pointer to rx buffer, set by application
  uint8_t bufTotalSize;     ///< Needed bytes in rx buffer, set by protocol
  uint8_t bufSize;          ///< Filled bytes in rxBuffer, set by protocol
};


class ALittleLessProtocol {
public:
  virtual ~ALittleLessProtocol() {}
  
  // Tx part
  
  virtual bool canSend() = 0;
  virtual void startFrame(llp_MsgType msgType, uint8_t cmdId, uint8_t len) = 0;
  virtual void sendByte(uint8_t b) = 0;
  virtual void sendChar(char c) = 0;
  virtual void sendData(uint8_t len, const uint8_t *data) = 0;
  virtual void sendStr(uint8_t len, const char *str) = 0;
  virtual void endFrame() = 0;
  virtual void abortFrame() = 0;

  // Rx part
  
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) = 0;
  virtual void handleMsg(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &rx) = 0;
  virtual void handleByte(llp_MsgType msgType, uint8_t cmdId, uint8_t pos, uint8_t data) = 0;
  virtual void handleBytesFinish(llp_MsgType msgType, uint8_t cmdId, uint8_t chkSum, bool chkSumOK) = 0;
  
  // general part

  virtual uint8_t getCmdId(const char cmd[3]) = 0;
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]) = 0;
  
  virtual void loop() = 0;
};

#endif // _LITTLELESSPROTOCOLTYPES_H_
