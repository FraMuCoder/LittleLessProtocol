/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include "LittleLessProtocolA.h"

static uint8_t hex2int(char hex) {
  if ((hex >= '0') && (hex <= '9')) {
    return hex - '0';
  } else if ((hex >= 'a') && (hex <= 'f')) {
    return hex - 'a' + 10;
  } else if ((hex >= 'A') && (hex <= 'F')) {
    return hex - 'A' + 10;
  } else {
    return 0;
  }
}

static char int2hex(uint8_t i) {
  if (i < 10) {
    return '0' + i;
  } else {
    return 'A' + i - 10;
  }
}

LittleLessProtocolA::LittleLessProtocolA(Stream &stream)
  : m_stream(stream)
  , m_readState(frameState::type)
  , m_msgType(llp_MsgType::none)
  , m_cmdId(0)
  , m_cmdLen(0)
  , m_writeState(frameState::type)
{}

bool LittleLessProtocolA::canSend() {
  return m_writeState == frameState::type;
}

void LittleLessProtocolA::startFrame(llp_MsgType msgType, uint8_t cmdId, uint8_t len) {
  if (!canSend()) return;

  switch (msgType) {
    case llp_MsgType::request:
      m_stream.print('>');
      break;
    case llp_MsgType::response:
      m_stream.print('<');
      break;
    case llp_MsgType::error:
      m_stream.print('!');
      break;
    case llp_MsgType::update:
      m_stream.print('#');
      break;
    default:
      return;
  }
  
  char cmd[3];
  if (!getCmdStr(cmdId, cmd)) {
    abortFrame();
    return;
  }
  m_stream.write(cmd, 3);
  
  m_stream.print(':');
  
  m_stream.print(int2hex(len >> 4));
  m_stream.print(int2hex(len & 0x0F));
  
  m_stream.print(':');
  
  m_writeState = frameState::dataBin;
}

bool LittleLessProtocolA::setBin() {
  if (m_writeState == frameState::dataBin) {
    return true;
  } else if (m_writeState == frameState::dataASCII) {
    m_writeState = frameState::dataBin;
    m_stream.print('"');
    return true;
  } else {
    abortFrame();
    return false;
  }
}

bool LittleLessProtocolA::setASCII() {
  if (m_writeState == frameState::dataBin) {
    m_stream.print('"');
    m_writeState = frameState::dataASCII;
    return true;
  } else if (m_writeState == frameState::dataASCII) {
    return true;
  } else {
    abortFrame();
    return false;
  }
}

void LittleLessProtocolA::sendByte(uint8_t b) {
  if (!setBin()) return;
  m_stream.print(int2hex(b >> 4));
  m_stream.print(int2hex(b & 0x0F));
}

void LittleLessProtocolA::sendChar(char c) {
  if ((c >= ' ') && (c <= 127)) {
    if (!setASCII()) return;
    if ((c == '\\') || (c == '"')) {
      m_stream.print('\\');
    }
    m_stream.print(c);
  } else {
    sendByte(uint8_t(c));
  }
}

void LittleLessProtocolA::sendData(uint8_t len, const uint8_t *data) {
  for (; len > 0; --len, ++data) {
    sendByte(*data);
  }
}

void LittleLessProtocolA::sendStr(uint8_t len, const char *str) {
  for (; len > 0; --len, ++str) {
    sendChar(*str);
  }
  setBin();
}

void LittleLessProtocolA::sendStr_P(uint8_t len, const char *str) {
  for (; len > 0; --len, ++str) {
    sendChar(pgm_read_byte(str));
  }
  setBin();
}

void LittleLessProtocolA::endFrame() {
  if ((m_writeState == frameState::dataBin) || (m_writeState == frameState::dataASCII)) {
    setBin();
    m_stream.print(':');
    m_stream.print('F');  // ToDo CRC
    m_stream.print('F');
  }
  abortFrame();
}

void LittleLessProtocolA::abortFrame() {
  m_stream.print('\r');
  m_stream.print('\n');
  m_writeState = frameState::type;
}


