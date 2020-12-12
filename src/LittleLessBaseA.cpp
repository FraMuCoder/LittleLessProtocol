/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include "LittleLessBaseA.h"
#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <string.h>
#endif

static const char CMD_VERSION[3] _LLP_FLASHMEM_ = "ver";
static const char CMD_ECHO[3]    _LLP_FLASHMEM_ = "ech";
static const char CMD_DEBUG[3]   _LLP_FLASHMEM_ = "dbg";

const char * const LittleLessBaseA::S_CMDS[3] _LLP_FLASHMEM_ = {
  CMD_VERSION, CMD_ECHO, CMD_DEBUG
};

static uint8_t combineVersions(uint8_t v1, uint8_t v2) {
  if ((v1 & 0xF0) > (v2 & 0xF0)) {
    v1 = (v1 & 0x0F) | (v2 & 0xF0); // max version to the min
  }
  if ((v1 & 0x0F) < (v2 & 0x0F)) {
    v1 = (v1 & 0xF0) | (v2 & 0x0F); // min version to the max
  }
  return v1;
}

static inline bool isVersionValid(uint8_t v) {
  return (v >> 4) >= (v & 0x0F);
}

LittleLessBaseA::LittleLessBaseA(Stream &stream, uint8_t version)
  : LittleLessProtocolA(stream)
  , m_connected(false)
  , m_conState(conState::noConnection)
  , m_protoVersion(S_PROTO_VERSION)
  , m_ownVersion(version)
  , m_otherVersion(0xF0)
  , m_combinedVersion(version)
  , m_reqVerResp(false)
  , m_reqVerRequ(false)
{}

  
bool LittleLessBaseA::canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Version: return canHandleVersion(msgType, rx);
    default:           return false;
  }
}

void LittleLessBaseA::handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Version: handleVersionData(msgType, rx); break;
  }
}

void LittleLessBaseA::handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &, llp_result result) {
    switch (cmdId) {
      case cmd::Version: handleVersionFinish(msgType, result); break;
    }
}

uint8_t LittleLessBaseA::getCmdId(const char cmd[3]) {
  for (uint8_t i = 0; i < sizeof(S_CMDS)/sizeof(S_CMDS[0]); ++i) {
  #ifdef __AVR__
    char *str = pgm_read_ptr(S_CMDS + i);
    if (0 == memcmp_P(cmd, str, 3)) {
      return i;
    }
  #else
    if (0 == memcmp(cmd, S_CMDS[i], 3)) {
      return i;
    }
  #endif
  }
  return 0xFF;
}

bool LittleLessBaseA::getCmdStr(uint8_t cmdId, char cmd[3]) {
  if (cmdId < sizeof(S_CMDS)/sizeof(S_CMDS[0])) {
  #ifdef __AVR__
    char *str = pgm_read_ptr(S_CMDS + cmdId);
    memcpy_P(cmd, str, 3);
  #else
    memcpy(cmd, S_CMDS[cmdId], 3);
  #endif
    return true;
  } else {
    return false;
  }
}

void LittleLessBaseA::loop() {
  LittleLessProtocolA::loop();
  if (m_reqVerResp && canSend()) {
    sendVersion(llp_MsgType::response);
    m_reqVerResp = false;
  } else if (m_reqVerRequ && canSend()) {
    sendVersion(llp_MsgType::request);
    m_reqVerRequ = false;
  }
}

bool LittleLessBaseA::canHandleVersion(llp_MsgType msgType, llp_RxStruct &rx) {
  if (rx.msgTotalSize >= 3) {
    rx.buf = &m_rxBuf;
    rx.bufTotalSize = sizeof(m_rxBuf);
    m_conState = conState::waitProtoVersion;
    return true;
  } else {
    return false;
  }
}

void LittleLessBaseA::handleVersionData(llp_MsgType msgType, const llp_RxStruct &rx) {
  uint8_t pos = rx.bufOffset;
  switch (m_conState) {
    case conState::waitProtoVersion:
      m_protoVersion = combineVersions(S_PROTO_VERSION, m_rxBuf);
      if (isVersionValid(m_protoVersion)) m_conState = conState::waitAppVersion;
      else                                m_conState = conState::versionError;
      break;
    case conState::waitAppVersion:
      m_otherVersion = m_rxBuf;
      m_conState = conState::waitCombinedAppVersion;
      break;
    case conState::waitCombinedAppVersion:
      m_otherVersion = combineVersions(m_otherVersion, m_rxBuf);
      m_combinedVersion = combineVersions(m_ownVersion, m_otherVersion);
      if (isVersionValid(m_combinedVersion)) m_conState = conState::waitLen;
      else                                   m_conState = conState::versionError;
      break;
    case conState::waitLen:
      m_AppStrLen = m_rxBuf;
      m_conState = conState::waitAppName;
      {
        uint8_t len;
        char *name;
        getAppName(len, &name);
        if (len > 0xf) len = 0xf;
        if (len != (m_AppStrLen >> 4)) m_conState = conState::versionError;
      }
      break;
    case conState::waitAppName:
      {
        uint8_t len;
        char *name;
        getAppName(len, &name);
        pos -= 4;
      #ifdef __AVR__
        if (m_rxBuf != pgm_read_byte(name + pos)) {
          m_conState = conState::versionError;
          break;
        }
      #else
        if (m_rxBuf != name[pos]) {
          m_conState = conState::versionError;
          break;
        }
      #endif
        if ((pos+1) >= len) {
          if ((m_AppStrLen & 0x0F) == 0) m_conState = conState::waitVersionDone;
          else                           m_conState = conState::waitAppExtra;
        }
      }
      break;
    case conState::waitAppExtra:
      pos -= 4 + (m_AppStrLen >> 4);
      if ((pos+1) >= (m_AppStrLen & 0x0F)) {
        m_conState = conState::waitVersionDone;
      }
      break;
    default:
      m_conState = conState::versionError;
      break;
  }
}

void LittleLessBaseA::handleVersionFinish(llp_MsgType msgType, llp_result result) {
  bool oldCon = m_connected;
  if ((m_conState == conState::waitVersionDone) && (llp_result::ok == result)) {
    m_connected = true;
    m_conState = conState::connected;
    if (msgType == llp_MsgType::request) {
      m_reqVerResp = true;
    }
  } else {
    m_connected = false;
    m_protoVersion = S_PROTO_VERSION;
  }
  if (oldCon != m_connected) {
    handleConStateChanged(m_connected);
  }
}

void LittleLessBaseA::sendVersion(llp_MsgType msgType) {
  char *name;
  uint8_t nameLen;
  getAppName(nameLen, &name);
  if (nameLen > 0xf) nameLen = 0xf;
  char *extra;
  uint8_t extraLen;
  getAppExtra(extraLen, &extra);
  if (extraLen > 0xf) extraLen = 0xf;
  uint8_t len = 4 + nameLen + extraLen;
  startFrame(msgType, uint8_t(cmd::Version), len);
  sendByte(S_PROTO_VERSION);
  sendByte(m_ownVersion);
  sendByte(m_combinedVersion);
  sendByte((nameLen << 4) | extraLen);
  sendStr_P(nameLen, name);
  sendStr_P(extraLen, extra);
  endFrame();
}
