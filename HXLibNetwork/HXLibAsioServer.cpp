//
// HXLibServerAccept.cpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2016 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "HXLibAsioServer.h"


HXLibAccept::HXLibAccept(io_service& io_service, HXLibAsioServer* server)
	: socket_(io_service)
{
	m_server = server;
	m_numofWrite = 0;
}
HXLibAccept::~HXLibAccept()
{
	//socket_.close();
	LOG(LogDebug, "用户连接池数据释放linkid(%d)\n", m_linkid.id);
}

tcp::socket& HXLibAccept::socket()
{
	return socket_;
}
void HXLibAccept::close()
{
	socket_.close();
}
void HXLibAccept::start()
{
	boost::asio::ip::address clientaddr;
	clientaddr = socket_.remote_endpoint().address();
	m_linkid = m_server->Join(shared_from_this(), clientaddr.to_string().c_str());
	//
	//如果没有足够的链接支撑的话断开
	if (m_linkid.id == 0) {
		socket_.close();
		return;
	}
	boost::asio::async_read(socket_,  boost::asio::buffer(read_msg_.data(), HXMessagePackage::header_length), 
					boost::bind(&HXLibAccept::handle_read_header, shared_from_this(),  boost::asio::placeholders::error));
}
void HXLibAccept::send(const HXMessagePackage& msg)
{
	if (m_server) {
		m_server->AddSendStat(msg.length());
	}
	boost::mutex::scoped_lock lock(m_mutex);
	if (!socket_.is_open())
		return;
	if (write_msgs_.empty()) {
		write_msgs_.push_back(msg);
		m_numofWrite = (int)write_msgs_.size();
		boost::asio::async_write(socket_, boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), 
		boost::bind(&HXLibAccept::handle_write, shared_from_this(), boost::asio::placeholders::error));
	}
	else
		write_msgs_.push_back(msg);
}
void HXLibAccept::handle_read_header(const error_code& error)
{
	if (!error)
	{
		boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.body(), read_msg_.header()->length), 
						boost::bind(&HXLibAccept::handle_read_body, shared_from_this(), boost::asio::placeholders::error));
	}
	else
	{
		m_server->Leave(shared_from_this(), HXMessagePackage::Header::Dissconnect_ReadError);
	}
}

void HXLibAccept::handle_read_body(const error_code& error)
{
	if (!error)
	{
		if (m_server) {
			read_msg_.linkid(m_linkid);
			//只处理非心跳包
			if (read_msg_.header()->id != HXMessagePackage::Header::ID_Heartbeat)
				m_server->m_queue.add(read_msg_, &m_bigData);
			else {
				double dTime = *(double*)read_msg_.body();
				HXMessagePackage msg;
				msg.header()->id = HXMessagePackage::Header::ID_Heartbeat;
				msg.header()->id2 = 0;
				msg.header()->length = sizeof(double);
				memcpy(msg.body(), &dTime, sizeof(double));
				send(msg);
				//LOG(LogDebug, "Recv Heartbeat\n");
			}
		}
		boost::asio::async_read(socket_, boost::asio::buffer(read_msg_.data(), HXMessagePackage::header_length), 
				boost::bind(&HXLibAccept::handle_read_header, shared_from_this(), boost::asio::placeholders::error));
	}
	else
	{
		m_server->Leave(shared_from_this(), HXMessagePackage::Header::Dissconnect_ReadError);
	}
}

void HXLibAccept::handle_write(const error_code& error)
{
	if (!error)
	{
		//继续发送队列中的消息直到发送完毕
		HXMessagePackage front;
		bool bSendMessage = false;
		{
			if (m_server) {
				m_server->AddSendCompleteStat();
			}
			boost::mutex::scoped_lock lock(m_mutex);
			write_msgs_.pop_front();
			m_numofWrite = (int)write_msgs_.size();
			if (!write_msgs_.empty()) {
				memcpy(&front, &write_msgs_.front(), sizeof(HXMessagePackage));
				bSendMessage = true;
			}
		}
		if (bSendMessage) {
			boost::asio::async_write(socket_, boost::asio::buffer(front.data(), front.length()), 
						boost::bind(&HXLibAccept::handle_write, shared_from_this(), boost::asio::placeholders::error));
			//m_server->AddSendComplete();
		}
	}
	else
	{
		m_server->Leave(shared_from_this(), HXMessagePackage::Header::Dissconnect_WriteError);
	}
}

HXLibServerAccept::HXLibServerAccept(io_service& io_service, const tcp::endpoint& endpoint, HXLibAsioServer* server)
	: io_service_(io_service),
	acceptor_(io_service, endpoint)
{
	m_server = server;
	start_accept();
}

