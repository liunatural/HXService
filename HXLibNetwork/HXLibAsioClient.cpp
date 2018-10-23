#include "HXLibAsioClient.h"

#ifdef _WINDOWS
#include <Sensapi.h>
#endif

namespace boost {
	void throw_exception(std::exception const & e) {
		LOG(LogError, "线程内出错:%s\n", e.what());
	}
};

HXLibClientAccept::HXLibClientAccept(io_service& io_service, tcp::resolver::iterator endpoint_iterator, HXMessageQueueAB* recv
	, const char* ip, int port)
	: io_service_(io_service),
	socket_(io_service)
{
	m_ip = ip;
	m_port = port;
	m_recv = recv;
	m_bConnected = false;
	m_numofWrite = 0;
	async_connect(socket_, endpoint_iterator, boost::bind(&HXLibClientAccept::handle_connect, this,  boost::asio::placeholders::error));
}

void HXLibClientAccept::write(const HXMessagePackage& msg)
{
	io_service_.post(boost::bind(&HXLibClientAccept::do_write, this, msg));
}

void HXLibClientAccept::close()
{
	io_service_.post(boost::bind(&HXLibClientAccept::do_close, this));
	closemsg(HXMessagePackage::Header::Dissconnect_ClosedByMain);
}
void HXLibClientAccept::closemsg(unsigned short state)
{
	HXMessagePackage msg;
	msg.header()->id = HXMessagePackage::Header::ID_Disconnect;
	msg.header()->id2 = state;
	msg.header()->length = 0;
	msg.m_linkid.id = 0;
	m_recv->add(msg);
	LOG(LogDebug, "与服务器断开ip(%s:%d)状态(%d)\n", m_ip.c_str(), m_port, state);
}
void HXLibClientAccept::handle_connect(const error_code& error)
{
	if (!error)
	{
		//无延迟
		//socket_.set_option(asio::ip::tcp::no_delay(true));
		//保持连接状态
		socket_.set_option(boost::asio::socket_base::keep_alive(true));
		LOG(LogDebug, "链接服务器ip(%s:%d)成功\n", m_ip.c_str(), m_port);
		//加入链接成功队列
		HXMessagePackage msg;
		msg.header()->id = HXMessagePackage::Header::ID_Connect;
		msg.header()->id2 = 0;
		msg.header()->length = 0;
		msg.m_linkid.id = 0;
		m_recv->add(msg);
		m_bConnected = true;
		async_read(socket_,  boost::asio::buffer(read_msg_.data(), HXMessagePackage::header_length), boost::bind(&HXLibClientAccept::handle_read_header, this,  boost::asio::placeholders::error));
	}
	else {
		LOG(LogError, "链接服务器ip(%s:%d)出错:%s\n", m_ip.c_str(), m_port, error.message().c_str());
		//加入链接成功队列
		HXMessagePackage msg;
		msg.header()->id = HXMessagePackage::Header::ID_ConnectFailure;
		msg.header()->id2 = 0;
		msg.header()->length = (int)error.message().size();
		strcpy(msg.body(), error.message().c_str());
		msg.m_linkid.id = 0;
		m_recv->add(msg);
	}
}

void HXLibClientAccept::handle_read_header(const error_code& error)
{
	if (!error)
	{
		 boost::asio::async_read(socket_,  boost::asio::buffer(read_msg_.body(), read_msg_.header()->length), 
		 			boost::bind(&HXLibClientAccept::handle_read_body, this,  boost::asio::placeholders::error));
	}
	else
	{
		do_close();
		closemsg(HXMessagePackage::Header::Dissconnect_ReadError);
	}
}

void HXLibClientAccept::handle_read_body(const error_code& error)
{
	if (!error)
	{
		if (m_recv)
		{
			read_msg_.m_linkid.id = 0;
			if (read_msg_.header()->id != HXMessagePackage::Header::ID_Heartbeat)
				m_recv->add(read_msg_, &m_bigData);
			else {
				double dTime = *(double*)read_msg_.body();
				this->m_client->OnRecvHeartbeat(dTime);
			}
		}
		 boost::asio::async_read(socket_,  boost::asio::buffer(read_msg_.data(), HXMessagePackage::header_length), 
		 boost::bind(&HXLibClientAccept::handle_read_header, this,  boost::asio::placeholders::error));
	}
	else
	{
		do_close();
		closemsg(HXMessagePackage::Header::Dissconnect_ReadError);
	}
}

