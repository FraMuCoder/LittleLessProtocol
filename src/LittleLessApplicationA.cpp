/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include "LittleLessApplicationA.h"
#include <Arduino.h>

static const char CMD_VERSION[3] PROGMEM = "ver";
static const char CMD_ECHO[3]    PROGMEM = "ech";
static const char CMD_DEBUF[3]   PROGMEM = "dbg";

const char * const LittleLessApplicationA::S_CMDS[3] PROGMEM = {
  CMD_VERSION, CMD_ECHO, CMD_DEBUF
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

void LittleLessApplicationA::handleMsg(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &rx) {
}

void LittleLessApplicationA::handleByte(llp_MsgType msgType, uint8_t cmdId, uint8_t pos, uint8_t data) {
  switch ((cmd)cmdId) {
    case cmd::Version: handleVersionByte(msgType, pos, data); break;
  }
}

void LittleLessApplicationA::handleBytesFinish(llp_MsgType msgType, uint8_t cmdId, uint8_t chkSum, bool chkSumOK) {
    switch ((cmd)cmdId) {
      case cmd::Version: handleVersionBytesFinish(msgType, chkSum, chkSumOK); break;
    }
}

uint8_t LittleLessApplicationA::getCmdId(const char cmd[3]) {
  for (uint8_t i = 0; i < sizeof(S_CMDS)/sizeof(S_CMDS[0]); ++i) {
    char *str = pgm_read_ptr(S_CMDS + i);
    if (0 == memcmp_P(cmd, str, 3)) {
      return i;
    }
  }
  return 0xFF;
}

bool LittleLessApplicationA::getCmdStr(uint8_t cmdId, char cmd[3]) {
  if (cmdId < sizeof(S_CMDS)/sizeof(S_CMDS[0])) {
    char *str = pgm_read_ptr(S_CMDS + cmdId);
    memcpy_P(cmd, str, 3);
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
  if (rx.bufTotalSize >= 3) {
    rx.buf = NULL;
    rx.handleSingleBytes = true;
    m_appState = appState::waitProtoVersion;
    return true;
  } else {
    return false;
  }
}

void LittleLessApplicationA::handleVersionByte(llp_MsgType msgType, uint8_t pos, uint8_t data) {
  switch (m_appState) {
    case appState::waitProtoVersion:
      m_protoVersion = combineVersions(S_PROTO_VERSION, data);
      if (isVersionValid(m_protoVersion)) m_appState = appState::waitAppVersion;
      else                                m_appState = appState::versionError;
      break;
    case appState::waitAppVersion:
      m_otherVersion = data;
      m_appState = appState::waitCombinedAppVersion;
      break;
    case appState::waitCombinedAppVersion:
      m_otherVersion = combineVersions(m_otherVersion, data);
      m_combinedVersion = combineVersions(m_ownVersion, m_otherVersion);
      if (isVersionValid(m_combinedVersion)) m_appState = appState::waitLen;
      else                                   m_appState = appState::versionError;
      break;
    case appState::waitLen:
      m_AppStrLen = data;
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
        if (data != pgm_read_byte(name + pos)) {
          m_appState = appState::versionError;
          break;
        }
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

void LittleLessApplicationA::handleVersionBytesFinish(llp_MsgType msgType, uint8_t chkSum, bool chkSumOK) {
  if ((m_appState == appState::waitVersionDone) && chkSumOK) {
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
