#include "HXLibMessageQueue.h"
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include "zlib.h"

const HXMessagePackage*	HXMessageQueue::GetPackage(unsigned int index)
{
	if (index >= m_queue.size())
		return 0;
	return &m_queue[index];
}
//返回消息包数量
int			HXMessageQueue::GetCount() {
	return (int)m_queue.size();
}
void		HXMessageQueue::RemoveFront() {
	if (m_queue.size() > 0) {
		if (m_queue[0].bigDataPointer)
			delete[] m_queue[0].bigDataPointer;
		m_queue[0].bigDataPointer = 0;
	}
	m_queue.pop_front();
}
void		HXMessageQueue::Add(const HXMessagePackage& msg) {
	m_queue.push_back(msg);
}
void		HXMessageQueue::Reset() {
	for (int i = 0; i < m_queue.size(); i++) {
		if (m_queue[i].bigDataPointer) {
			delete[] m_queue[i].bigDataPointer;
			m_queue[i].bigDataPointer = 0;
		}
	}
	m_queue.clear();
}

HXMessageQueueAB::HXMessageQueueAB() {
	m_current = 0;
	m_queueMax = 8162;
	m_stat = 0;
}
HXMessageQueueAB::~HXMessageQueueAB() {

}
//添加消息
void				HXMessageQueueAB::add(const HXMessagePackage& msg, HXBigMessageQueue* bigData)
{
	boost::mutex::scoped_lock lock(m_mutex);
	if (m_stat) {
		m_stat->recvPackages++;
		m_stat->recvBytes+= msg.length();
		m_recvFPSBytes.Add(msg.length());
		m_recvFPSPacks.Add(1.0);
		m_stat->recvBytesPerSecond = m_recvFPSBytes.GetFPS();
		m_stat->recvBytesPerSecondMax = m_recvFPSBytes.GetFPSMax();
		m_stat->recvBytesPerSecondAvg = m_recvFPSBytes.GetFPSAvg();
		m_stat->recvPackagesPerSecond = (unsigned int)m_recvFPSPacks.GetFPS();
		m_stat->recvPackagesPerSecondMax = (unsigned int)m_recvFPSPacks.GetFPSMax();
		m_stat->recvPackagesPerSecondAvg = (unsigned int)m_recvFPSPacks.GetFPSAvg();
	}
	if (bigData) {
		if (msg.header()->id == HXMessagePackage::Header::ID_BigStreamBegin ||
			msg.header()->id == HXMessagePackage::Header::ID_BigStreamData ||
			msg.header()->id == HXMessagePackage::Header::ID_BigStreamEnd)
		{
			if (bigData->RecvPackage(msg)) {
				HXMessagePackage temp = msg;
				temp.header()->id = bigData->m_header.id;
				temp.header()->id2 = bigData->m_header.id2;
				temp.header()->length = bigData->m_header.originalLength;
				if (bigData->m_recvData.size() > 0) {
					temp.bigDataPointer = new char[bigData->m_recvData.size()];
					memcpy(temp.bigDataPointer, &bigData->m_recvData.at(0), bigData->m_header.originalLength);
				}
				m_queue[m_current].Add(temp);
			}
			return;
		}
	}
	m_queue[m_current].Add(msg);
	//如果队列过大则清空第一个
	if (m_queue[m_current].GetCount() >= m_queueMax)
		m_queue[m_current].RemoveFront();
}
HXBigMessageQueue::HXBigMessageQueue() {
	m_bAllowLost = false;
	m_maxOfMemory = 0;
}
void			HXBigMessageQueue::SetAllowLostPackage(bool bAllowLost, unsigned int maxOfMemory)
{
	m_bAllowLost = bAllowLost;
	m_maxOfMemory = maxOfMemory;
}
//交换当前消息队列
HXMessageQueue&	HXMessageQueueAB::swap()
{
	boost::mutex::scoped_lock lock(m_mutex);
	int old = m_current;
	m_current = (m_current + 1) % 2;
	m_queue[m_current].Reset();
	return m_queue[old];
}
//返回当前主线程可以使用的队列
HXMessageQueue&	HXMessageQueueAB::get()
{
	return m_current == 0 ? m_queue[1] : m_queue[0];
}

