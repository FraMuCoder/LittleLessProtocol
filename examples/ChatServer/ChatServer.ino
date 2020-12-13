/*
 * Chat Server example
 * 
 * Most of this code is generated using protocol description from Chat.yaml.
 * See https://github.com/FraMuCoder/LittleLessProtocolSuite to see how to generate such code.
 *
 * This is a serial chat server. There are two serial connections. One using the HW serial
 * normally via Arduinos USB connector the other one using a SW serial using pin 2 and 3.
 *
 * You can use a USB-to-Serial-Adapter connected to the SW serial at pin 2 and 3
 * but you should not connect it directly because of different power sources.
 * Here is a possible connection using two optocouplers 4N35:
 *
 * Arduino <=> 2xOptocoupler <=> USB-to-Serial-Adapter
 *                  4N35
 *   ---+         +------+
 *   GND|---------|4    3|
 *      |         |      |
 * (Rx)2|-----+---|5    2|--------- Tx
 *      |     |   |      |
 *      |     R1  |6    1|--R2--+-- Vcc (3.3V or 5V)
 *      |     |   +------O      |
 *    5V|-----+                 |
 *      |     |                 |
 *      |     R3                |
 *      |     |   O------+      R4
 *      |     +---|1    6|      |
 *      |         |      |      |
 * (Tx)3|---------|2    5|------+-- Rx
 *      |         |      |
 *      |         |3    4|--------- GND
 *      |         +------+
 *
 *  R1  1k
 *  R2  100 for 3.3V logic or 220 for 5V logic
 *  R3  220
 *  R4  1k
 *
 * Try to send the following messages on both serial connections:
 *
 * First start the communication:
 *    >ver:0E:11111146"Chat""Client":FF
 * Login:
 *    >LOG:0B:05"mouse"04"1234":FF
 * List all currently logged in users:
 *    >LST:00::FF
 * Send some messages:
 *    >MSG:04:"Test":FF
 *    >MSG:64:"The quick brown fox jumps over a lazy dog. 0123456789_?!*+-/=# The five boxing wizards jump quickly.":FF
 *
 * by Frank Müller
 *
 * This is distributed under the MIT License.
 */

#include <LittleLessProtocol.h>
#include <SoftwareSerial.h>

// Set this to 1 to see some extra output
#define DBG_OUTPUT 0

// Pins for SoftwareSerial
int rxPin = 2;
int txPin = 3;

SoftwareSerial swSerial(rxPin, txPin);

class ChatProtocolA : public LittleLessBaseA {
public:
  struct cmd {
    enum : uint8_t  {
      Login = 8,
      Message = 9,
      UserList = 10
    };
  };

  ChatProtocolA(Stream &stream);

  virtual void getAppName(uint8_t &len, const char **name);
  virtual void getAppExtra(uint8_t &len, const char **extra);
  virtual void handleConStateChanged(bool conState);

