#pragma once
#include <thread>
#include "MsgQueue.h"
#include "HXLibNetwork.h"
#include "HXLibMessage.h"
#include "HXDBClient.h"
#include "protocol.h"

#include <boost/lockfree/spsc_queue.hpp>

class UserVerifier
{
public:
	UserVerifier(HXLibService* pNetService, HXDBClient* pDBClient);
	virtual ~UserVerifier();

	void AddMsgPackage(const HXMessagePackage& msgPackage);
	void HandleQueueInMsg();
	void HandleQueueOutMsg();
	int VerifyUserID(int userID, LinkID& linkID, LinkID& usrLinkID);

	void Start();

private:
	MsgQueue<HXMessagePackage> *queueIn;
	MsgQueue<HXMessagePackage> *queueOut;


	//boost::lockfree::spsc_queue<HXMessagePackage, boost::lockfree::capacity<10240> > *queueIn;
	//boost::lockfree::spsc_queue<HXMessagePackage, boost::lockfree::capacity<10240> > *queueOut;

	HXLibService *mpNetService;
	HXDBClient* mpDBClient;

	std::thread *thread_HandleQueueIn;
	std::thread *thread_HandleQueueOut;

};

