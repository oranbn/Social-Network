#include <encoderDecoder.h>
#include <time.h>

EncoderDecoder::EncoderDecoder() {}
EncoderDecoder::~EncoderDecoder() = default;

bool validDate(std::basic_string<char> &basicString);

enum opCodeEnum {
    REGISTER,
    LOGIN,
    LOGOUT,
    FOLLOW,
    POST,
    PM,
    LOGSTAT,
    STAT,
    BLOCK,
    NONE
};

opCodeEnum enumTranslator (std::string const& str) {
    if (str == "REGISTER") return REGISTER;
    if (str == "LOGIN") return LOGIN;
    if (str == "LOGOUT") return LOGOUT;
    if (str == "FOLLOW") return FOLLOW;
    if (str == "POST") return POST;
    if (str == "PM") return PM;
    if (str == "LOGSTAT") return LOGSTAT;
    if (str == "STAT") return STAT;
    if (str == "BLOCK") return BLOCK;
    return NONE;
}

bool EncoderDecoder::Encode(std::string line, char bytes[], int& length) {
    std::string firstWord;
    if (line.find(' ') != std::string::npos) {
        firstWord = line.substr(0, line.find(' ', 0));
        line = line.substr(firstWord.size() + 1);
    }
    else
        firstWord = line;
    std::vector<std::string> wordList;
    std::stringstream wordStream(line);
    switch((enumTranslator(firstWord))){
        case POST:
            return postEncode(line, bytes, length);
        case PM: {
            std::string userName = line.substr(0, line.find(' ', 0));
            line = line.substr(userName.size() + 1);
            return pmEncode(userName, line, bytes, length);
        }
        case STAT:
            return statEncode(line, bytes, length);
        case BLOCK:
            return blockEncode(line, bytes, length);
        case LOGSTAT:
            return logstatEncode(line,bytes,length);
        default:
            while(std::getline(wordStream, firstWord, ' '))
            {
                wordList.push_back(firstWord);
            }
    }
    switch (enumTranslator(firstWord)) {
        case NONE:
            return false;
        case REGISTER:
            return registerEncode(wordList, bytes, length);
        case LOGIN:
            return loginEncode(wordList, bytes, length);
        case LOGOUT:
            return logoutEncode(wordList, bytes, length);
        case FOLLOW:
            return followEncode(wordList, bytes, length);
        default:
            return false;
    }
}

bool EncoderDecoder::Decode(std::string& line, std::vector<char> messageToDecode) {
    // read 2 first bytes - make them short, switch short it can be 9/10/11 then, make a string for 9/10/11, if success return true else return fasle
    short opCode;
    try{
        char* op = new char[2];
        op[0] = messageToDecode.at(0);
        op[1] = messageToDecode.at(1);
        opCode = bytesToShort(op);
        delete[](op);
        switch(opCode){
            case 9: {
                std::string notificaion;
                if(messageToDecode.at(3)=='0')
                    notificaion = "PM";
                else
                    notificaion = "Public";
                    int firstSeparator=0;
                while(messageToDecode.at(firstSeparator) != '\0')
                    firstSeparator++;
                int secondSeparator=firstSeparator+1;
                while(messageToDecode.at(secondSeparator) != '\0')
                    secondSeparator++;
                char* postingUserBytes = new char[firstSeparator-3];
                char* contentBytes = new char[secondSeparator-firstSeparator-1];
                for(int i = 4; i < firstSeparator; i++)
                    postingUserBytes[i-4] = messageToDecode.at(i);
                for(int i = firstSeparator+1; i < secondSeparator; i++)
                    contentBytes[i-firstSeparator-1] = messageToDecode.at(i);
                std::string postingUser(postingUserBytes);
                std::string content(contentBytes);
                line = "NOTIFICATION "+notificaion+" "+postingUser+" "+content;
                return true;
            }
            case 10:{
                short messageOpCode;
                char* messageOP = new char[2];
                messageOP[0] = messageToDecode.at(2);
                messageOP[1] = messageToDecode.at(3);
                messageOpCode = bytesToShort(messageOP);
                delete[](messageOP);
                char* contentBytes = new char[messageToDecode.size()-4];
                for(int i = 4; i < messageToDecode.size(); i++)
                    contentBytes[i-4] = messageToDecode.at(i);
                std::string content(contentBytes);
                delete[](contentBytes);
                line = "ACK " + std::to_string(messageOpCode) +" "+ content;
                return true;
            }
            case 11: {
                short messageOpCode;
                char *op = new char[2];
                op[0] = messageToDecode.at(2);
                op[1] = messageToDecode.at(3);
                messageOpCode = bytesToShort(op);
                line = "ERROR "+messageOpCode;
                delete[](op);
                return true;
            }
            default:
                return false;
        }
    }
    catch(std::exception e)
    {
        return false;
    }
}

