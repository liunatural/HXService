#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "HXLibMessageQueue.h"
#include <boost/thread/mutex.hpp>
#include <boost/make_unique.hpp>
#include "HXLibNetwork.h"
#include "HXLibUtilities.h"

class HXLibAsioServer;

using boost::asio::io_service;
using boost::system::error_code;
using boost::asio::ip::tcp;

//----------------------------------------------------------------------

typedef std::deque<HXMessagePackage> MessagePackageQueue;


class HXLibAccept : public boost::enable_shared_from_this<HXLibAccept>
{
public:
	HXLibAccept(io_service& io_service, HXLibAsioServer* server);
	~HXLibAccept();
	tcp::socket& socket();
	void					start();
	void					close();
	void					send(const HXMessagePackage& msg);
	void					handle_read_header(const error_code& error);
	void					handle_read_body(const error_code& error);
	void					handle_write(const error_code& error);
	LinkID					GetLinkID() { return m_linkid; }
	void					ResetLinkID() { m_linkid.id = 0; }
	HXBigMessageQueue		m_bigData;
	int						m_numofWrite;
private:
	LinkID					m_linkid;
	HXLibAsioServer*		m_server;
	tcp::socket				socket_;
	HXMessagePackage		read_msg_;
	MessagePackageQueue		write_msgs_;
	boost::mutex			m_mutex;
};

typedef boost::shared_ptr<HXLibAccept> HXLibAcceptPtr;

//----------------------------------------------------------------------

class HXLibServerAccept
{
public:
	HXLibServerAccept(io_service& io_service, const tcp::endpoint& endpoint, HXLibAsioServer* server);
	void	start_accept();
	void	handle_accept(HXLibAcceptPtr session, const error_code& error);
	void	stop_accept();
private:
	HXLibAsioServer*	m_server;
	io_service&			io_service_;
	tcp::acceptor		acceptor_;
};

//----------------------------------------------------------------------
class HXLibAsioServer: public HXLibService
{
public:
	HXLibAsioServer();
	~HXLibAsioServer();

	//开启传输服务
	bool					Start(int port, int maxLinks);
	//关闭传输服务
	void					Stop();
	//发送消息
	bool					Send(LinkID linkid, const HXMessagePackage& msg);
	bool					Send(LinkID linkid, HXBigMessagePackage& msg);
	bool					Send(LinkID linkid, unsigned short id, unsigned short id2, const char* data, unsigned int datalength);
	void					Close(LinkID linkid);
	//设置链接器参数
	void					SetContext(LinkID linkid, const char* context);
	//获取连接器参数
	bool					GetContext(LinkID linkid, const char** result);
;
	virtual Statistics&		GetStatistics() { return m_stat; }
	bool					IsRunning() { return (m_server != 0); }
	//
	virtual	HXMessageQueue&	SwapQueue() { return m_queue.swap(); }
	HXMessageQueueAB		m_queue;
	//
	void					AddSendStat(size_t length);
	void					AddSendCompleteStat();
public://
protected:
	//连接池管理
	struct AcceptItem {
		HXLibAcceptPtr		link;
		LinkID				id;
		const char*			context;
		char				ip[32];
	};
	boost::mutex			m_linkMutex;
	boost::mutex			m_statMutex;
	AcceptItem				m_links[MAX_ACCEPT_ITEM];
	int						m_onlineLinks;
	int						m_maxLinks;
	//加入服务队列中，并返回一个linkid
	LinkID					Join(HXLibAcceptPtr link, const char* ip);
	void					Leave(HXLibAcceptPtr link, unsigned short dissconnectState);
	//
	void					OnService();
	friend class			HXLibAccept;
protected:
	io_service				m_io_service;
	HXLibServerAccept*		m_server;
	int						m_port;
	HXMessagePackage*		m_message;
	unsigned int			m_id;
	//
	Statistics				m_stat;
	HXFPSTimer				m_sendFPSBytes;
	HXFPSTimer				m_sendFPSPacks;
};
