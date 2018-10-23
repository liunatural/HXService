#pragma once

#include "HXLibNetwork.h"
#include <boost/thread/mutex.hpp>
#include <boost/make_unique.hpp>
#include <vector>
#include "HXLibUtilities.h"

typedef std::deque<HXMessagePackage> MessagePackageQueue;

#pragma pack(push, 1)
struct MBigHeader
{
	unsigned char	zipLevel;
	unsigned short	sectionLength;//数据块大小
	unsigned short	id, id2;
	unsigned int	dataLength;//压缩后整个数据大小
	unsigned int	originalLength;//原来数据大小
	unsigned short	sectionCount;//数据块数量
};
#pragma pack(pop)

//使用queue的队列，暂时先用这个
class HXMessageQueue : public HXMessageMap
{
public:
	MessagePackageQueue		m_queue;
	//返回一个消息指针
	const HXMessagePackage*	GetPackage(unsigned int index);
	//返回消息包数量
	int						GetCount();
	void					RemoveFront();
	void					Add(const HXMessagePackage& msg);
	void					Reset();
};

//大的数据包消息队列
class HXBigMessageQueue : public HXBigMessagePackage
{
public:
	HXBigMessageQueue();
	//返回一个消息指针
	const HXMessagePackage*		GetPackage(unsigned int index);
	//返回消息包数量
	int							GetCount();
	//添加一个消息
	virtual	void				AddPackage(const HXMessagePackage& pack);
	//添加一个大消息块
	virtual	void				SetBigPackage(unsigned short id, unsigned short id2, const void* ptr, int dataLength, ZipLevel zipLevel = ZipLevel_None);
	//接收一个消息，如果返回true表示整个数据块接收完毕
	bool						RecvPackage(const HXMessagePackage& pack);
	//读取大的完整数据包
	const MBigHeader&			GetBigPackage(std::vector<char>& result);
	virtual	void				SetAllowLostPackage(bool bAllowLost, unsigned int maxOfMemory);
	//是否允许丢帧
	virtual	bool				IsAllowLost() { return m_bAllowLost; }
	//返回最大缓存大小
	virtual unsigned int		GetAllowMaxOfMemory() { return m_maxOfMemory; }
	//
	bool						m_bAllowLost;
	unsigned int				m_maxOfMemory;
	//
	MessagePackageQueue			m_queue;
	MBigHeader					m_header;
	std::vector<char>			m_recvData;
};

class HXMessageQueueAB: public HXLibMessageQueueAB
{
public:
	HXMessageQueueAB();
	~HXMessageQueueAB();
	//添加消息
	void						add(const HXMessagePackage& msg, HXBigMessageQueue* bigData = 0);
	//交换当前消息队列
	HXMessageQueue&				swap();
	//返回当前主线程可以使用的队列
	HXMessageQueue&				get();
	//
	virtual void				Push(const HXMessagePackage& msg) { add(msg, 0); }
	//交换当前消息队列
	virtual HXMessageMap&		Swap() { return swap(); }
	//返回当前主线程可以使用的队列
	virtual HXMessageMap&		Get() { return get(); }
	//
	Statistics*					m_stat;
	HXFPSTimer					m_recvFPSBytes, m_recvFPSPacks;
protected:
	//
	HXMessageQueue				m_queue[2];
	int							m_current;
	int							m_queueMax;
	boost::mutex				m_mutex;
};