void HXLibClientAccept::do_write(HXMessagePackage msg)
{
	bool write_in_progress = !write_msgs_.empty();
	write_msgs_.push_back(msg);
	m_numofWrite = (int)write_msgs_.size();
	if (!write_in_progress)
	{
		 boost::asio::async_write(socket_,  boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), 
		 boost::bind(&HXLibClientAccept::handle_write, this,  boost::asio::placeholders::error));
	}
}

void HXLibClientAccept::handle_write(const error_code& error)
{
	if (!error)
	{
		write_msgs_.pop_front();
		m_numofWrite = (int)write_msgs_.size();
		if (!write_msgs_.empty())
		{
			 boost::asio::async_write(socket_,  boost::asio::buffer(write_msgs_.front().data(), write_msgs_.front().length()), 
			 boost::bind(&HXLibClientAccept::handle_write, this,  boost::asio::placeholders::error));
		}
	}
	else
	{
		do_close();
		closemsg(HXMessagePackage::Header::Dissconnect_WriteError);
	}
}

void HXLibClientAccept::do_close()
{
	if (!m_bConnected)
		return;
	m_bConnected = false;
	try {
		socket_.close();
	}
	catch (...) {

	}
}
HXLibAsioClient::HXLibAsioClient() {
	m_client = 0;
	m_bTimeLast = false;
	m_queue.m_stat = &m_stat;
	m_dTimeLast = 0;
	m_dNetDelay = 0;
}
HXLibAsioClient::~HXLibAsioClient() {
	Stop();
}
#ifdef _WINDOWS
bool	CheckNetworkAlive()
{
	DWORD   flags;//上网方式
	BOOL    bOnline = TRUE;//是否在线    
	bOnline = IsNetworkAlive(&flags);
	if (bOnline)//在线
	{
		if ((flags & NETWORK_ALIVE_LAN) == NETWORK_ALIVE_LAN)
			LOG(LogDefault, "局域网环境\n");
		else if ((flags & NETWORK_ALIVE_WAN) == NETWORK_ALIVE_WAN)
			LOG(LogDefault, "拨号连接环境\n");
		if ((flags & NETWORK_ALIVE_AOL) == NETWORK_ALIVE_AOL)
			LOG(LogDefault, "代理上网环境\n");
		return true;
	}
	else
		LOG(LogError, "网络不在线：Network Alive Unknown \n");
	return false;
}
#endif
int		HXLibAsioClient::Start(const char* ip, const char* port)
{
#ifdef _WINDOWS
	if (!CheckNetworkAlive())
		return 2;
#endif
	if (m_client) {
		LOG(LogError, "已经连接服务器，不可再连\n");
		return 1;
	}
	m_bTimeLast = false;
	tcp::resolver resolver(m_io_service);
	tcp::resolver::query query(ip, port);
	tcp::resolver::iterator iterator = resolver.resolve(query);
	LOG(LogDefault, "正在连接服务器\n");
	m_client = new HXLibClientAccept(m_io_service, iterator, &m_queue, ip, atoi(port));
	m_client->m_client = this;
	boost::thread t(boost::bind(&io_service::run, &m_io_service));
	t.timed_join(boost::posix_time::seconds(0));
	return 0;
}
bool	HXLibAsioClient::Send(const HXMessagePackage& msg) {
	if (m_client && m_client->IsConnected())
	{
		AddSendStat(msg.length());
		m_client->write(msg);
		return true;
	}
	return false;
}
void				HXLibAsioClient::OnRecvHeartbeat(double dTime) {
	HXLibCritical::Lock l(m_mutex);
	m_bTimeLast = false;
	m_dNetDelay = (GetTimer()->GetTickTimer() - dTime)*0.5;
}
HXMessageMap&		HXLibAsioClient::SwapQueue()// { return m_queue.swap(); }
{
	if (m_client && m_client->IsConnected() && !m_bTimeLast) {
		double dTime = GetTimer()->GetTickTimer();
		if (dTime > (m_dTimeLast + 0.5f)) {
			m_dTimeLast = dTime;
			{
				HXLibCritical::Lock l(m_mutex);
				m_bTimeLast = true;
			}
			HXMessagePackage msg;
			msg.header()->id = HXMessagePackage::Header::ID_Heartbeat;
			msg.header()->id2 = 0;
			msg.header()->length = sizeof(double);
			memcpy(msg.body(), &m_dTimeLast, sizeof(double));
			m_client->write(msg);
		}
		//boost::posix_time::ptime time_now;
		//boost::posix_time::millisec_posix_time_system_config::time_duration_type time_elapse;
		//time_now = boost::posix_time::microsec_clock::universal_time();
		//if (m_bTimeLast) {
		//	time_elapse = time_now - m_time_last;
		//	//每隔一秒发送一次心跳包
		//	if (time_elapse.total_seconds() > 0) {

		//		Send(HXMessagePackage::Header::ID_Heartbeat, 0, 0, 0);
		//		m_time_last = time_now;
		//		//LOG(LogDebug, "Send Heartbeat\n");
		//	}
		//}
		//else {
		//	m_bTimeLast = true;
		//	m_time_last = time_now;
		//}
	}
	return m_queue.swap();
}
void	HXLibAsioClient::AddSendStat(size_t length)
{
	m_stat.sendBytes += length;
	m_stat.sendPackages++;
	m_sendFPSBytes.Add(length);
	m_sendFPSPacks.Add(1.0);
	m_stat.sendBytesPerSecond = m_sendFPSBytes.GetFPS();
	m_stat.sendBytesPerSecondMax = m_sendFPSBytes.GetFPSMax();
	m_stat.sendBytesPerSecondAvg = m_sendFPSBytes.GetFPSAvg();
	m_stat.sendPackagesPerSecond = (unsigned int)m_sendFPSPacks.GetFPS();
	m_stat.sendPackagesPerSecondMax = (unsigned int)m_sendFPSPacks.GetFPSMax();
	m_stat.sendPackagesPerSecondAvg = (unsigned int)m_sendFPSPacks.GetFPSAvg();
}