short EncoderDecoder::bytesToShort(char* bytesArr)
{
    short result = (short)((bytesArr[0] & 0xff) << 8);
    result += (short)(bytesArr[1] & 0xff);
    return result;
}
void shortToBytes(short num, char* bytesArr)
{
    bytesArr[0] = ((num >> 8) & 0xFF);
    bytesArr[1] = (num & 0xFF);
}

bool validDate(std::basic_string<char> &date) {
    if(date[2] != '-' || date[5] != '-' || date.length()!=10)
        return false;
    std::string day;
    day.push_back(date[0]);
    day.push_back(date[1]);
    std::string month;
    month.push_back(date[3]);
    month.push_back(date[4]);
    std::string year;
    year.push_back(date[6]);
    year.push_back(date[7]);
    year.push_back(date[8]);
    year.push_back(date[9]);
    try {
        if (std::stoi(day) <= 0 || std::stoi(day) > 31)
            return false;
        if (std::stoi(month) <= 0 || std::stoi(day) > 12)
            return false;
        if (std::stoi(year) < 1900 || std::stoi(year) > 2022)
            return false;
    }
    catch(std::exception e){
        return false;
    }
    return true;
}

bool EncoderDecoder::registerEncode(std::vector<std::string> wordList, char bytes[], int& length){
    if(wordList.size() != 3)
        return false;
    if(!validDate(wordList[3]))
        return false;
    char opcodeByteArr[2];
    shortToBytes(1, opcodeByteArr);
    char zeroByte = '\0';
    char const *userNameBytes = wordList[0].c_str();
    char const *passwordBytes = wordList[1].c_str();
    char const *birthDayBytes = wordList[2].c_str();
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    int updatedFreeSpot = 2;
    for(int i=0; i<sizeof(*userNameBytes);i++)
        bytes[i+updatedFreeSpot] = userNameBytes[i];
    updatedFreeSpot += sizeof(*userNameBytes);
    bytes[updatedFreeSpot] = zeroByte;
    updatedFreeSpot++;
    for(int i=0; i<sizeof(*passwordBytes);i++)
        bytes[updatedFreeSpot+i] = passwordBytes[i];
    updatedFreeSpot += sizeof(*passwordBytes);
    bytes[updatedFreeSpot] = zeroByte;
    updatedFreeSpot++;
    for(int i=0; i<sizeof(*birthDayBytes);i++)
        bytes[updatedFreeSpot+i] = birthDayBytes[i];
    bytes[sizeof(*bytes)-2] = zeroByte;
    bytes[sizeof(*bytes)-1] = ';';
    return true;
}

bool EncoderDecoder::loginEncode(std::vector<std::string> wordList, char bytes[], int& length){
    if(wordList.size() != 2)
        return false;
    char opcodeByteArr[2];
    shortToBytes(2, opcodeByteArr);
    char zeroByte = '\0';
    char const *userNameBytes = wordList[0].c_str();
    char const *passwordBytes = wordList[1].c_str();
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 2;
    for(int i=0; i<sizeof(*userNameBytes);i++)
        bytes[i+length] = userNameBytes[i];
    length += sizeof(*userNameBytes);
    bytes[length] = zeroByte;
    length++;
    for(int i=0; i<sizeof(*passwordBytes);i++)
        bytes[length+i] = passwordBytes[i];
    length += sizeof(*passwordBytes);
    bytes[length] = zeroByte;
    length++;
    bytes[length] = '1';
    length++;
    bytes[length] = ';';
    return true;
}

bool EncoderDecoder::logoutEncode(std::vector<std::string> wordList, char bytes[], int& length){
    if(wordList.size() != 0)
        return false;
    char opcodeByteArr[2];
    shortToBytes(3, opcodeByteArr);
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    return true;
}

