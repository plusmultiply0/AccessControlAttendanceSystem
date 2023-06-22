#include "stdafx.h"
#include "SerialPort.h"
#include <process.h>  
#include <iostream> 

bool SerialPort::s_bExit = false;
const UINT SLEEP_TIME_INTERVAL = 5;

SerialPort::SerialPort(void)
//: m_hListenThread(INVALID_HANDLE_VALUE)
{
	m_hComm = INVALID_HANDLE_VALUE;
	//m_hListenThread = INVALID_HANDLE_VALUE;

	InitializeCriticalSection(&m_csCommunicationSync);
}
SerialPort::~SerialPort()
{
	//CloseListenTread();
	ClosePort();
	DeleteCriticalSection(&m_csCommunicationSync);
}
bool SerialPort::InitPort(UINT portNo /*= 1*/, UINT baud /*= CBR_9600*/, char parity /*= 'N'*/,
	UINT databits /*= 8*/, UINT stopsbits /*= 1*/, DWORD dwCommEvents /*= EV_RXCHAR*/)
{
	/** 临时变量,将制定参数转化为字符串形式,以构造DCB结构 */
	CHAR szDCBparam[50];
	sprintf_s(szDCBparam, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopsbits);
	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
//    std::cout<<openPort(portNo)<<' '<<portNo<<std::endl;
	if (!openPort(portNo))
	{
//        std::cout<<openPort(portNo)<<' '<<std::endl;
		return false;
	}
	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);
	/** 是否有错误发生 */
	BOOL bIsSuccess = TRUE;
	/** 在此可以设置输入输出的缓冲区大小,如果不设置,则系统会设置默认值.
	*  自己设置缓冲区大小时,要注意设置稍大一些,避免缓冲区溢出
	*/
	/*if (bIsSuccess )
	{
	bIsSuccess = SetupComm(m_hComm,10,10);
	}*/
	/** 设置串口的超时时间,均设为0,表示不使用超时限制 */
	COMMTIMEOUTS  CommTimeouts;
	CommTimeouts.ReadIntervalTimeout = 0;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadTotalTimeoutConstant = 0;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	CommTimeouts.WriteTotalTimeoutConstant = 0;
	if (bIsSuccess)
	{
		bIsSuccess = SetCommTimeouts(m_hComm, &CommTimeouts);
//        std::cout<<"status0:"<<' '<<bIsSuccess<<std::endl;
	}

	DCB  dcb;
//    std::cout<<"status1:"<<' '<<bIsSuccess<<std::endl;
	if (bIsSuccess)
	{
		 //将ANSI字符串转换为UNICODE字符串  
		DWORD dwNum = MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, NULL, 0);

        wchar_t *pwText = new wchar_t[dwNum];
		if (!MultiByteToWideChar(CP_ACP, 0, szDCBparam, -1, pwText, dwNum))
		{
			bIsSuccess = TRUE;
		}
		// 获取当前串口配置参数,并且构造串口DCB参数 
		bIsSuccess = GetCommState(m_hComm, &dcb)&&BuildCommDCB(szDCBparam, &dcb);
//        std::cout<<"two status:"<<GetCommState(m_hComm, &dcb)<<' '<<BuildCommDCB(reinterpret_cast<LPCSTR>(pwText), &dcb)<<std::endl;
		//开启RTS flow控制 
		//dcb.fRtsControl = RTS_CONTROL_DISABLE;
		// 释放内存空间
		delete[] pwText;
	}
//    std::cout<<"status2:"<<' '<<bIsSuccess<<std::endl;
	if (bIsSuccess)
	{
		// 使用DCB参数配置串口状态 
		bIsSuccess = SetCommState(m_hComm, &dcb);
	}
	// 清空串口缓冲区
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	// 离开临界段 
	LeaveCriticalSection(&m_csCommunicationSync);
//    std::cout<<"status3:"<<' '<<bIsSuccess<<std::endl;
	return bIsSuccess == TRUE;
}
bool SerialPort::InitPort(UINT portNo, const LPDCB& plDCB)
{
	/** 打开指定串口,该函数内部已经有临界区保护,上面请不要加保护 */
	if (!openPort(portNo))
	{
//        std::cout<<"here 1"<<std::endl;
		return false;
	}
	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);
	/** 配置串口参数 */
	if (!SetCommState(m_hComm, plDCB))
	{
//        std::cout<<"here 2"<<std::endl;
		return false;
	}
	/**  清空串口缓冲区 */
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);
	/** 离开临界段 */
	LeaveCriticalSection(&m_csCommunicationSync);
	return true;
}
void SerialPort::ClosePort()
{
	/** 如果有串口被打开，关闭它 */
	if (m_hComm != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hComm);
		m_hComm = INVALID_HANDLE_VALUE;
	}
}
bool SerialPort::openPort(UINT portNo)
{
	/** 进入临界段 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 把串口的编号转换为设备名 */
	char szPort[50];
	sprintf_s(szPort, "COM%d", portNo);

	/** 打开指定的串口 */
	m_hComm = CreateFileA(szPort,  /** 设备名,COM1,COM2等 */
		GENERIC_READ | GENERIC_WRITE, /** 访问模式,可同时读写 */
		0,                            /** 共享模式,0表示不共享 */
		NULL,                         /** 安全性设置,一般使用NULL */
		OPEN_EXISTING,                /** 该参数表示设备必须存在,否则创建失败 */
		0,
		0);

	/** 如果打开失败，释放资源并返回 */
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}
	/** 退出临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);
	return true;
}


UINT SerialPort::GetBytesInCOM()
{
	DWORD dwError = 0;  /** 错误码 */
	COMSTAT  comstat;   /** COMSTAT结构体,记录通信设备的状态信息 */
	memset(&comstat, 0, sizeof(COMSTAT));

	UINT BytesInQue = 0;
	/** 在调用ReadFile和WriteFile之前,通过本函数清除以前遗留的错误标志 */
	if (ClearCommError(m_hComm, &dwError, &comstat))
	{
		BytesInQue = comstat.cbInQue; /** 获取在输入缓冲区中的字节数 */
	}
	return BytesInQue;
}
bool SerialPort::ReadChar(UCHAR &cRecved)
{
	BOOL  bResult = TRUE;
	DWORD BytesRead = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);

	/** 从缓冲区读取一个字节的数据 */
	bResult = ReadFile(m_hComm, &cRecved, 1, &BytesRead, NULL);
	if ((!bResult))
	{
		/** 获取错误码,可以根据该错误码查出错误原因 */
		DWORD dwError = GetLastError();

		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);

		return false;
	}

	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return (BytesRead == 1);

}

bool SerialPort::WriteData(unsigned char* pData, unsigned int length)
{
	BOOL   bResult = TRUE;
	DWORD  BytesToSend = 0;
	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	/** 临界区保护 */
	EnterCriticalSection(&m_csCommunicationSync);
	/** 向缓冲区写入指定量的数据 */
	bResult = WriteFile(m_hComm, pData, length, &BytesToSend, NULL);
	if (!bResult)
	{
		DWORD dwError = GetLastError();
		/** 清空串口缓冲区 */
		PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_RXABORT);
		LeaveCriticalSection(&m_csCommunicationSync);
		return false;
	}
	/** 离开临界区 */
	LeaveCriticalSection(&m_csCommunicationSync);

	return true;
}