void HXLibServerAccept::start_accept()
{
	HXLibAcceptPtr new_session(new HXLibAccept(io_service_, m_server));
	acceptor_.async_accept(new_session->socket(), boost::bind(&HXLibServerAccept::handle_accept, this, new_session, boost::asio::placeholders::error));
}

void HXLibServerAccept::handle_accept(HXLibAcceptPtr session, const error_code& error)
{
	if (!error)
	{
		session->start();
	}
	start_accept();
}
void HXLibServerAccept::stop_accept()
{
	acceptor_.close();
}
HXLibAsioServer::HXLibAsioServer() {
	m_maxLinks = MAX_ACCEPT_ITEM;
	m_port = 12345;
	m_server = 0;
	m_message = new HXMessagePackage();
	m_id = 0;
	m_onlineLinks = 0;
	m_queue.m_stat = &m_stat;
}
HXLibAsioServer::~HXLibAsioServer() {
	if (m_message)
		delete m_message;
}
void	HXLibAsioServer::OnService()
{
	try
	{
		using namespace std; // For atoi.
		tcp::endpoint endpoint(tcp::v4(), m_port);// atoi(argv[i]));
		m_server = new HXLibServerAccept(m_io_service, endpoint, this);
		m_io_service.run();
	}
	catch (std::exception& e)
	{
		LOG(LogError, "Exception: %s\n", e.what());
	}
}
bool	HXLibAsioServer::Start(int port, int maxLinks)
{
	if (m_server)
		return false;
	m_maxLinks = maxLinks;
	if (m_maxLinks < 0 || m_maxLinks >= MAX_ACCEPT_ITEM)
	{
		LOG(LogError, "maxLinks参数错误，必须得是0到%d之间\n", MAX_ACCEPT_ITEM);
		return false;
	}
	m_port = port;
	boost::function0< void> f = boost::bind(&HXLibAsioServer::OnService, this);
	boost::thread thrd(f);
	thrd.timed_join(boost::posix_time::seconds(0));
	LOG(LogDebug, "开启一个新的服务，端口号是(%d)最大链接(%d)\n", this->m_port, maxLinks);
	return true;
}
void	HXLibAsioServer::Stop() {
	if (m_server) {
		LOG(LogDebug, "端口号是(%d)的服务关闭了\n", m_port);
		m_io_service.stop();
		m_server->stop_accept();
		delete m_server;
		m_server = 0;
	}
}
LinkID			HXLibAsioServer::Join(HXLibAcceptPtr link, const char* ip)
{
	LinkID linkid;
	{
		boost::mutex::scoped_lock lock(m_linkMutex);
		if (m_id >= (unsigned short)MAXSHORT) {
			LOG(LogDebug, "ID空间用完了，重新从0开始\n");
			m_id = 0;
		}
		m_id++;
		for (int i = 0; i < m_maxLinks; i++) {
			if (m_links[i].id.id == 0) {
				m_links[i].link = link;
				m_links[i].id.sid = m_id;
				m_links[i].id.sindex = i;
				memset(m_links[i].ip, 0, sizeof(m_links[i].ip));
				strncpy(m_links[i].ip, ip, 31);
				linkid = m_links[i].id;
				break;
			}
		}
		if (linkid.id == 0) {
			LOG(LogError, "连接池链接在线用户数已经达到上限(%d/%d)ip(%s)\n", m_onlineLinks, m_maxLinks, ip);
			return linkid;
		}
		else {
			m_onlineLinks++;
			LOG(LogDebug, "新用户加入linkid(%d),ip(%s),在线(%d/%d)\n", linkid.id, ip, m_onlineLinks, m_maxLinks);
		}
	}
	//用户链接数据包压入到客户端
	if (linkid.id != 0)
	{
		HXMessagePackage msg;
		msg.header()->id = HXMessagePackage::Header::ID_Connect;
		msg.header()->id2 = 0;
		msg.header()->length = (int)strlen(ip)+1;
		memcpy(msg.body(), ip, strlen(ip) + 1);
		msg.linkid(linkid);
		m_queue.add(msg);
	}
	return linkid;
}
void					HXLibAsioServer::Leave(HXLibAcceptPtr link, unsigned short dissconnectState)
{
	LinkID linkid;
	{
		boost::mutex::scoped_lock lock(m_linkMutex);
		linkid = link->GetLinkID();
		link->ResetLinkID();
		if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
		{
			if(m_links[linkid.sindex].link)
				m_onlineLinks--;
			m_links[linkid.sindex].id.id = 0;
			m_links[linkid.sindex].link = nullptr;
			LOG(LogDebug, "用户退出linkid(%d),ip(%s)状态(%d)在线(%d/%d)\n", linkid.id, m_links[linkid.sindex].ip, dissconnectState, m_onlineLinks, m_maxLinks);
		}
	}
	//用户退出数据包压入到客户端
	if(linkid.id != 0)
	{
		HXMessagePackage msg;
		msg.header()->id = HXMessagePackage::Header::ID_Disconnect;
		msg.header()->id2 = dissconnectState;
		msg.header()->length = 0;
		msg.linkid(linkid);
		m_queue.add(msg);
	}
}
void	HXLibAsioServer::AddSendCompleteStat()
{
	boost::mutex::scoped_lock lock(m_statMutex);
	m_stat.sendPackagesComplete++;
	if (m_stat.sendPackagesComplete > m_stat.sendPackages)
		m_stat.sendPackagesComplete = m_stat.sendPackages;
}
void	HXLibAsioServer::AddSendStat(size_t length)
{
	boost::mutex::scoped_lock lock(m_statMutex);
	m_stat.sendBytes += length;
	m_stat.sendPackages++;
	m_sendFPSBytes.Add(length);
	m_sendFPSPacks.Add(1.0f);
	m_stat.sendBytesPerSecond = m_sendFPSBytes.GetFPS();
	m_stat.sendBytesPerSecondMax = m_sendFPSBytes.GetFPSMax();
	m_stat.sendBytesPerSecondAvg = m_sendFPSBytes.GetFPSAvg();
	m_stat.sendPackagesPerSecond = (unsigned int)m_sendFPSPacks.GetFPS();
	m_stat.sendPackagesPerSecondMax = (unsigned int)m_sendFPSPacks.GetFPSMax();
	m_stat.sendPackagesPerSecondAvg = (unsigned int)m_sendFPSPacks.GetFPSAvg();
}
bool	HXLibAsioServer::Send(LinkID linkid, HXBigMessagePackage& msg)
{
	boost::mutex::scoped_lock lock(m_linkMutex);
	if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
	{
		if (msg.IsAllowLost()) {
			unsigned int allocMemory = m_links[linkid.sindex].link->m_numofWrite * sizeof(HXMessagePackage);
			if (allocMemory >= msg.GetAllowMaxOfMemory()) {
				LOG(LogDefault, "Server: Allow Lost(%.03f/%.03f)\n", (double)allocMemory / (1024.0*1024.0), (double)msg.GetAllowMaxOfMemory() / (1024.0*1024.0));
				return false;
			}
		}
		for (int i = 0; i < msg.GetCount(); i++) {
			m_links[linkid.sindex].link->send(*msg.GetPackage(i));
		}
		return true;
	}
	return false;
}
bool	HXLibAsioServer::Send(LinkID linkid, const HXMessagePackage& msg)
{
	boost::mutex::scoped_lock lock(m_linkMutex);
	if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
	{
		m_links[linkid.sindex].link->send(msg);
		return true;
	}
	return false;
}

