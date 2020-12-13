/*
 * Chat Server example
 * 
 * Most of this code is generated using protocol description from Chat.yaml.
 * See https://github.com/FraMuCoder/LittleLessProtocolSuite to see how to generate such code.
 *
 * by Frank Müller
 *
 * This is distributed under the MIT License.
 */

//////////////////////////////////////////////////
// LittleLessBase handling
//////////////////////////////////////////////////

const char ChatProtocolA::PROTO_NAME[]  PROGMEM = "Chat";
const char ChatProtocolA::PROTO_EXTRA[] PROGMEM = "Server";

void ChatProtocolA::getAppName(uint8_t &len, const char **name) {
  len = strlen_P(PROTO_NAME);
  *name = PROTO_NAME; 
}

void ChatProtocolA::getAppExtra(uint8_t &len, const char **extra) {
  len = strlen_P(PROTO_EXTRA);
  *extra = PROTO_EXTRA;
}


//////////////////////////////////////////////////
// Command ID handling
//////////////////////////////////////////////////

static const char CMD_LOGIN[3] PROGMEM = "LOG";
static const char CMD_MESSAGE[3] PROGMEM = "MSG";
static const char CMD_USER_LIST[3] PROGMEM = "LST";

const char * const ChatProtocolA::S_CMDS[3] PROGMEM = {
  CMD_LOGIN, CMD_MESSAGE, CMD_USER_LIST
};

uint8_t ChatProtocolA::getCmdId(const char cmd[3]) {
  for (uint8_t i = 0; i < sizeof(S_CMDS)/sizeof(S_CMDS[0]); ++i) {
    char *str = pgm_read_ptr(S_CMDS + i);
    if (0 == memcmp_P(cmd, str, 3)) {
      return 8 + i;  
    }
  }
  return LittleLessBaseA::getCmdId(cmd);
}

bool ChatProtocolA::getCmdStr(uint8_t cmdId, char cmd[3]) {
  if (   (cmdId >= 8)
      && ((cmdId - 8) < sizeof(S_CMDS)/sizeof(S_CMDS[0]))) {
    char *str = pgm_read_ptr(S_CMDS + cmdId - 8);  
    memcpy_P(cmd, str, 3);
    return true;
  } else {
    return LittleLessBaseA::getCmdStr(cmdId, cmd);
  }
}


//////////////////////////////////////////////////
// general rx handling
//////////////////////////////////////////////////

bool ChatProtocolA::canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Login: return canHandleLogin(msgType, rx);
    case cmd::Message: return canHandleMessage(msgType, rx);
    case cmd::UserList: return canHandleUserList(msgType, rx);
    default:
      return LittleLessBaseA::canHandleMsg(msgType, cmdId, rx);
  }
}

void ChatProtocolA::handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx) {
  switch (cmdId) {
    case cmd::Login: handleLoginData(msgType, rx); break;
    case cmd::Message: handleMessageData(msgType, rx); break;
    case cmd::UserList: handleUserListData(msgType, rx); break;
    default:
      LittleLessBaseA::handleMsgData(msgType, cmdId, rx);
      break;
  }
}

void ChatProtocolA::handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &rx, llp_result result) {
  switch (cmdId) {
    case cmd::Login: handleLoginFinish(msgType, rx, result); break;
    case cmd::Message: handleMessageFinish(msgType, rx, result); break;
    case cmd::UserList: handleUserListFinish(msgType, rx, result); break;
    default:
      LittleLessBaseA::handleMsgFinish(msgType, cmdId, rx, result);
      break;
  }
}