  virtual uint8_t getCmdId(const char cmd[3]);
  virtual bool getCmdStr(uint8_t cmdId, char cmd[3]);
  virtual bool canHandleMsg(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgData(llp_MsgType msgType, uint8_t cmdId, llp_RxStruct &rx);
  virtual void handleMsgFinish(llp_MsgType msgType, uint8_t cmdId, const llp_RxStruct &rx, llp_result result);

  inline bool canHandleLogin(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleLoginData(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleLoginFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result);
  inline bool canHandleMessage(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleMessageData(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleMessageFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result);
  inline bool canHandleUserList(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleUserListData(llp_MsgType msgType, llp_RxStruct &rx);
  inline void handleUserListFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result);

  void setOtherSide(ChatProtocolA *other) {
    m_other = other;
  }

private:
  static const char PROTO_NAME[];
  static const char PROTO_EXTRA[];
  static const char * const S_CMDS[];
  ChatProtocolA   *m_other;
  bool            m_isLoggedIn;
  uint8_t         m_userID; // 0xFF means no user logged in
  uint8_t         m_rxBuffer[100];

  bool sendLoggedInUserName(const ChatProtocolA *proto);
};

class UserDB {
public:
  enum result {
    OK,
    unknownUser,
    pwWrong
  };

  UserDB();

  uint8_t addUser(const String &name, const String &pw);
  const String *getUser(uint8_t id);
  result checkPW(const String &name, const String &pw, uint8_t &id);
private:
  static const uint8_t MAX_USER = 3;
  uint8_t       m_userCnt;
  String        m_user[MAX_USER];
  String        m_pw[MAX_USER];
};

ChatProtocolA chatA(Serial);
ChatProtocolA chatB(swSerial);

UserDB userDB;

void setup() {
  Serial.begin(9600);
  swSerial.begin(9600);

  chatA.setOtherSide(&chatB);
  chatB.setOtherSide(&chatA);

  chatA.requestVersion();
  chatB.requestVersion();
}

void loop() {
  chatA.loop();
  chatB.loop();

  static long last = 0;
  long ms = millis();
  if ((ms - last) >= 5000) {
    if (!chatA.isConnected()) { chatA.requestVersion(); }
    if (!chatB.isConnected()) { chatB.requestVersion(); }
    last = ms;
  }
}

ChatProtocolA::ChatProtocolA(Stream &stream)
    : LittleLessBaseA(stream, 0x11)
    , m_other(NULL)
    , m_isLoggedIn(false)
    , m_userID(0xFF)
  {}

//////////////////////////////////////////////////
// Connection state handling
//////////////////////////////////////////////////

void ChatProtocolA::handleConStateChanged(bool conState) {
#if DBG_OUTPUT == 1
  Serial.print(F("handleConStateChanged "));
  Serial.println(conState);
#endif
}


//////////////////////////////////////////////////
// LOG - Login (8)
// Login to chat system.
// If the user name is not known at chat system a new account is created.
/////////////////////////
// Message types: >
// Senders: Client
// Receivers: Server
//
// Client tries to login at server.
//
// | Param   | Length | Description
// | ------- | ------:| -----------
// | userLen |      1 | Length of user name (binary, 1..10)
// | user    |  1..10 | User name (ASCII)
// | pwLen   |      1 | Length of password (binary, 4..20)
// | pw      |  4..20 | Password (ASCII)
/////////////////////////
// Message types: <
// Senders: Server
// Receivers: Client
//
// Server sends login result to client.
//
// | Param  | Length | Description
// | ------ | ------:| -----------
// | result |      1 | 0 => OK, 1 => error (binary)
//////////////////////////////////////////////////

bool ChatProtocolA::canHandleLogin(llp_MsgType msgType, llp_RxStruct &rx) {
  if (   isConnected() && (msgType == llp_MsgType::request) 
      && (rx.msgTotalSize >= 7) && (rx.msgTotalSize <= 32)) {
    // new login try, 1st logout
    m_userID = 0xFF;
    m_isLoggedIn = false;
  
    // prepare rx buffer
    rx.buf = m_rxBuffer;
    rx.bufTotalSize = sizeof(m_rxBuffer); // read all at once
    return true;
  } else {
    return false;
  }
}

void ChatProtocolA::handleLoginData(llp_MsgType msgType, llp_RxStruct &rx) {
  uint8_t lenUser = m_rxBuffer[0];
  uint8_t lenPW = m_rxBuffer[lenUser + 1];
  uint8_t loginResult = 0x01; // start with err

  // check user name an PW length
  if ((lenUser < 1) || (lenUser > 10) || (lenPW < 4) || (lenPW > 20)) {
    rx.buf = NULL;  // this will fail mesg rx evin if checksum is ok
    return;
  }

  // make \0 terminate strings
  m_rxBuffer[lenUser + 1] = 0;
  m_rxBuffer[lenUser + lenPW + 2] = 0;

  String user((char*)&m_rxBuffer[1]);
  String pw((char*)&m_rxBuffer[lenUser + 2]);

  UserDB::result res = userDB.checkPW(user, pw, m_userID);

  if (UserDB::unknownUser == res) {
    m_userID = userDB.addUser(user, pw);
  }

  if (m_userID == 0xFF) {
    rx.buf = NULL;  // this will fail msg rx even if checksum is ok
  }
}

void ChatProtocolA::handleLoginFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result) {
  uint8_t res;
  if (llp_result::ok == result) {
  #if DBG_OUTPUT == 1
    Serial.println(F("logged in"));
  #endif
    m_isLoggedIn = true;
    res = 0;
  } else {
  #if DBG_OUTPUT == 1
    Serial.println(F("login failed"));
  #endif
    m_userID = 0xFF;
    m_isLoggedIn = false;
    res = 1;
  }
  delay(1); // looks like SoftwareSerial Tx sometimes gets stuck without a delay after Rx
  startFrame(msgType, cmd::Login, 1);
  sendByte(res);
  endFrame();
}


//////////////////////////////////////////////////
// MSG - Message (9)
// Send a chat message.
/////////////////////////
// Message types: >
// Senders: Client
// Receivers: Server
//
// Client sends a message to server.
// There is no response from server.
//
// | Param | Length | Description
// | ----- | ------:| -----------
// | text  | 1..100 | Chat message (ASCII)
/////////////////////////
// Message types: <
// Senders: Server
// Receivers: Client
//
// Server sends a message from a client to an other client.
//
// | Param   | Length | Description
// | ------- | ------:| -----------
// | userLen |      1 | Length of user name (binary, 1..10)
// | user    |  1..10 | User name (ASCII)
// | text    | 1..100 | Chat message (ASCII)
//////////////////////////////////////////////////

bool ChatProtocolA::canHandleMessage(llp_MsgType msgType, llp_RxStruct &rx) {
  if ((llp_MsgType::request == msgType) && (100 >= rx.msgTotalSize) && m_isLoggedIn) {
    // prepare rx buffer
    rx.buf = m_rxBuffer;
    rx.bufTotalSize = sizeof(m_rxBuffer); // read all at once
    return true;
  }
  return false;
}

void ChatProtocolA::handleMessageData(llp_MsgType msgType, llp_RxStruct &rx) {
  // nothing to do here
}

void ChatProtocolA::handleMessageFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result) {
  if ((llp_result::ok == result) && (NULL != m_other) && m_other->m_isLoggedIn) {
    const String *name = userDB.getUser(m_userID);
    if (NULL != name) {
      m_other->startFrame(llp_MsgType::update, cmd::UserList, 1 + name->length() + rx.msgTotalSize);
      m_other->sendByte(name->length());
      m_other->sendStr(name->length(), name->c_str());
      m_other->sendStr(rx.msgTotalSize, (char *)m_rxBuffer);
      m_other->endFrame();
    }
  }
}


//////////////////////////////////////////////////
// LST - User list (10)
// List logged in users.
// 
// A typical sequence is:
// 
// 1. `>LST:00::FF`
// 2. `#LST:05:"UserA":FF`
// 3. `#LST:05:"UserB":FF`
// 4. `#LST:05:"UserC":FF`
// 5. `<LST:02:0300:FF`
/////////////////////////
// Message types: >
// Senders: Client
// Receivers: Server
//
// Client requests the list of logged in users.
//
// Empty data
/////////////////////////
// Message types: #
// Senders: Server
// Receivers: Client
//
// Server sends one of the logged in users.
// The server send one message of this type for every logged in user also the requested user.
// This messages are send before result is send.
//
// | Param | Length | Description
// | ----- | ------:| -----------
// | user  |  1..10 | User name (ASCII)
/////////////////////////
// Message types: <
// Senders: Server
// Receivers: Client
//
// Sever finish the client request to list all logged in users.
//
// | Param  | Length | Description
// | ------ | ------:| -----------
// | count  |      1 | User count (0..255) (binary)
//////////////////////////////////////////////////

bool ChatProtocolA::canHandleUserList(llp_MsgType msgType, llp_RxStruct &rx) {
  if ((llp_MsgType::request == msgType) && (0 == rx.msgTotalSize) && m_isLoggedIn) {
    // empty data, no buffer needed
    return true;
  } else {
    return false;
  }
}

void ChatProtocolA::handleUserListData(llp_MsgType msgType, llp_RxStruct &rx) {
  // this is never called
}

void ChatProtocolA::handleUserListFinish(llp_MsgType msgType, const llp_RxStruct &rx, llp_result result) {
  if (llp_result::ok == result) {
    delay(1); // looks like SoftwareSerial Tx sometimes gets stuck without a delay after Rx
    uint8_t users = 0;
    if (sendLoggedInUserName(m_other)) ++users;
    if (sendLoggedInUserName(this)) ++users;
    startFrame(llp_MsgType::response, cmd::UserList, 1);
    sendByte(users);
    endFrame();
  }
}

bool ChatProtocolA::sendLoggedInUserName(const ChatProtocolA *proto) {
  if ((NULL != proto) && proto->m_isLoggedIn) {
    const String *name = userDB.getUser(proto->m_userID);
    if (NULL != name) {
      startFrame(llp_MsgType::update, cmd::UserList, name->length());
      sendStr(name->length(), name->c_str());
      endFrame();
      return true;
    }
  }
  return false;
}
  

//////////////////////////////////////////////////
// UserDB
//////////////////////////////////////////////////

UserDB::UserDB()
  : m_userCnt(0)
{}

uint8_t UserDB::addUser(const String &name, const String &pw) {
  uint8_t id = 0xFF;
  
  if (m_userCnt < MAX_USER) {
    id = m_userCnt;
    m_user[id] = name;
    m_pw[id] = pw;
    ++m_userCnt;
  }
  
  return id;
}

const String *UserDB::getUser(uint8_t id) {
  if (id < m_userCnt) {
    return &m_user[id];
  } else {
    return NULL;
  }
}

UserDB::result UserDB::checkPW(const String &name, const String &pw, uint8_t &id) {
  for (uint8_t i = 0; i < m_userCnt; ++i) {
    if (m_user[i] == name) {
      if (m_pw[i] == pw) {
        id = i;
        return OK;
      } else {
        return pwWrong;
      }
    }
  }
  return unknownUser;
}
