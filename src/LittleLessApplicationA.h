/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSAPPLICATIONA_H_
#define _LITTLELESSAPPLICATIONA_H_

#include "LittleLessProtocolA.h"

class LittleLessApplicationA : public LittleLessProtocolA {
public:
  enum class cmd {
    Version,
    Echo,
    Debug,
    Res3,
    Res4,
    Res5,
    Res6,
    Res7,
    FirstUser
  };
  
  LittleLessApplicationA(Stream &stream, uint8_t version);

  // Rx part
  
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsg(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &rx);
  virtual void handleByte(llp_MsgType msgType, uint8_t cmdId, uint8_t pos, uint8_t data);
  virtual void handleBytesFinish(llp_MsgType msgType, uint8_t cmdId, uint8_t chkSum, bool chkSumOK);

  // general part

  virtual uint8_t getCmdId(const char cmd[3]);
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]);
  
  virtual void loop();

  virtual void getAppName(uint8_t &len, char **name) = 0;
  virtual void getAppExtra(uint8_t &len, char **extra) = 0;
  
  bool isConnected() { return m_connected; }

private:
  enum class appState {
    noConnection,
    waitProtoVersion,
    waitAppVersion,
    waitCombinedAppVersion,
    waitLen,
    waitAppName,
    waitAppExtra,
    waitVersionDone,
    versionError,
    connected
  };

  bool          m_connected;
  appState      m_appState;
  uint8_t       m_protoVersion;
  const uint8_t m_ownVersion;       ///< upper nibble max. supported version, lower nibbler min. supported version
  uint8_t       m_otherVersion;
  uint8_t       m_combinedVersion;
  uint8_t       m_AppStrLen;
  bool          m_reqVerResp;

  static const uint8_t S_PROTO_VERSION = 0xF0;  // Debug version
  static const char * const S_CMDS[3] PROGMEM;

  bool canHandleVersion(llp_MsgType msgType, llp_RxStruct &rx);
  void handleVersionByte(llp_MsgType msgType, uint8_t pos, uint8_t data);
  void handleVersionBytesFinish(llp_MsgType msgType, uint8_t chkSum, bool chkSumOK);
  void sendVersion(llp_MsgType msgType);
};

#endif // _LITTLELESSAPPLICATIONA_H_
