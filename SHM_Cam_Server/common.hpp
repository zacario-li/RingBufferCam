#ifndef COMMON_H
#define COMMON_H
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <thread>
#include <vector>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include "rb-common.hpp"

#define VERSION_NUM_COMM "0.2b"
#define MAT_DATA_SIZE 6220800 /* 1920 * 1080 * 3 */
#define MSG_BUFSIZE 10
#define CAPACITY 10
#define YOLO_WORKING_BIT 0x1
#define FACE_WORKING_BIT 0x2
#define OCR_WORKING_BIT 0x4
#define CMD_RBCAM_CONNECT "RBCAM_CONNECT"
#define CMD_RBCAM_DISCONNECT "RBCAM_DISCONNECT"

using namespace cv;
using namespace std;
typedef enum{
    SHM_PATH = 0,
    SHM_MSG = 1,
    SHM_SOCK = 2,
}shm_path_type;

typedef enum{
    RBCAM_CONNECT = 0,
    RBCAM_DISCONNECT = 1,
}rbcam_ctl_type;

typedef struct _rb_msg_type
{
	long int msg_type;
    rbcam_ctl_type msg_status;
}rb_msg_type;

typedef struct _MatData{
	int rows;
	int cols;
	int channels;
	size_t length;
	uchar data[MAT_DATA_SIZE];
}MatData;

string get_shm_path(string app,shm_path_type type);
bool get_available_webcam_id(int *pId);
void msg_thread(string app,int *pWorking_bits);
int create_msg_dir(string app);
void client_test_thread();//for test only

//for other app use
int init_and_connect_cam(std::string appname);
void close_cam(int sockfd);
Mat shm_get_image(ShmRingBuffer<MatData> *pSHM,MatData *pSD,std::string appname);
//end
int shm_connect_camera(std::string appname);
std::vector<int> shm_init_cam_server(std::string server_name);
void shm_start_server(std::string server_name,int *pWorking_bits,int working_bit);
void shm_start_client(std::string server_name);
void shm_camera_status_monitor(VideoCapture *pCam, int camIndex, int *pWorkingBit);
std::string shm_gen_uuid_string();
std::string shm_receive_and_send_rsp(int sockfd,std::string str);
std::string shm_send_and_wait_rsp(int sockfd, std::string str);
std::string shm_parse_data_frame(std::string data);
std::string shm_prepare_cmd_frame(std::string str);

#endif
