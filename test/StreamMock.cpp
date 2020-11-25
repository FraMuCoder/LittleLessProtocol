/*
 * Little Less Protocol
 * Copyright (C) 2020 Frank Mueller
 *
 * SPDX-License-Identifier: MIT
 */

#include "StreamMock.h"

const int DEC = 10;

int StreamMock::read() {
  if (available()) {
    int d = m_dataIn[0];
    m_dataIn.pop_front();
    return d;
  } else {
    return -1;
  }
}

int StreamMock::peek() {
  if (available()) {
    return m_dataIn[0];
  } else {
    return -1;
  }
}

size_t StreamMock::write(uint8_t d) { 
  m_dataOut.push_back(d);
  return 1;
}

size_t StreamMock::write(const char *str) {
  return write((const uint8_t *)str, std::strlen(str));
}

size_t StreamMock::write(const uint8_t *buffer, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    write(buffer[i]);
  }
  return size;
}

size_t StreamMock::print(const char str[]) {
  return write(str);
}

size_t StreamMock::print(char c) {
  return write(c);
}

size_t StreamMock::println(const char str[]) {
  size_t ret = println(str);
  ret += println();
  return ret;
}

size_t StreamMock::println(char c) {
  size_t ret = println(c);
  ret += println();
  return ret;
}

size_t StreamMock::println(void) {
  return print("\r\n");
}
