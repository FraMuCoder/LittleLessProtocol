/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include "LittleLessApplicationA.h"
#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <string.h>
#endif

static const char CMD_VERSION[3] _LLP_FLASHMEM_ = "ver";
static const char CMD_ECHO[3]    _LLP_FLASHMEM_ = "ech";
static const char CMD_DEBUG[3]   _LLP_FLASHMEM_ = "dbg";

const char * const LittleLessApplicationA::S_CMDS[3] _LLP_FLASHMEM_ = {
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

LittleLessApplicationA::LittleLessApplicationA(Stream &stream, uint8_t version)
  : LittleLessProtocolA(stream)
  , m_connected(false)
  , m_appState(appState::noConnection)
  , m_protoVersion(S_PROTO_VERSION)
  , m_ownVersion(version)
  , m_otherVersion(0xF0)
  , m_combinedVersion(version)
  , m_reqVerResp(false)
{}

  
bool LittleLessApplicationA::canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch ((cmd)cmdId) {
    case cmd::Version: return canHandleVersion(msgType, rx);
    default:           return false;
  }
}

void LittleLessApplicationA::handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch ((cmd)cmdId) {
    case cmd::Version: handleVersionData(msgType, rx); break;
  }
}

void LittleLessApplicationA::handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, uint8_t chkSum, bool msgOK) {
    switch ((cmd)cmdId) {
      case cmd::Version: handleVersionFinish(msgType, chkSum, msgOK); break;
    }
}

uint8_t LittleLessApplicationA::getCmdId(const char cmd[3]) {
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

bool LittleLessApplicationA::getCmdStr(uint8_t cmdId, char cmd[3]) {
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

void LittleLessApplicationA::loop() {
  LittleLessProtocolA::loop();
  if (m_reqVerResp && canSend()) {
    sendVersion(llp_MsgType::response);
    m_reqVerResp = false;
  }
}

bool LittleLessApplicationA::canHandleVersion(llp_MsgType msgType, llp_RxStruct &rx) {
  if (rx.msgTotalSize >= 3) {
    rx.buf = &m_rxBuf;
    rx.bufTotalSize = sizeof(m_rxBuf);
    m_appState = appState::waitProtoVersion;
    return true;
  } else {
    return false;
  }
}

void LittleLessApplicationA::handleVersionData(llp_MsgType msgType, const llp_RxStruct &rx) {
  uint8_t pos = rx.bufOffset;
  switch (m_appState) {
    case appState::waitProtoVersion:
      m_protoVersion = combineVersions(S_PROTO_VERSION, m_rxBuf);
      if (isVersionValid(m_protoVersion)) m_appState = appState::waitAppVersion;
      else                                m_appState = appState::versionError;
      break;
    case appState::waitAppVersion:
      m_otherVersion = m_rxBuf;
      m_appState = appState::waitCombinedAppVersion;
      break;
    case appState::waitCombinedAppVersion:
      m_otherVersion = combineVersions(m_otherVersion, m_rxBuf);
      m_combinedVersion = combineVersions(m_ownVersion, m_otherVersion);
      if (isVersionValid(m_combinedVersion)) m_appState = appState::waitLen;
      else                                   m_appState = appState::versionError;
      break;
    case appState::waitLen:
      m_AppStrLen = m_rxBuf;
      m_appState = appState::waitAppName;
      {
        uint8_t len;
        char *name;
        getAppName(len, &name);
        if (len > 0xf) len = 0xf;
        if (len != (m_AppStrLen & 0x0F)) m_appState = appState::versionError;
      }
      break;
    case appState::waitAppName:
      {
        uint8_t len;
        char *name;
        getAppName(len, &name);
        pos -= 4;
      #ifdef __AVR__
        if (m_rxBuf != pgm_read_byte(name + pos)) {
          m_appState = appState::versionError;
          break;
        }
      #else
        if (m_rxBuf != name[pos]) {
          m_appState = appState::versionError;
          break;
        }
      #endif
        if ((pos+1) >= len) {
          if ((m_AppStrLen >> 4) == 0) m_appState = appState::waitVersionDone;
          else                         m_appState = appState::waitAppExtra;
        }
      }
      break;
    case appState::waitAppExtra:
      pos -= 4 + (m_AppStrLen & 0x0F);
      if ((pos+1) >= (m_AppStrLen >> 4)) {
        m_appState = appState::waitVersionDone;
      }
      break;
    default:
      m_appState = appState::versionError;
      break;
  }
}

void LittleLessApplicationA::handleVersionFinish(llp_MsgType msgType, uint8_t chkSum, bool msgOK) {
  if ((m_appState == appState::waitVersionDone) && msgOK) {
    m_connected = true;
    m_appState = appState::connected;
    if (msgType == llp_MsgType::request) {
      m_reqVerResp = true;
    }
  } else {
    m_connected = false;
    m_protoVersion = S_PROTO_VERSION;
  }
}

void LittleLessApplicationA::sendVersion(llp_MsgType msgType) {
  char *name;
  uint8_t nameLen;
  getAppName(nameLen, &name);
  if (nameLen > 0xf) nameLen = 0xf;
  char *extra;
  uint8_t extraLen;
  getAppExtra(extraLen, &extra);
  if (extraLen > 0xf) extraLen = 0xf;
  uint8_t len = 4 + nameLen + extraLen;
  startFrame(llp_MsgType::response, uint8_t(cmd::Version), len);
  sendByte(S_PROTO_VERSION);
  sendByte(m_ownVersion);
  sendByte(m_combinedVersion);
  sendByte((extraLen << 4) | nameLen);
  sendStr_P(nameLen, name);
  sendStr_P(extraLen, extra);
  endFrame();
}
