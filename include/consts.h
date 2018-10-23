#include <stdint.h>

#define USER_ID_LENGTH (32)
#define FILE_NAME_LENGTH (60)
#define SCENE_SERVER_ID_LENGTH (32)

#define MIN_SEAT_NUMBER 1
#define MAX_SEAT_NUMBER 30


//OBS���ݰ�����
#define VIDEO_SEQUENCE_HEADER 0
#define AUDIO_SEQUENCE_HEADER 1
#define VIDEO_DATA						  2	
#define AUDIO_DATA						  3


enum ERROR_CODE
{
	NO_AVAILABLE_SEAT_NUMBER	= 1,					//û�п��õ���ϯ������
	PLAYER_ID_NOT_EXISTS			= 2,					//���ID���б��в�����	
	INVALID_SEAT_NUMBER,									//��Ч����ϯ��
};

//�ͻ�������
enum UserType
{
	VIP					= 1,
	Capsule				= 2,
	VideoCamera,
	SceneServer,
	OBSServer,
};

//�û�״̬
enum UserState
{
	state_initial = 0,
	state_entering,
	state_ready,
	state_left,
	state_present_flowers,					//����׻�
	state_stop_present_flowers,			//����׻�	
};

typedef struct _FlvSeqHeader_
{
	uint8_t id;  //08: Audio Sequence Header;  09��Video Sequence Header
	int size;
	unsigned char* data;
} FlvSeqHeader;
