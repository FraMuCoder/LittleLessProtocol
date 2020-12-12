/*
 * Simple Echo Service using LittleLessProtocolA
 *
 * Most of this code is generated using protocol description from EchoService.yaml.
 * See https://github.com/FraMuCoder/LittleLessProtocolSuite to see how to generate such code.
 *
 * Only the following things are added:
 *  - Add one byte member as rx buffer
 *  - implement the member functions canHandleEcho(...), handleEchoData(...) and handleEchoFinish(...)
 * That's all :-)
 *
 * Try to send following commands using serial monitor:
 *
 * This will be just echoed:
 *    >ECH:03:010203:FF
 * This will be echoed but as hex data;
 *    >ECH:04:"Test":FF
 *
 * This is an invalid frame, it will be echoed until an error was detected
 *    >ECH:05:0102030405060708:FF
 *
 * This is a response, it will not be echoed:
 *    <ECH:03:010203:FF
 *
 * by Frank Müller
 *
 * This is distributed under the MIT License.
 */

#include <LittleLessProtocol.h>

class EchoServiceProtocolA : public LittleLessProtocolA {
public:
  struct cmd {
    enum : uint8_t  {
      Echo = 0
    };
  };

  EchoServiceProtocolA(Stream &stream)
    : LittleLessProtocolA(stream)
  {}

  virtual uint8_t getCmdId(const char cmd[3]);
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]);
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, llp_result result);

  inline bool canHandleEcho(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleEchoData(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleEchoFinish(llp_MsgType msgType, llp_result result);

private:
  static const char * const S_CMDS[];
  uint8_t m_rxBuffer; // this was added manual, just one byte rx buffer
};

EchoServiceProtocolA echoServiceProtocol(Serial);

void setup() {
  Serial.begin(9600);
}

void loop() {
  echoServiceProtocol.loop();
}


//////////////////////////////////////////////////
// ECH - Echo (0)
// Use markdown to describe your command.
/////////////////////////
// Message types: >, <
//
// All data received via '>' is echoed via '<'
//
// Just some binary data
//////////////////////////////////////////////////

bool EchoServiceProtocolA::canHandleEcho(llp_MsgType msgType, llp_RxStruct &rx) {
  if (llp_MsgType::request == msgType) { // only react on a request
    rx.buf = &m_rxBuffer;
    rx.bufTotalSize = sizeof(m_rxBuffer);

    // start echo frame
    startFrame(llp_MsgType::response, cmd::Echo, rx.msgTotalSize);

    return true;
  } else {
    return false;
  }
}

void EchoServiceProtocolA::handleEchoData(llp_MsgType msgType, llp_RxStruct &rx) {
  // this is called for every received data byte, just forward it
  sendByte(m_rxBuffer);
}

void EchoServiceProtocolA::handleEchoFinish(llp_MsgType msgType, llp_result result) {
  if (llp_result::ok == result) {
    endFrame();   // end a valid frame
  } else {
    abortFrame(); // abort on error
  }
}
