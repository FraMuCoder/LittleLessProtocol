/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSPROTOCOLTYPES_H_
#define _LITTLELESSPROTOCOLTYPES_H_

#include <stdint.h>

#ifdef __AVR__
  #define _LLP_FLASHMEM_ PROGMEM
#endif

#ifndef _LLP_FLASHMEM_
  #define _LLP_FLASHMEM_
#endif

enum class llp_MsgType : uint8_t {
  request   = 0,
  response  = 1,
  error     = 2,
  update    = 3,
  // 4 .. 7 for future use
  none      = 0xF
};

struct llp_RxStruct {
  /// Needed of message, set by protocol
  uint8_t msgTotalSize;

  /// Pointer to rx buffer, set by application
  uint8_t *buf;

  /// Size of rx buffer, set by application
  /// If this is less than msgTotalSize handleMsgData() will be called more than one times.
  /// bufTotalSize must be >= 1.
  uint8_t bufTotalSize;
  
  /// Offset of buf start, set by protocol.
  /// This will be increased by bufTotalSize every time handleMsgData() must be called. 
  uint8_t bufOffset;

  /// Filled bytes in rxBuffer, set by protocol
  uint8_t bufSize;
};

enum class llp_result : uint8_t {
  ok,
  error,
  frameError,
  unknownMsgType,
  unknownCommand,
  applicationAbort,
  busy
};

class ALittleLessProtocol {
public:
  virtual ~ALittleLessProtocol() {}

  // Rx part
  
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) = 0;
  virtual void handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) = 0;
  virtual void handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, llp_result result) = 0;
  
  // general part

  virtual uint8_t getCmdId(const char cmd[3]) = 0;
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]) = 0;
  
  virtual void loop() = 0;
};

#endif // _LITTLELESSPROTOCOLTYPES_H_
