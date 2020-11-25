/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef _STREAMMOCK_H_
#define _STREAMMOCK_H_

#include <cstdint>
#include <cstring>
#include <string>
#include <deque>

extern const int DEC;

class StreamMock {
public:

  virtual int available() { return m_dataIn.size(); }
  virtual int read();
  virtual int peek();

  virtual size_t write(uint8_t);
  size_t write(const char *str);
  virtual size_t write(const uint8_t *buffer, size_t size);
  size_t write(const char *buffer, size_t size) {
    return write((const uint8_t *)buffer,  size);
  }

  size_t print(const char[]);
  size_t print(char);

  size_t println(const char[]);
  size_t println(char);
  size_t println(void);

  void putInput(uint8_t in) {
    m_dataIn.push_back(in);
  }

  void putInput(char in) { putInput((uint8_t)in); }

  void putInput(const std::string &in) {
    for (const char &c : in) {
      putInput(c);
    }
  }

  size_t getOutputSize() {
    return m_dataOut.size();
  }

  uint8_t getOutputByte() {
    uint8_t out = m_dataOut[0];
    m_dataOut.pop_front();
    return out;
  }

  std::string getOutputStr() {
    std::string str;
    for (const uint8_t &d : m_dataOut) {
      str += (char)d;
    }
    m_dataOut.clear();
  }

private:
  std::deque<uint8_t>     m_dataIn;
  std::deque<uint8_t>     m_dataOut;
};

#endif // _STREAMMOCK_H_
