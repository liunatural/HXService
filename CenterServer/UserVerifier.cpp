#include "UserVerifier.h"

UserVerifier::UserVerifier(HXLibService* pNetService, HXDBClient* pDBClient) : mpNetService(pNetService), mpDBClient(pDBClient)
{
	queueIn = new MsgQueue<HXMessagePackage>(10000);
	queueOut = new MsgQueue<HXMessagePackage>(10000);

}

UserVerifier::~UserVerifier()
{
	if (NULL != queueIn)
	{
		delete queueIn;
		queueIn = NULL;
	}

	if (NULL != queueOut)
	{
		delete queueOut;
		queueOut = NULL;
	}
}

void UserVerifier::AddMsgPackage(const HXMessagePackage& msgPackage)
{
	queueIn->put(msgPackage);
	//while(!queueIn->push(msgPackage));
}

void UserVerifier::HandleQueueInMsg()
{

	while (true)
	{
		HXMessagePackage messagepack;
		queueIn->get(messagepack);

		//while (!queueIn->pop(messagepack));
		LinkID linkID = messagepack.linkid();

		switch (messagepack.header()->id)
		{
		case ID_User_Verify:
		{
			HXMessagePackage* pMsgPackage = (HXMessagePackage*)messagepack.body();
			LinkID usrLnkID = pMsgPackage->linkid();
			int userID =*(int*) (pMsgPackage->body());

			VerifyUserID(userID, linkID, usrLnkID);
			break;
		}

		}
	}
}

void UserVerifier::HandleQueueOutMsg()
{
	while (true)
	{
		HXMessagePackage messagepack;
		queueOut->get(messagepack);
		//while (!queueOut->pop(messagepack));
		mpNetService->Send(messagepack.linkid(), messagepack);
	}
}

int UserVerifier::VerifyUserID(int userID, LinkID& linkID, LinkID& usrLinkID)
{
	HXMessagePackage  verifyMsgPackage;
	verifyMsgPackage.header()->id = ID_User_Verify;
	verifyMsgPackage.header()->id2 = s2c_rsp_id_verify;
	verifyMsgPackage.header()->length =  sizeof(int);
	memcpy(verifyMsgPackage.body(), &userID, sizeof(int));
	verifyMsgPackage.linkid(usrLinkID);

	HXMessagePackage  msgPackage;
	msgPackage.header()->id = ID_User_Verify;
	msgPackage.header()->id2 = s2c_rsp_id_verify;
	msgPackage.header()->length = verifyMsgPackage.length() +4;
	memcpy(msgPackage.body(), &verifyMsgPackage, verifyMsgPackage.length() + 4);
	msgPackage.linkid(linkID);
	queueOut->put(msgPackage);
	//while(!queueOut->push(msgPackage));
	return 1;
}

void UserVerifier::Start()
{
	thread_HandleQueueIn = new std::thread(&UserVerifier::HandleQueueInMsg, this);
	thread_HandleQueueOut = new std::thread(&UserVerifier::HandleQueueOutMsg, this);
}
