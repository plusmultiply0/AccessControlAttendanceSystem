#ifndef SERIALPORT_H_  
#define SERIALPORT_H_ 
#include <Windows.h>  

#pragma once
class SerialPort
{
	public:
		SerialPort();
		~SerialPort();
	public:
		bool InitPort(UINT  portNo = 2, UINT  baud = CBR_9600, char  parity = 'N', UINT  databits = 8, UINT  stopsbits = 1, DWORD dwCommEvents = EV_RXCHAR);
		bool InitPort(UINT  portNo, const LPDCB& plDCB);
		//bool OpenListenThread();
		//bool CloseListenTread();
		bool WriteData(unsigned char* pData, unsigned int length);
		UINT GetBytesInCOM();
		bool ReadChar(UCHAR &cRecved);
	private:
		bool openPort(UINT  portNo);
		void ClosePort();
		//static UINT WINAPI ListenThread(void* pParam);
	private:
		HANDLE  m_hComm; //串口句柄 
		static bool s_bExit; //线程退出标志变量 
		//volatile HANDLE    m_hListenThread;	//线程句柄 
		CRITICAL_SECTION   m_csCommunicationSync; //同步互斥,临界区保护,!< 互斥操作串口  
};
#endif



