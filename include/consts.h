#include <stdint.h>

#define USER_ID_LENGTH (32)
#define FILE_NAME_LENGTH (60)
#define SCENE_SERVER_ID_LENGTH (32)

#define MIN_SEAT_NUMBER 1
#define MAX_SEAT_NUMBER 30


//OBS数据包类型
#define VIDEO_SEQUENCE_HEADER 0
#define AUDIO_SEQUENCE_HEADER 1
#define VIDEO_DATA						  2	
#define AUDIO_DATA						  3


enum ERROR_CODE
{
	NO_AVAILABLE_SEAT_NUMBER	= 1,					//没有可用的座席供分配
	PLAYER_ID_NOT_EXISTS			= 2,					//玩家ID在列表中不存在	
	INVALID_SEAT_NUMBER,									//无效的座席号
};

//客户端类型
enum UserType
{
	VIP					= 1,
	Capsule				= 2,
	VideoCamera,
	SceneServer,
	OBSServer,
};

//用户状态
enum UserState
{
	state_initial = 0,
	state_entering,
	state_ready,
	state_left,
	state_present_flowers,					//玩家献花
	state_stop_present_flowers,			//玩家献花	
};

typedef struct _FlvSeqHeader_
{
	uint8_t id;  //08: Audio Sequence Header;  09：Video Sequence Header
	int size;
	unsigned char* data;
} FlvSeqHeader;
