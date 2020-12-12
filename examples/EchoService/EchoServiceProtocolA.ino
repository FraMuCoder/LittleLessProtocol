
//////////////////////////////////////////////////
// Command ID handling
//////////////////////////////////////////////////

static const char CMD_ECHO[3] PROGMEM = "ECH";

const char * const EchoServiceProtocolA::S_CMDS[1] PROGMEM = {
  CMD_ECHO
};

uint8_t EchoServiceProtocolA::getCmdId(const char cmd[3]) {
  for (uint8_t i = 0; i < sizeof(S_CMDS)/sizeof(S_CMDS[0]); ++i) {
    char *str = pgm_read_ptr(S_CMDS + i);
    if (0 == memcmp_P(cmd, str, 3)) {
      return 0 + i;  
    }
  }
  return 0xFF;
}

bool EchoServiceProtocolA::getCmdStr(uint8_t cmdId, char cmd[3]) {
  if (   (cmdId >= 0)
      && ((cmdId - 0) < sizeof(S_CMDS)/sizeof(S_CMDS[0]))) {
    char *str = pgm_read_ptr(S_CMDS + cmdId - 0);  
    memcpy_P(cmd, str, 3);
    return true;
  } else {
    return false;
  }
}


//////////////////////////////////////////////////
// general rx handling
//////////////////////////////////////////////////

bool EchoServiceProtocolA::canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Echo: return canHandleEcho(msgType, rx);
    default:
      return false;
  }
}

void EchoServiceProtocolA::handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Echo: handleEchoData(msgType, rx); break;
    default:
      break;
  }
}

void EchoServiceProtocolA::handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, llp_result result) {
  switch (cmdId) {
    case cmd::Echo: handleEchoFinish(msgType, result); break;
    default:
      break;
  }
}