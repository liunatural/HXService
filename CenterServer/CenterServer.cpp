#include "HXLibNetwork.h"
#include <vector>
#include "../sharetest.h"
#include "UserManager.h"
#include <direct.h>
#include <io.h>
#include <thread>
#include "HXDBClient.h"
#include "UserVerifier.h"
#include <windows.h>


int main()
{

	//*********������־ϵͳ***********//
	char drive[512], dir[512];
	_splitpath(_pgmptr, drive, dir, 0, 0);
	strcat(drive, dir);
	strcat(drive, "log/");
	//��־Ŀ¼�����ڣ��ʹ�����
	if (0 != _access(drive, 0))
	{
		if (0 != _mkdir(drive))
		{
			cout << "������־Ŀ¼ʧ�ܣ�" << endl;
			return -1;
		}
	}

	strcat(drive, "CenterServer/");
	if (0 != _access(drive, 0))
	{
		if (0 != _mkdir(drive))
		{
			cout << "������־Ŀ¼ʧ�ܣ�" << endl;
			return -1;
		}
	}

	SetupLogFiles(drive, true);


	//***********��ȡ�����ļ�**************//
	HXLibConfigService* confReader = CreateConfigReader();
	char path[256] = { 0 };
	_getcwd(path, 256);
	strcat(path, "/CenterService.xml");
	bool ret = confReader->OpenFile(path);
	if (!ret)
	{
		LOG(LogError, "��ȡ�����ļ�ʧ��.\n");

		return FAIL;
	}


	////**********��ʼ�����ݿ�ͻ���************//
	//char dbName[100] = { 0 };
	//char dbUser[100] = { 0 };
	//char dbPwd[100] = { 0 };
	//char dbServerName[100] = { 0 };
	//int dbPort = confReader->GetInt("root.DBServer.HostPort");
	//confReader->GetStr("root.DBServer.HostName", dbServerName);
	//confReader->GetStr("root.DBServer.Database", dbName);
	//confReader->GetStr("root.DBServer.UserName", dbUser);
	//confReader->GetStr("root.DBServer.Password", dbPwd);

	//HXDBClient dbClient(dbServerName, dbPort, dbName, dbUser, dbPwd);
	//int iRet = FAIL;
	//
	//iRet = dbClient.CreateDBConn();


	//if (SUCCESS != iRet)
	//{
	//	LOG(LogError, "�������ݿ����ʧ��.\n");
	//	return FAIL;
	//}

	UserManager *userMgr = new UserManager();
	HXLibService*	service = CreateNewService();
	userMgr->SetNetworkService(&service);


	////��ʼ���û���֤��
	//UserVerifier usrVerifier(service, &dbClient);
	//usrVerifier.Start();


	//*****�������ķ�����*****//
	int port = confReader->GetInt("root.Server.port");
	int maxlinks = confReader->GetInt("root.Server.maxlinks");
	if (!service->Start(port, maxlinks))
		return FAIL;

	while (true) 
	{
		HXMessageMap& mmap = service->SwapQueue();
		for (int i = 0; i < mmap.GetCount(); i++) 
		{
			const HXMessagePackage* pack = mmap.GetPackage(i);
			
			LinkID linkID = pack->linkid();
			int userId = (pack->linkid()).sid;

			switch (pack->header()->id)		//������ϢID������ID�ֱ�����߼�����
			{
					case HXMessagePackage::Header::ID_Connect:
					{
						User*  user = new User(pack->linkid());
						userMgr->AddUser(user);
						break;
					}
					case HXMessagePackage::Header::ID_Disconnect:
					{
						userMgr->DeleteUser(userId);
						break;
					}
					case ID_User_Login:
					{
						int cmdID = pack->header()->id2;
						if (s2c_upd_user_state == cmdID)
						{
							//����������������ת���û�״̬�仯��Ϣ��Ŀǰ��ʼ�׻��������׻�״̬��
							userMgr->ForwardMsg(linkID, pack);
						}
						break;
					}
					case ID_Global_Notify:
					{
						int cmdID = pack->header()->id2;
						if (s2c_begin_flying == cmdID)
						{
							//�����г���������ת���������
							userMgr->ForwardCommand(pack);
						}
						else if (s2c_play_video == cmdID)
						{
							//�����г���������ת��������Ƶ����
							userMgr->ForwardCommand(pack);
						}
						else if (s2c_stand_up == cmdID)
						{
							//�����г���������ת��վ������
							userMgr->ForwardCommand(pack);
						}
						else if (s2c_walk == cmdID)
						{
							//�����г���������ת����������
							userMgr->ForwardCommand(pack);
						}
						else if (s2c_client_list_external == cmdID)
						{
							//����������������ת����һ�������������û��б�
							userMgr->ForwardMsg(linkID, pack);
						}
						else if (s2c_user_leave_external == cmdID)
						{
							//����������������ת����һ�������������û��뿪��Ϣ
							userMgr->ForwardMsg(linkID, pack);
						}
						else if (s2s_req_usr_list == cmdID)
						{
							userMgr->ForwardMsg(linkID, pack);
						}

						break;
					}
					case ID_Global_Transform:
					{
						//����������������ת����һ�������������û��˶��任��Ϣ
						userMgr->ForwardMsg(linkID, pack);
						break;
					}

					case ID_User_Transform:
					{
						TransformInfo* transInfo = (TransformInfo*)pack->body();
						transInfo->plyId = userId;
						//userMgr->UpdatePlayerTransform(userId, *transInfo);
						break;
					}
					case ID_FLV_StreamReback:
					{
						userMgr->OnStreamReback(userId, (unsigned char*)pack->body(), pack->body_length());
					}
					break;
					case ID_FLV_Sequence_Header:
					{
						uint8_t* data = (uint8_t*)pack->body();
						int len = pack->body_length();
						userMgr->CacheFlvSeqHeaderData(data, len);

						break;
					}
					case ID_FLV_Stream:
					{
						char* data = (char*)pack->body();
						int len = pack->body_length();
						FlvStreamReback reback = *(FlvStreamReback*)(data + len - sizeof(FlvStreamReback));
						userMgr->SetOBSStreamClient(userId);
						service->Send(pack->linkid(), ID_FLV_StreamReback, reback.dTimer);
						{
							userMgr->SendFlvStream(data, len);
						}
						break;
					}
					case ID_User_Verify:
					{
						//usrVerifier.AddMsgPackage(*pack);
						break;
					}

			}// switch end
		}// for loop end

		Sleep(1);

	} //while end
    return 0;
}