bool EncoderDecoder::followEncode(std::vector<std::string> wordList, char bytes[], int& length){
    if(wordList.size() != 2)
        return false;
    char opcodeByteArr[2];
    char followByteArr[2];
    shortToBytes(4, opcodeByteArr);
    if(wordList[0] == "0")
        shortToBytes(0, followByteArr);
    else if(wordList[0] == "1")
        shortToBytes(1, followByteArr);
    else
        return false;
    char zeroByte = '\0';
    char const *userNameBytes = wordList[1].c_str();
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    bytes[2] = followByteArr[0];
    bytes[3] = followByteArr[1];
    length = 4;
    for(int i=0; i<sizeof(*userNameBytes);i++)
        bytes[i+length] = userNameBytes[i];
    length += sizeof(*userNameBytes);
    bytes[length] = zeroByte;
    length++;
    bytes[length] = ';';
    length++;
    return true;
}

bool EncoderDecoder::postEncode(std::string content, char bytes[], int& length){
    //message can be containing lots of words
    char opcodeByteArr[2];
    shortToBytes(5, opcodeByteArr);
    char zeroByte = '\0';
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 2;
    char const *contentBytes = content.c_str();
    for(int i=0; i<sizeof(*contentBytes);i++)
        bytes[i+length] = contentBytes[i];
    length += sizeof(*contentBytes);
    bytes[length] = zeroByte;
    length++;
    bytes[length] = ';';
    length++;
    return true;
}

bool EncoderDecoder::pmEncode(std::string userName, std::string content, char bytes[], int& length){
    //auto now = std::chrono::system_clock::now();
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string sendDate(18, '\0');
    std::strftime(&sendDate[0], sendDate.size(), "%d-%m-%Y %H:%M", std::localtime(&now));
    char opcodeByteArr[2];
    shortToBytes(6, opcodeByteArr);
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 2;
    char zeroByte = '\0';
    char const *userNameBytes = userName.c_str();
    char const *contentBytes = content.c_str();
    char const *sendDateBytes = sendDate.c_str();
    for(int i=0; i<sizeof(*userNameBytes);i++)
        bytes[i+length] = userNameBytes[i];
    length += sizeof(*userNameBytes);
    bytes[length] = zeroByte;
    length++;
    for(int i=0; i<sizeof(*contentBytes);i++)
        bytes[length+i] = contentBytes[i];
    length += sizeof(*contentBytes);
    bytes[length] = zeroByte;
    length++;
    for(int i=0; i<sizeof(*sendDateBytes);i++)
        bytes[length+i] = contentBytes[i];
    length += sizeof(*sendDateBytes);
    bytes[length] = zeroByte;
    length++;
    bytes[length] = zeroByte;
    length++;
    bytes[length] = ';';
    return true;
}

bool EncoderDecoder::logstatEncode(std::string line, char bytes[], int& length){
    if(line != "LOGSTAT")
        return false;
    char opcodeByteArr[2];
    shortToBytes(7, opcodeByteArr);
    char zeroByte = '\0';
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 3;
    bytes[2] = zeroByte;
    return true;
}

bool EncoderDecoder::statEncode(std::string userListString, char bytes[], int& length){
    char opcodeByteArr[2];
    shortToBytes(8, opcodeByteArr);
    char zeroByte = '\0';
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 2;
    char const *usernameListBytes = userListString.c_str();
    for(int i=0; i<sizeof(*usernameListBytes);i++)
        bytes[i+length] = usernameListBytes[i];
    length += sizeof(*usernameListBytes);
    bytes[length] = '\0';
    length++;
    bytes[length] = ';';
    length++;
    return true;
}

bool EncoderDecoder::blockEncode(std::string username, char bytes[], int& length){
    char opcodeByteArr[2];
    shortToBytes(12, opcodeByteArr);
    char zeroByte = '\0';
    bytes[0] = opcodeByteArr[0];
    bytes[1] = opcodeByteArr[1];
    length = 2;
    char const *usernameBytes = username.c_str();
    for(int i=0; i<sizeof(*usernameBytes);i++)
        bytes[i+length] = usernameBytes[i];
    length += sizeof(*usernameBytes);
    bytes[length] = '\0';
    length++;
    bytes[length] = ';';
    length++;
    return true;
}