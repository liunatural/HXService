#include "HXLibNetwork.h"
#include <vector>
#include "../sharetest.h"
#include "PlayerManager.h"
#include <direct.h>
#include <thread>
#include  "SvcTimer.h"
#include "ErrorMsg.h"
#include "direct.h"
#include "io.h"


void TimeFun(PlayerManager *playerMgr)
{
	io_service io;
	SvcTimer p(io, playerMgr);
	io.run();
}

int main()
{
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

	strcat(drive, "SceneServer/");
	if (0 != _access(drive, 0))
	{
		if (0 != _mkdir(drive))
		{
			cout << "������־Ŀ¼ʧ�ܣ�" << endl;
			return -1;
		}
	}

	//*****����Ҫ������־ϵͳ�ļ�*****//
	SetupLogFiles(drive, true);

	//*****��ȡ�����ļ�*****//
	HXLibConfigService* confReader = CreateConfigReader();
	char path[MAX_PATH] = { 0 };
	_getcwd(path, MAX_PATH);
	strcat(path, "/SceneService.xml");
	bool ret = confReader->OpenFile(path);
	if (!ret)
	{
		LOG(LogError, "�����ļ���ȡʧ��\n");
		return FAIL;
	}

	//*****��������������*****//
	int port = confReader->GetInt("root.Server.port");
	int maxlinks = confReader->GetInt("root.Server.maxlinks");
	char sceneServerID[SCENE_SERVER_ID_LENGTH + 1] = { 0 };
	confReader->GetStr("root.Server.name", sceneServerID);
	
	HXLibService*	service = CreateNewService();
	if (!service->Start(port, maxlinks))
	{
		return FAIL;
	}

	//*****�����û�������*****//
	PlayerManager *playerMgr = new PlayerManager();
	playerMgr->SetNetworkService(service);

	playerMgr->SetSceneServerID(sceneServerID);

	//*****�������ķ�����*****//
	char centerServerIP[50] = { 0 };
	char centerServerPort[50] = { 0 };
	confReader->GetStr("root.CenterServer.ip", centerServerIP);
	confReader->GetStr("root.CenterServer.port", centerServerPort);
	HXLibClient* 	centerServerConn = CreateNewClient();
	if (centerServerConn->Start(centerServerIP, centerServerPort) != 0)
	{
		LOG(LogError, "�������ķ�����ʧ��!\n");
		return ERR_CONNECT_CENTER_SERVER;
	}
	playerMgr->SetCenterSvrConnection(centerServerConn);



	//*****������ʱ���������λ�ñ任��Ϣ*****//
	std::thread tr(&TimeFun, playerMgr);
	tr.detach();


	//*****����Ϣѭ������	*****//
	while (true)
	{
		//�����뱾���û�����Ϣ����
		HXMessageMap& mmap = service->SwapQueue();
		for (int i = 0; i < mmap.GetCount(); i++)
		{
			const HXMessagePackage* pack = mmap.GetPackage(i);

			LinkID linkID = pack->linkid();
			int plyId = (pack->linkid()).sid;

			switch (pack->header()->id)		//������ϢID������ID�ֱ�����߼�����
			{
			case HXMessagePackage::Header::ID_Connect:
			{
				Player*  ply = new Player(pack->linkid());
				ply->SetSceneServerID(sceneServerID);
				playerMgr->AddPlayer(ply);
				
				playerMgr->SendClientList(linkID);
				break;
			}
			case HXMessagePackage::Header::ID_Disconnect:
			{
				playerMgr->SendPlayerLeaveMsg(plyId);
				break;
			}
			case ID_User_Notify:
			{
				int cmdID = pack->header()->id2;
				break;

			}
			case ID_Global_Notify:
			{
				int cmdID = pack->header()->id2;

				if (c2s_begin_flying == cmdID)
				{
					//�����ķ�����ת������Ϣ
					centerServerConn->Send(ID_Global_Notify, s2c_begin_flying, NULL, 0);
				}
				else if (c2s_play_video == cmdID)
				{
					//�����ķ�����ת��������Ƶ��Ϣ
					centerServerConn->Send(ID_Global_Notify, s2c_play_video, NULL, 0);
				}
				else if (c2s_stand_up == cmdID)
				{
					//�����ķ�����ת���û�վ����Ϣ
					centerServerConn->Send(ID_Global_Notify, s2c_stand_up, NULL, 0);
				}
				else if (c2s_walk == cmdID)
				{
					//�����ķ�����ת���û���ʼ������Ϣ
					centerServerConn->Send(ID_Global_Notify, s2c_walk, NULL, 0);
				}
				else if (c2s_present_flowers == cmdID)
				{
					//������VIP�ͻ��˹㲥��ǰ�û���״̬Ϊ�׻�״̬
					playerMgr->BroadcastUserState(plyId, ID_User_Login, state_present_flowers);
				}
				else if (c2s_stop_present_flowers == cmdID)
				{
					//������VIP�ͻ��˹㲥��ǰ�û���״̬Ϊ�����׻�״̬
					playerMgr->BroadcastUserState(plyId, ID_User_Login, state_stop_present_flowers);
				}
				else if (c2s_seen_external == cmdID)
				{
					//******������ֻ�յ�һ��***********//

					//�����û��糡���������ɼ�
					playerMgr->SetUserVisibilityExternal(true);

					//�����ķ��������͵�ǰ�����������ϵ��û��б�
					playerMgr->SendClientListToCenterServer();

					//�����ķ������������������������ϵ��û��б���Ϣ
					centerServerConn->Send(ID_Global_Notify, s2s_req_usr_list, NULL, 0);
				}
				break;
			}
			case ID_User_Login:
			{
				int cmdID = pack->header()->id2;
				char* userID = (char*)pack->body();
				int len = pack->header()->length;

				if (c2s_tell_seat_num == cmdID)		//���մ�VIP�ͻ��˷��������κ���Ϣ
				{
					int seatNum = *(int*)pack->body();

					//����λ�ŵ�һ�����
					bool bRet = playerMgr->UpdatePlayerSeatNumber(plyId, seatNum);
					if (bRet)
					{
						//������VIP�ͻ��˹㲥��ǰ�û���״̬Ϊ��ʼ״̬
						playerMgr->BroadcastUserState(plyId, ID_User_Login, state_initial);
					}

					//******ÿ����һ���û���ִ��һ��******//
					if (playerMgr->GetUserVisibilityExternal())
					{
						//�����ķ��������͵�ǰ�����������ϵ��û��б�
						playerMgr->SendClientListToCenterServer();

						//�����ķ������������������������ϵ��û��б���Ϣ
						centerServerConn->Send(ID_Global_Notify, s2s_req_usr_list, NULL, 0);
					}

				}
				else if (c2s_tell_user_type == cmdID)		//���մ�VIP�ͻ��˷������û�������Ϣ
				{
					int userType = *(int*)pack->body();

					//�����û�����
					bool bRet = playerMgr->UpdateUserType(plyId, (UserType)userType);

				}
				else if (c2s_tell_user_profile == cmdID)	//���մӽ�����ͻ��˷������û�ID����ģ�ļ�����
				{
					int retPlyId;
					FaceModel* faceModel = (FaceModel*)pack->body();
					playerMgr->BindFaceModeWithSeatNumber(linkID, faceModel, retPlyId);

					//������VIP�ͻ��˹㲥��ǰ�û���״̬Ϊ��Ӱ״̬
					playerMgr->BroadcastUserState(retPlyId, ID_User_Login, state_entering);
				}
				else if (c2s_tell_ready == cmdID)		//���մ�VIP�ͻ��˷�����׼��������Ϣ
				{
					//������VIP�ͻ��˹㲥��ǰ�û���״̬Ϊ����״̬
					int seatNum = *(int*)pack->body();
					playerMgr->SendUserReadyMsg(seatNum);
				}
				break;
			}
			case ID_User_Transform:		//���λ�ñ任��Ϣ
			{
				TransformInfo* transInfo = (TransformInfo*)pack->body();
				transInfo->plyId = plyId;

				playerMgr->UpdatePlayerTransform(plyId, *transInfo);

				break;
			}
			//case ID_User_Verify:
			//{
			//	int cmdID = pack->header()->id2;
			//	if (c2s_req_id_verify == cmdID)
			//	{
			//		HXMessagePackage msgPackage;
			//		msgPackage.header()->id = ID_User_Verify; //����λ�ñ任��Ϣ
			//		msgPackage.header()->id2 = c2s_req_id_verify;
			//		msgPackage.header()->length = pack->length() + 4;

			//		memcpy(msgPackage.body(), (void*)pack, pack->length() + 4);

			//		int userID = *(int*)(pack->body());
			//		//cout << "�û�" << userID << "������֤��" << endl;

			//		centerServerConn->Send(msgPackage); //�����ķ����������û������֤����
			//	}
			//	break;
			//}
			}//switch end

		}// for end
		
		//���ղ��������ķ���������Ϣ��Ŀǰ�ǽ��ղ�ת��OBS��FLV����Ƶ���������û������֤��Ϣ��
		if (centerServerConn)
		{
			HXMessageMap& mmap1 = centerServerConn->SwapQueue();
			for (int i = 0; i < mmap1.GetCount(); i++)
			{
				const HXMessagePackage* pack = mmap1.GetPackage(i);
				LinkID linkID = pack->linkid();
				switch (pack->header()->id)
				{
				case ID_Global_Notify:
				{
					int cmdID = pack->header()->id2;
					if (s2c_begin_flying == cmdID)
					{
						//������VIP�ͻ��˷����������
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_begin_flying);
					}
					else if (s2c_play_video == cmdID)
					{
						//������VIP�ͻ��˷���������Ƶ����
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_play_video);
					}
					else if (s2c_stand_up == cmdID)
					{
						//������VIP�ͻ��˷���վ������
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_stand_up);
					}
					else if (s2c_walk == cmdID)
					{
						//������VIP�ͻ��˷�����ʼ��������
						playerMgr->BroadcastControlCmd(ID_User_control, s2c_walk);
					}
					else if (s2c_client_list_external == cmdID)
					{
						//������VIP�ͻ��˷������ķ�����ת�����������������������ϵ��û��б�
						playerMgr->SendMsg(*pack);
					}
					else if (s2c_user_leave_external == cmdID)
					{
						//������VIP�ͻ��˷������ķ�����ת�����������������������ϵ��û��뿪��Ϣ
						playerMgr->SendMsg(*pack);
					}
					else if (s2s_req_usr_list == cmdID)
					{
						//�����û��糡���������ɼ�
						playerMgr->SetUserVisibilityExternal(true);

						playerMgr->SendClientListToCenterServer();
					}
					break;
				}
				case ID_User_Login:
				{
					int cmdID = pack->header()->id2;
					if (s2c_upd_user_state == cmdID) 
					{
						//������VIP�ͻ��˷������ķ�����ת�����������������������ϵ��û�״̬�仯��Ϣ(��ʼ�׻��������׻��ȵ�)
						playerMgr->BroadcastExternalUserState(pack);
					}
					break;
				}
				case ID_Global_Transform:
				{
					//
					playerMgr->SendMsg(*pack);
					break;
				}
				//case ID_User_Verify:
				//{
				//	int cmdID = pack->header()->id2;
				//	if (s2c_rsp_id_verify == cmdID)
				//	{

				//		HXMessagePackage* pMsgPackage = (HXMessagePackage*)pack->body();
				//		LinkID usrLnkID = pMsgPackage->linkid();

				//		int userID = *(int*)(pMsgPackage->body());
				//		cout << "�û�" << userID << "��֤��ϣ�" << endl;

				//		service->Send(usrLnkID, *pMsgPackage);		//������ͻ��˷����û������֤���
				//	}
				//	break;
				//}
				case ID_FLV_Sequence_Header:
				{
					uint8_t* data = (uint8_t*)pack->body();
					int len = pack->body_length();
					playerMgr->CacheFlvSeqHeaderData(data, len);

					break;
				}
				case ID_FLV_Stream:
				{
					char* data = (char*)pack->body();
					int len = pack->body_length();
					{
						playerMgr->SendFlvStream(data, len);
					}
					break;
				}

				} //switch end
			} // for end
		}// if end 	
		
		


		Sleep(1);
	
	}

	return 0;
}

