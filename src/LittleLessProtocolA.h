/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSPROTOCOLA_H_
#define _LITTLELESSPROTOCOLA_H_

#include <Stream.h>
#include "LittleLessProtocolTypes.h"

//
// example frames:
//   a request:   '>ver:0B:11111167"MyProtoServer":FF\r\n'
//   a response:  '<ver:0B:11211167"MyProtoClient":FF\r\n'
//   an error:    '!TST:01:AE:FF\r\n'
//   an update:   '#DAT:05:0102030405:FF\r\n'
//
//  '>ver:0A:111167"MyProtoServer":FF\r\n'
//    '>'                       Message type
//    '0A'                      Message length, one hex byte
//    '11111167"MyProtoServer"' Message data in hex or ASCCI
//    'FF'                      Checksum, one hex byte
//    '\r\n'                    Frame end
//

class LittleLessProtocolA : public ALittleLessProtocol {
public:
  LittleLessProtocolA(Stream &stream);

  // Tx part
  
  virtual bool canSend();
  virtual void startFrame(llp_MsgType msgType, uint8_t cmdId, uint8_t len);
  virtual void sendByte(uint8_t b);
  virtual void sendChar(char c);
  virtual void sendData(uint8_t len, const uint8_t *data);
  virtual void sendStr(uint8_t len, const char *str);
  virtual void sendStr_P(uint8_t len, const char *str);
  virtual void endFrame();
  virtual void abortFrame();

  virtual void loop();

private:
  enum class frameState {
    type,         // < > ! #
    cmd,          // 3 chars
    colon1,       // :
    len,          // 2 x hex
    colon2,       // :
    dataBin,      // n x 2 x hex
    dataASCII,    // "ascii"
    dataASCIIesc, // backslash
    colon3,       // :
    checkSum,     // 2 x hex
    done,         // \r\n
    error
  };
  
  Stream       &m_stream;
  frameState    m_readState;
  llp_MsgType   m_msgType;
  uint8_t       m_cmdId;
  char          m_cmd[3];
  uint8_t       m_cmdLen;
  llp_RxStruct  m_rxData;
  frameState    m_writeState;
  
  int readHex(char c);
  bool setBin();
  bool setASCII();
  void handleError();
  void fillBuffer(uint8_t d);
};


#endif // _LITTLELESSPROTOCOLA_H_