void LittleLessProtocolA::loop() {
  if (m_stream.available() > 0) {
    char c = m_stream.read();

    if ((c == '\n') || (c == '\r')) {
      if (m_readState == frameState::done) {
        handleMsgFinish(m_msgType, m_cmdId, 0xFF, true);
      } else {
        handleMsgFinish(m_msgType, m_cmdId, 0xFF, false);
      }
      m_readState = frameState::type;
      m_cmdLen = 0;
    } else {
      switch (m_readState) {
        ////////////////////////////////////////////////////////
        case frameState::type:
          switch (c) {
            case '>': m_msgType = llp_MsgType::request; break;
            case '<': m_msgType = llp_MsgType::response; break;
            case '!': m_msgType = llp_MsgType::error; break;
            case '#': m_msgType = llp_MsgType::update; break;
            default:  m_msgType = llp_MsgType::none; break;
          }
          if ( m_msgType != llp_MsgType::none) m_readState = frameState::cmd;
          else                                 handleError();
          break;
        ////////////////////////////////////////////////////////
        //case frameState::colon1:
        case frameState::colon1:
        case frameState::colon2:
        case frameState::colon3:
          if (c == ':') {
            m_readState = (frameState)((int)m_readState + 1);
          } else {
            handleError();
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::cmd:
          m_cmd[m_cmdLen] = c;
          ++m_cmdLen;
          if (m_cmdLen >= 3) {
            m_cmdId = getCmdId(m_cmd);
            if (m_cmdId < 0xFF) {
              m_readState = frameState::colon1;
            } else {
              handleError();
            }
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::len:
          {
            int val = readHex(c);
            if (val >= 0) {
              m_rxData.msgTotalSize = (uint8_t)val;
              m_rxData.bufOffset = 0;
              m_rxData.bufSize = 0;
              if (canHandleMsg(m_msgType, m_cmdId, m_rxData)) m_readState = frameState::colon2;
              else                                            handleError();
            }
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::dataBin:
          if (c == '"') {
            m_readState = frameState::dataASCII;
          } else if ((m_rxData.bufOffset + m_rxData.bufSize) < m_rxData.msgTotalSize) {
            int val = readHex(c);
            if (val >= 0) {
              m_rxData.buf[m_rxData.bufSize] = (uint8_t)val;
              ++m_rxData.bufSize;
              if (m_rxData.bufSize >= m_rxData.bufTotalSize) {
                handleMsgData(m_msgType, m_cmdId, m_rxData);
                m_rxData.bufOffset += m_rxData.bufTotalSize;
                m_rxData.bufSize = 0;
              }
              if ((m_rxData.bufOffset + m_rxData.bufSize) >= m_rxData.msgTotalSize) {
                if (m_rxData.bufSize > 0) {
                  handleMsgData(m_msgType, m_cmdId, m_rxData);
                }
                m_readState = frameState::colon3;
              }
            }
          } else {
            handleError();
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::dataASCII:
          if (c == '"') {
            if ((m_rxData.bufOffset + m_rxData.bufSize) >= m_rxData.msgTotalSize) {
              if (m_rxData.bufSize > 0) {
                handleMsgData(m_msgType, m_cmdId, m_rxData);
              }
              m_readState = frameState::colon3;
            } else {
              m_readState = frameState::dataBin;
            }
          } else if ((m_rxData.bufOffset + m_rxData.bufSize) < m_rxData.msgTotalSize) {
            if (c == '\\') {
              m_readState = frameState::dataASCIIesc;
            } else {
              m_rxData.buf[m_rxData.bufSize] = (uint8_t)c;
              ++m_rxData.bufSize;
              if (m_rxData.bufSize >= m_rxData.bufTotalSize) {
                handleMsgData(m_msgType, m_cmdId, m_rxData);
                m_rxData.bufOffset += m_rxData.bufTotalSize;
                m_rxData.bufSize = 0;
              }
            }
          } else {
            handleError();
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::dataASCIIesc:
          m_rxData.buf[m_rxData.bufSize] = (uint8_t)c;
          ++m_rxData.bufSize;
          if (m_rxData.bufSize >= m_rxData.bufTotalSize) {
            handleMsgData(m_msgType, m_cmdId, m_rxData);
            m_rxData.bufOffset += m_rxData.bufTotalSize;
            m_rxData.bufSize = 0;
          }
          if ((m_rxData.bufOffset + m_rxData.bufSize) >= m_rxData.msgTotalSize) {
            if (m_rxData.bufSize > 0) {
              handleMsgData(m_msgType, m_cmdId, m_rxData);
            }
            m_readState = frameState::colon3;
          } else {
            m_readState = frameState::dataASCII;
          }
          break;
        ////////////////////////////////////////////////////////
        case frameState::checkSum:
          {
            int val = readHex(c);
            if (val >= 0) {
              // (uint8_t)val; // ToDo CRC check
              m_readState = frameState::done;
            }
          }
          break;
      }
    }
  }
}

int LittleLessProtocolA::readHex(char c) {
  if (((c >= '0') && (c <= '9')) ||
      ((c >= 'a') && (c <= 'f')) ||
      ((c >= 'A') && (c <= 'F'))) {
    if (m_cmdLen >= 2) m_cmdLen = 0;
    m_cmd[m_cmdLen] = c;
    ++m_cmdLen;
    if (m_cmdLen >= 2) {
      return (hex2int(m_cmd[0]) << 4) | hex2int(m_cmd[1]);
    } else {
      return -1; // not finish
    }
  } else {
    handleError();
    return -2;  // error
  }
}

void LittleLessProtocolA::handleError() {
  if (   (m_readState >= frameState::colon2)
      && (m_readState <= frameState::done)) {
    handleMsgFinish(m_msgType, m_cmdId, 0xFF, false);
  }
  m_readState = frameState::error;
}
