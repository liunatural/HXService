#pragma once 

#include <cstdlib>
#include <deque>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "boost/asio.hpp"
#include "HXLibMessageQueue.h"
#include "HXLibNetwork.h"
#include "HXLibUtilities.h"

using boost::asio::io_service;
using boost::system::error_code;
using boost::asio::ip::tcp;
class HXLibAsioClient;

//客户端网络响应类
class HXLibClientAccept
{
public:
	HXLibClientAccept(io_service& io_service, tcp::resolver::iterator endpoint_iterator, HXMessageQueueAB* recv, const char* ip, int port);
	void						write(const HXMessagePackage& msg);
	void						close();
	inline	bool				IsConnected() { return m_bConnected; }
	//
	void						closemsg(unsigned short state);
	HXBigMessageQueue			m_bigData;
	int							m_numofWrite;
private:
	void						handle_connect(const error_code& error);
	void						handle_read_header(const error_code& error);
	void						handle_read_body(const error_code& error);
	void						do_write(HXMessagePackage msg);
	void						handle_write(const error_code& error);
	void						do_close();
private:
	HXMessageQueueAB*			m_recv;
	io_service&					io_service_;
	tcp::socket					socket_;
	HXMessagePackage			read_msg_;
	MessagePackageQueue			write_msgs_;
	bool						m_bConnected;
	std::string					m_ip;
	int							m_port;
	friend class HXLibAsioClient;
	HXLibAsioClient*			m_client;
};
class HXLibAsioClient: public HXLibClient
{
public:
	HXLibAsioClient();
	~HXLibAsioClient();
	//链接服务器，提供IP	和端口号
	virtual	int					Start(const char* ip, const char* port);
	//发送消息给服务器端
	virtual	bool				Send(const HXMessagePackage& msg);
	virtual	bool				Send(HXBigMessagePackage& msg);
	//发送消息给服务器端
	virtual	bool				Send(unsigned short id, unsigned short id2, const char* data, unsigned int datalength);
	//关闭与服务器的链接
	virtual	void				Stop();
	//交换一次消息队列中的数据，并且返回给主线程，主线程可以循环调用
	virtual	HXMessageMap&		SwapQueue();
	virtual Statistics&			GetStatistics() { return m_stat; }
	//消息队列
	HXMessageQueueAB			m_queue;
	void						AddSendStat(size_t length);
	virtual double				GetNetDelay() { return m_dNetDelay; }
protected:
	void						OnRecvHeartbeat(double dTime);
	friend class HXLibClientAccept;
protected:
	Statistics					m_stat;
	HXFPSTimer					m_sendFPSBytes, m_sendFPSPacks;
	io_service					m_io_service;
	HXLibClientAccept*			m_client;
	bool						m_bTimeLast;
	double						m_dTimeLast;
	HXLibCritical				m_mutex;
	double						m_dNetDelay;//网络延迟
};