//返回一个消息指针
const HXMessagePackage*		HXBigMessageQueue::GetPackage(unsigned int index)
{
	if (index >= m_queue.size())
		return 0;
	return &m_queue[index];
}
//返回消息包数量
int							HXBigMessageQueue::GetCount()
{
	return (int)m_queue.size();
}
//添加一个消息
void						HXBigMessageQueue::AddPackage(const HXMessagePackage& pack)
{
	m_queue.push_back(pack);
}
//添加一个大消息块
void						HXBigMessageQueue::SetBigPackage(unsigned short id, unsigned short id2, const void* ptr, int dataLength, ZipLevel zipLevel)
{
	//zip unzip
	int originalLength = dataLength;
	std::vector<Bytef> comp;
	comp.resize(dataLength + 2048);
	uLongf length = (uLongf)comp.size();
	if ((int)zipLevel > 0) {
		if (compress2(&comp.at(0), &length, (Bytef*)ptr, dataLength, zipLevel) == Z_OK) {
			ptr = &comp.at(0);
			dataLength = length;
		}
	}
	//
	HXMessagePackage temp;
	HXMessagePackage::Header* header = temp.header();
	header->id = HXMessagePackage::Header::ID_BigStreamBegin;
	header->id2 = 0;
	header->length = sizeof(MBigHeader);
	m_header.id = id;
	m_header.id2 = id2;
	m_header.zipLevel = zipLevel;
	m_header.dataLength = dataLength;
	m_header.originalLength = originalLength;
	m_header.sectionLength = (HXMessagePackage::max_body_length);
	m_header.sectionCount = m_header.dataLength / m_header.sectionLength + 1;
	memcpy(temp.body(), &m_header, sizeof(MBigHeader));
	m_queue.clear();
	//
	AddPackage(temp);
	for (int i = 0; i < m_header.sectionCount; i++) {
		header->id = HXMessagePackage::Header::ID_BigStreamData;
		header->id2 = i;
		int begin = i * (int)m_header.sectionLength;
		int count = begin + m_header.sectionLength;
		if (count > dataLength)
			count = dataLength;
		header->length = count - begin;
		memcpy(temp.body(), (char*)ptr + begin, count - begin);
		AddPackage(temp);
	}
	//
	header->id = HXMessagePackage::Header::ID_BigStreamEnd;
	header->id2 = 0;
	header->length = sizeof(MBigHeader);
	memcpy(temp.body(), &m_header, sizeof(MBigHeader));
	AddPackage(temp);
	//
}
bool						HXBigMessageQueue::RecvPackage(const HXMessagePackage& pack)
{
	const HXMessagePackage::Header* header = pack.header();
	if (header->id == HXMessagePackage::Header::ID_BigStreamBegin) {
		this->m_queue.clear();
		m_header = *pack.body<MBigHeader>();
		m_recvData.resize(m_header.dataLength);
	}
	else if (header->id == HXMessagePackage::Header::ID_BigStreamData)
	{
		if (m_recvData.size() != m_header.dataLength || m_recvData.size() == 0) {
			LOG(LogError, "数据块没有初始化开头定义");
			return false;
		}
		if (header->id2 >= m_header.sectionCount)
		{
			LOG(LogError, "大消息数据索引值超出块大小，数据出现异常");
			return false;
		}
		int recvDataLength = (int)header->length + (int)header->id2*(int)m_header.sectionLength;
		assert(recvDataLength <= m_header.dataLength);
		if (recvDataLength > m_header.dataLength)
		{
			LOG(LogError, "大消息数据超出原来数据包大小，数据出现异常");
			return false;
		}
		memcpy(&m_recvData.at(0) + (int)header->id2*(int)m_header.sectionLength, pack.body(), header->length);
	}
	else if (header->id == HXMessagePackage::Header::ID_BigStreamEnd)
	{
		MBigHeader EndHeader = *pack.body<MBigHeader>();
		if (EndHeader.dataLength == m_header.dataLength && EndHeader.sectionCount == m_header.sectionCount)
		{
			if (m_header.zipLevel > 0) {
				std::vector<char> comp = m_recvData;
				m_recvData.resize(m_header.originalLength);
				uLongf compLength = (uLongf)m_recvData.size();
				uLongf srcLength = (uLongf)comp.size();
				if (uncompress2((Bytef*)&m_recvData.at(0), &compLength, (Bytef*)&comp.at(0), &srcLength) == Z_OK) {
					return (compLength == m_header.originalLength);
				}
			}
			else
				return true;
		}
	}
	return false;
}
const MBigHeader&			HXBigMessageQueue::GetBigPackage(std::vector<char>& result)
{
	//zip unzip
	result = m_recvData;
	return m_header;// m_recvData.size();
}