bool	HXLibAsioServer::Send(LinkID linkid, unsigned short id, unsigned short id2, const char* data, unsigned int datalength)
{
	boost::mutex::scoped_lock lock(m_linkMutex);
	if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
	{
		HXMessagePackage msg;
		msg.header()->id = id;
		msg.header()->id2 = id2;
		if (!data)
			datalength = 0;
		msg.header()->length = datalength;
		if (datalength > 0)
			memcpy(msg.body(), data, datalength);
		m_links[linkid.sindex].link->send(msg);
		return true;
	}
	return false;
}
void	HXLibAsioServer::Close(LinkID linkid)
{
	HXLibAcceptPtr temp = nullptr;
	{
		boost::mutex::scoped_lock lock(m_linkMutex);
		if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid) {
			temp = m_links[linkid.sindex].link;
			temp->close();
		}
	}
	if (temp != nullptr) {
		//Leave(temp, HXMessagePackage::Header::Dissconnect_ClosedByMain);
		//temp->close();
	}
}
void	HXLibAsioServer::SetContext(LinkID linkid, const char* context)
{
	boost::mutex::scoped_lock lock(m_linkMutex);
	if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
	{
		m_links[linkid.sindex].context = context;
	}
}
//获取连接器参数
bool	HXLibAsioServer::GetContext(LinkID linkid, const char** result)
{
	boost::mutex::scoped_lock lock(m_linkMutex);
	if (linkid.sindex < m_maxLinks && linkid.sid == m_links[linkid.sindex].id.sid)
	{
		if(result)
			*result = m_links[linkid.sindex].context;
		return true;
	}
	return false;
}

HXLIBNETWORK_API HXLibService*	CreateNewService(void)
{
	return new HXLibAsioServer();
}

