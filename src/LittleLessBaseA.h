/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _LITTLELESSBASEA_H_
#define _LITTLELESSBASEA_H_

#include "LittleLessProtocolA.h"

class LittleLessBaseA : public LittleLessProtocolA {
public:
  struct cmd {
    enum : uint8_t  {
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
  };
  
  LittleLessBaseA(Stream &stream, uint8_t version);

  // Rx part
  
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, bool msgOK);

  // general part

  virtual uint8_t getCmdId(const char cmd[3]);
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]);
  
  virtual void loop();

  virtual void getAppName(uint8_t &len, const char **name) = 0;
  virtual void getAppExtra(uint8_t &len, const char **extra) = 0;
  
  bool isConnected() { return m_connected; }
  virtual void handleConStateChanged(bool conState) = 0;

  uint8_t getEfectiveRxVersion() { return m_otherVersion >> 4; }
  uint8_t getEfectiveTxVersion() { return m_combinedVersion >> 4; }

private:
  enum class conState {
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
  conState      m_conState;
  uint8_t       m_rxBuf;
  uint8_t       m_protoVersion;
  const uint8_t m_ownVersion;       ///< upper nibble max. supported version, lower nibbler min. supported version
  uint8_t       m_otherVersion;
  uint8_t       m_combinedVersion;
  uint8_t       m_AppStrLen;
  bool          m_reqVerResp;

  static const uint8_t S_PROTO_VERSION = 0xF0;  // Debug version
  static const char * const S_CMDS[3] _LLP_FLASHMEM_;

  bool canHandleVersion(llp_MsgType msgType, llp_RxStruct &rx);
  void handleVersionData(llp_MsgType msgType, const llp_RxStruct &rx);
  void handleVersionFinish(llp_MsgType msgType, bool msgOK);
  void sendVersion(llp_MsgType msgType);
};

#endif // _LITTLELESSBASEA_H_
