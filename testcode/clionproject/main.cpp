#include "stdafx.h"
#include "SerialPort.h"
#include <process.h>
#include <iostream>
#include <string>

#include <sstream>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

using namespace std;

UCHAR  CmdReadId[8] = { 0x01, 0x08, 0xA1, 0x20, 0x00, 0x01, 0x00, 0x76 };

UCHAR  CmdReadBlock[8] = { 0x01, 0x08, 0xA3, 0x20, 0x00, 0x01, 0x00, 0x00 }; //01 08 A3 20 01 01 00 75

UCHAR  Cmd[23] = { 0x01, 0x17, 0xA4, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
UCHAR  Cmdwrite[23] = { 0x01, 0x17, 0xA4, 0x20, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

void CheckSumOut(UCHAR *buf, UCHAR len)
{
	UCHAR i;
	UCHAR checksum;
	checksum = 0;
	for (i = 0; i < (len - 1); i++)
	{
		checksum ^= buf[i];
	}
	buf[len - 1] = (UCHAR)~checksum;
}

bool CheckSumIn(UCHAR *buf, UCHAR len)
{
	UCHAR i;
	UCHAR checksum;
	checksum = 0;
	for (i = 0; i < (len - 1); i++)
	{
		checksum ^= buf[i];
	}
	if (buf[len - 1] == (UCHAR)~checksum)
	{
		return true;
	}
	return false;
}

//字节流转换为十六进制字符串的另一种实现方式
void Hex2Str(const UCHAR *sSrc, UCHAR *sDest, int nSrcLen)
{
	int  i;
	char szTmp[3];

	for (i = 0; i < nSrcLen; i++)
	{
		sprintf_s(szTmp, "%02X", (unsigned char)sSrc[i]);
		memcpy(&sDest[i * 2], szTmp, 2);
	}
	sDest[nSrcLen * 2 ] = '\0';
	return;
}
//十六进制字符串转换为字节流
void HexStrToByte(const UCHAR* source,  UCHAR* dest, int sourceLen)
{
    short i;
    unsigned char highByte, lowByte;

    for (i = 0; i < sourceLen; i += 2)
    {
        highByte = toupper(source[i]);
        lowByte = toupper(source[i + 1]);

        if (highByte > 0x39)
            highByte -= 0x37;
        else
            highByte -= 0x30;

        if (lowByte > 0x39)
            lowByte -= 0x37;
        else
            lowByte -= 0x30;

        dest[i / 2] = (highByte << 4) | lowByte;
    }
    return;
}

//将读取到的块数据（员工id）转换为整数
int UcharUsrIdtoInt(UCHAR temp[]){
    char s = (char)temp[1];
    int usrdata = s - '0';
    cout<<"usr id is:"<<usrdata<<endl;
    return usrdata;
}
//将读取到的块数据（总出勤时间）转换为整数
int UcharUsrClockTimetoInt(UCHAR temp[]){
    char stemp[33];
    int testnum = 0;
    int firstnum = (char)temp[0]-'0';
    for(int j=1;j<=firstnum;j++){
        char s = (char)temp[j];
        stemp[j-1]=s;
//      一个月小于2592000秒，位数小于等于7位
    }
    string str = string(stemp);
//    cout<<"str:"<<str<<endl;
    int usrdata = stoi(str);
    cout<<"usr month clock time is:"<<usrdata<<endl;
    return usrdata;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

//返回自然月总出勤时间，上班打卡时为0，下班打卡时不为0
int cpppostrequest(int usriddata){
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize Curl" << std::endl;
        return 1;
    }

    // 设置请求URL
    std::string url = "http://192.168.2.5:5000/clockin";

    // 创建JSON对象并设置数据
    json data;
    data["userid"] = usriddata;

    // 将JSON数据转换为字符串
    std::string jsonData = data.dump();

    // 设置Content-Type为JSON
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // 设置Curl选项
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);

    // 创建响应存储变量
    std::string response;

    // 传递response变量作为回调函数的参数
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    // 发送请求并等待响应
    CURLcode res = curl_easy_perform(curl);
    int monthclockintime = 0;
    if (res != CURLE_OK) {
        std::cerr << "Curl request failed: " << curl_easy_strerror(res) << std::endl;
    } else {
        // 提取响应内容
        long statusCode;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
        std::cout << "Response code: " << statusCode << std::endl;
//        std::cout << "Response body: " << response << std::endl;

        // 处理响应数据
        json jsonResponse = json::parse(response);
        // ...
        std::cout << "Response json: " << jsonResponse << std::endl;

        if (jsonResponse.contains("monthclockintime")){
            monthclockintime = jsonResponse["monthclockintime"].get<int>();
        }
        if (jsonResponse.contains("attendtime")){
            int attendtime = jsonResponse["attendtime"].get<int>();
            int hour = attendtime/3600;
            int minute = (attendtime%3600)/60;
            int second = (attendtime%60);
            cout<<"本次出勤时间为："<<hour<<"小时，"<<minute<<"分钟，"<<second<<"秒。"<<endl;
        }
    }
    // 清理Curl句柄
    curl_easy_cleanup(curl);
    return monthclockintime;
}

//实现写数据（月出勤时间）到卡中
void writemonthtimetocard(int monthclockintime,SerialPort mySerialPort){
    UCHAR indata[100];
    UINT i;
    INT block;
    UINT len = 0;
    UCHAR inbyte;
    UCHAR revdata[32];

    for (i = 0; i < 32; i++)
    {
        indata[i] = '0';
    }
    string str = std::to_string(monthclockintime);
    for(int j=0;j<str.length();j++){
        indata[j]=str[j];
    }
    block = 60;
    Cmdwrite[1] = 0x17;
    Cmdwrite[2] = 0xA4;
    Cmdwrite[4] = (UCHAR)block;
    HexStrToByte(&indata[0], &Cmdwrite[6],32);

    CheckSumOut(Cmdwrite, Cmdwrite[1]);

    mySerialPort.WriteData(Cmdwrite, Cmdwrite[1]);  //通过串口发送读数据块指令给读写器
    Sleep(1000); // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
    len = mySerialPort.GetBytesInCOM(); //获取串口缓冲区中字节数

    UINT readbytes = 0;
    do // 获取串口缓冲区数据
    {
        inbyte = 0;
        if (mySerialPort.ReadChar(inbyte) == true)
        {
            revdata[readbytes] = inbyte;
            readbytes++;
        }
    } while (--len);

    if ((revdata[0] = 0x01) && (revdata[1] == 8) && (revdata[1] == readbytes) && (revdata[2] == 0x0A4) && (revdata[3] = 0x20)) //判断是否为写数据返回的数据包
    {
        bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
        if (status)
        {
            if (revdata[4] == 0x00) //写数据块成功
            {
                cout << "写数据到15扇区的数据块" << block << "成功！" << endl << endl;
            }
            else  //写数据块失败
            {
                cout << "读,写数据块失败,失败原因如下：" << endl;
                cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
                cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
                cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块."  << endl;
                cout << "4. 密码控制块不可以读或写." << endl;
            }
        }
    }
}

int main(int argc, _TCHAR* argv[])
{
	UCHAR inbyte;
	UCHAR revdata[32];
	UINT len = 0;
	UINT readbytes;
	UINT i;

    CHAR status;
    UCHAR indata[100];
    INT block;

	SerialPort mySerialPort;
    system("chcp 65001");

    if (!mySerialPort.InitPort(3)) //初始化COM3，并打开COM3
    {
        cout << "初始化COM3失败，请检查读写器端口是否为COM3，或者是否被其它软件打开占用！" << endl;
        cout << "按任意键后，回车退出程序！" << endl;
        cin >> inbyte;
    }
    else
    {
        cout << "初始化COM3成功!" << endl;
//        cout << "输入 “1” 按回车键写数据块" << endl;
//        cout << "输入 “2” 按回车键读数据块" << endl;

        while (true)
        {
//            cin >> inbyte;
            inbyte = '2';
            block = -1;
            status = -1;
            switch (inbyte)
            {
                case '1':
                    cout << "请将IC卡放读写器感应区内，输入要写入数据的块号（比如：1，2，4，5，6，8等）按回车键" << endl;
                    cin >> block;
                    if (block > 0)
                    {
                        cout << "输入要写入的16进制数据（0-F）按回车键" << endl;
                        for (i = 0; i < 32; i++)
                        {
                            indata[i] = '0';
                        }
                        cin >> indata;
                        Cmd[1] = 0x17;
                        Cmd[2] = 0xA4;
                        Cmd[4] = (UCHAR)block;
                        HexStrToByte(&indata[0], &Cmd[6],32);  //将输入的字符转成16进制字节数并拷贝到数组（命令）中
                    }
                    break;
                case '2':
//                    cout << "请将IC卡放读写器感应区内，输入要读的块号（比如：1，2，4，5，6，8等）按回车开始读卡……" << endl;
                    cout<<"程序会等待5秒，请在5秒内放上卡！"<<endl;
                    Sleep(5000);
                    cout<<"等待结束，开始读卡！"<<endl;
//                    cin >> block;
                    block = 56;
                    if (block > 0)
                    {
                        Cmd[1] = 0x08;
                        Cmd[2] = 0xA3;
                        Cmd[4] = (UCHAR)block;
                    }
                    break;
                default:
                    cout << "******输入错误！输入错误！输入错误！******" << endl << endl;
                    cout << "输入 “1” 按回车键写数据块" << endl;
                    cout << "输入 “2” 按回车键读数据块" << endl;
            }
            if (block > 0)
            {
                CheckSumOut(Cmd, Cmd[1]);
//                Sleep(1000);
                mySerialPort.WriteData(Cmd, Cmd[1]);  //通过串口发送读数据块指令给读写器
//                cout<<"data is:"<<mySerialPort.WriteData(Cmd, Cmd[1])<<endl;
                Sleep(1000); // 延时200毫秒等待读写器返回数据，延时太小可能无法接收完整的数据包
                len = mySerialPort.GetBytesInCOM(); //获取串口缓冲区中字节数

                if (len >= 8)
                {
                    readbytes = 0;
                    do // 获取串口缓冲区数据
                    {
                        inbyte = 0;
                        if (mySerialPort.ReadChar(inbyte) == true)
                        {
                            revdata[readbytes] = inbyte;
//                            cout<<"inbyte data:"<<inbyte<<endl;
                            readbytes++;
                        }
                    } while (--len);

//                    UCHAR blockdata1[16];
//                    UCHAR temp1[33];
//                    for (i = 0; i < 16; i++)
//                    {
//                        blockdata1[i] = revdata[12 + i]; //复制数据到数组
//                    }
//                    Hex2Str(&blockdata1[0], &temp1[0], 16); // 数据块数据转换为字符
//                    cout << "读数据块成功，数据块" << block << "数据为：" << temp1 << endl << endl;

                    if ((revdata[0] = 0x01) && (revdata[1] == 8) && (revdata[1] == readbytes) && (revdata[2] == 0x0A4) && (revdata[3] = 0x20)) //判断是否为写数据返回的数据包
                    {
                        bool status = CheckSumIn(revdata, revdata[1]);  //计算校验和
                        if (status)
                        {
                            if (revdata[4] == 0x00) //写数据块成功
                            {
                                cout << "写数据到数据块" << block << "成功！" << endl << endl;
                            }
                            else  //写数据块失败
                            {
                                cout << "读,写数据块失败,失败原因如下：" << endl;
                                cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
                                cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
                                cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块."  << endl;
                                cout << "4. 密码控制块不可以读或写." << endl;
                            }
                        }
                    }
                    if ((revdata[0] = 0x01) && ((revdata[1] == 8) || (revdata[1] == 22)) && (revdata[1] == readbytes) && (revdata[2] == 0xA3) && (revdata[3] = 0x20))//判断是否为读数据块返回的数据包
                    {
                        bool status = CheckSumIn(revdata, revdata[1]); //计算校验和
                        if (status)
                        {
                            if (revdata[4] == 0x00) //读数据块成功
                            {
                                UCHAR blockdata[16];
                                UCHAR temp[33];
                                for (i = 0; i < 16; i++)
                                {
                                    blockdata[i] = revdata[5 + i]; //复制数据到数组
                                }
                                Hex2Str(&blockdata[0], &temp[0], 16); // 数据块数据转换为字符

                                cout << "读数据块成功，数据块" << block << "数据为：" << temp << endl << endl;

                                //                              读取id
                                int usrd = UcharUsrIdtoInt(temp);
                                int monthclockintime = cpppostrequest(usrd);

                                cout<<"monthclockintime:"<<monthclockintime<<endl;
//                              读取月出勤时间
//                                int usrmonthtime = UcharUsrClockTimetoInt(temp);
//                              下班时写数据到卡中
                                if(monthclockintime){
                                    writemonthtimetocard(monthclockintime,mySerialPort);
                                }
                                Sleep(5000);
                            }
                            else //读数据块失败
                            {
                                cout << "读,写数据块失败,失败原因如下：" << endl;
                                cout << "1. 检查IC卡是否放置在读写器的感应区内." << endl;
                                cout << "2. IC卡对应扇区密码与读写器读写密码不一致." << endl;
                                cout << "3. 输入的数据块值超过IC卡的最大数据块数值，比如S50卡有63个数据块." << endl;
                                cout << "4. 密码控制块不可以读或写." << endl;

                                cout<<endl<<"未探测到卡片，请重试！"<<endl;
                                cout<<"程序会暂停10s，等待再次读卡！"<<endl<<endl;
                                Sleep(10000);
                            }
                        }
                    }
                }
                else
                {
                    cout << "读写器超时……，请检查读卡器的连接是否正常！" << endl;
                    while (len > 0) //如果缓冲区中有数据，将缓冲区中数据清空
                    {
                        mySerialPort.ReadChar(inbyte);
                    }
                }
            }
            else
            {
                if (cin.fail())
                {
                    cin.clear();
                    cin.sync();
                    cout << "******输入错误，请输入数字******" << endl << endl;
                    cout << "输入 “1” 按回车键写数据块" << endl;
                    cout << "输入 “2” 按回车键读数据块" << endl;
                }
            }
        }
    }
}