bool	HXLibAsioClient::Send(HXBigMessagePackage& msg)
{
	if (m_client && m_client->IsConnected())
	{
		if (msg.IsAllowLost()) {
			unsigned int allocMemory = m_client->m_numofWrite * sizeof(HXMessagePackage);
			LOG(LogDebug, "IsAllowLost(%.03f/%.03f)\n", (double)allocMemory/(1024.0*1024.0), (double)msg.GetAllowMaxOfMemory() / (1024.0*1024.0));
			if (allocMemory >= msg.GetAllowMaxOfMemory()) {
				LOG(LogDefault, "Allow Lost(%.03f/%.03f)\n", (double)allocMemory / (1024.0*1024.0), (double)msg.GetAllowMaxOfMemory() / (1024.0*1024.0));
				return false;
			}
		}
		for (int i = 0; i < msg.GetCount(); i++) {
			m_client->write(*msg.GetPackage(i));
			AddSendStat(msg.GetPackage(i)->length());
		}
		return true;
	}
	return false;
}
bool	HXLibAsioClient::Send(unsigned short id, unsigned short id2, const char* data, unsigned int datalength)
{
	if (m_client && m_client->IsConnected())
	{
		HXMessagePackage msg;
		msg.header()->id = id;
		msg.header()->id2 = id2;
		msg.header()->length = datalength;
		if (datalength > 0)
			memcpy(msg.body(), data, datalength);
		m_client->write(msg);
		AddSendStat(msg.length());
		return true;
	}
	return false;
}
void	HXLibAsioClient::Stop() {
	if (m_client) {
		m_io_service.stop();
		m_client->close();
		delete m_client;
		m_client = 0;
	}
}

HXLIBNETWORK_API HXLibClient*	CreateNewClient(void)
{
	return new HXLibAsioClient();
}
