#include "common.hpp"


#ifndef SHD_LIBS
int main()
{
	Mat img; //this mat variable will be used during the whole app life
	int cam_index = -1; //opencv will use this index to open camera device
	MatData cam_frame; //used for temporaily containing the mat file, then this will be pushed in the shared memory
	int cam_app_working_bits = 0; //we have 3 clients, we use this int variable to indicate which client is using the camera
	std::vector<ShmRingBuffer<MatData>*> shm_vector; //shared memory buffer vector for easily manage

	std::string prefixname = "/tmp"; //local socket file will be created at /tmp dir, cause thus can be accessed by any user
	std::string sockname = (prefixname + get_shm_path("yolo",SHM_SOCK));

	//start yolo server now, we pass working bits, and YOLO_WORKING_BIT.
	//this server response for communicate this its client
	//client send "connect/disconnect" cmd to this, then server control the cam_app_working_bits
	std::thread start_server_yolo(shm_start_server,sockname,&cam_app_working_bits,YOLO_WORKING_BIT);
	start_server_yolo.detach();

	sockname.clear();
	sockname = (prefixname + get_shm_path("face",SHM_SOCK));
	//start face server now, we pass working bits, and FACE_WORKING_BIT.
	//this server response for communicate this its client
	//client send "connect/disconnect" cmd to this, then server control the cam_app_working_bits
	std::thread start_server_face(shm_start_server,sockname,&cam_app_working_bits,FACE_WORKING_BIT);
	start_server_face.detach();

	sockname.clear();
	sockname = (prefixname + get_shm_path("ocr",SHM_SOCK));
	//start face ocr now, we pass working bits, and OCR_WORKING_BIT.
	//this server response for communicate this its client
	//client send "connect/disconnect" cmd to this, then server control the cam_app_working_bits
	std::thread start_server_ocr(shm_start_server,sockname,&cam_app_working_bits,OCR_WORKING_BIT);
	start_server_ocr.detach();


	if( !get_available_webcam_id(&cam_index))
	{
		std::cout<<"error, can not open any camera"<<std::endl;
	}

	VideoCapture cam(cam_index);

	//start camera monitor thread
	/*we monitor camera status and working bits, if no client is using camera resource, this will release camera
	if anyone requests to use camera, this will open camera
	*/
	std::thread camera_monitor(shm_camera_status_monitor,&cam,cam_index,&cam_app_working_bits);
	camera_monitor.detach();
	//end

	//init shared memory vector
	ShmRingBuffer<MatData> buffer(CAPACITY, true,get_shm_path("yolo",SHM_PATH).c_str());
	buffer.clear();
	shm_vector.push_back(&buffer); 
	ShmRingBuffer<MatData> buffer1(CAPACITY, true,get_shm_path("face",SHM_PATH).c_str());
	buffer1.clear();
	shm_vector.push_back(&buffer1);
	ShmRingBuffer<MatData> buffer2(CAPACITY, true,get_shm_path("ocr",SHM_PATH).c_str());
	buffer2.clear();
	shm_vector.push_back(&buffer2);
	//end

	while(true)
	{
		memset(&cam_frame,0,sizeof(cam_frame));

		if(cam_app_working_bits & (YOLO_WORKING_BIT|FACE_WORKING_BIT|OCR_WORKING_BIT) && cam.isOpened())
		{
			cam.read(img);
			cam_frame.rows = img.rows;
			cam_frame.cols = img.cols;
			cam_frame.channels = img.channels();
			cam_frame.length = img.total() * ( (img.channels() == 3)?img.elemSize():img.elemSize1() ) ;
			memcpy(&(cam_frame.data),img.data,cam_frame.length);
			
			if(cam_app_working_bits & YOLO_WORKING_BIT)
			{
				shm_vector.at(0)->push_back(cam_frame);
			}
			if(cam_app_working_bits & FACE_WORKING_BIT)
			{
				shm_vector.at(1)->push_back(cam_frame);
			}
			if(cam_app_working_bits & OCR_WORKING_BIT)
			{
				shm_vector.at(2)->push_back(cam_frame);
			}
		}else
		{
			//std::cout<<"cam_app_working_bits is:"<<cam_app_working_bits<<std::endl;
		}
		usleep(50000); //50ms
	}
	return 1;
}
#endif

string get_shm_path(string app,shm_path_type type)
{
	string name,path,sockid,msgid,line;
	map<string,string> key_pair,key_pair1,temp;
	map<string,string>::iterator m;
	map<string,map<string,string>> whole_key_pair;
	ifstream readCfg("cfg.txt");
	while(getline(readCfg,line))
	{
		name.clear(),path.clear(),sockid.clear();key_pair.clear(),key_pair1.clear();
		stringstream iss(line);
		if(line[0] == '#')
		{
			continue;
		}
		getline(iss,name,':');
		getline(iss,path,':');
		getline(iss,sockid,':');
		getline(iss,msgid,',');//we don't use msg now, please note it.
		key_pair.insert(make_pair(path,sockid));
		whole_key_pair.insert(make_pair(name,key_pair));
	}

	temp = whole_key_pair.find(app)->second;
	m = temp.begin();
	if(type == SHM_SOCK){
		return m->second;
	}else if(type == SHM_PATH){
		return m->first;
	}
}

bool get_available_webcam_id(int *pId)
{
	bool ret = false;
	for(int i=0;i<10;i++)
	{
		VideoCapture cam(i);
		if(cam.isOpened())
		{
			*pId = i;
			ret = true;
			break;
		}
	}
	return ret;
}

void client_test_thread()
{
	static MatData data_frame;
    std::string appname = "face";
    if(-1 == init_and_connect_cam(appname))
    {
        std::cout<<"connect error"<<std::endl;
        return;
    }

	std::string name = get_shm_path(appname,SHM_PATH);
    ShmRingBuffer<MatData> buffer(CAPACITY,false,name.c_str());

    while(true)
    {
        Mat cam_img = shm_get_image(&buffer,&data_frame,appname);
        if(cam_img.cols <= 0)
        {
            sleep(1);
        }else
        {
            imshow("ret",cam_img);
            waitKey(1);
        }

    }
}

void msg_thread(string app,int *pWorking_bits)
{
	int msgid = -1;
	int result = -1;
	rb_msg_type rb_msg;
	key_t msgKey = -1;
	
	string msg_id_path = get_shm_path(app,SHM_MSG);
	//create msg file
	string id_path = "/tmp";
	id_path += msg_id_path;

	rmdir(id_path.c_str());
	if(0 != create_msg_dir(id_path))
	{
		std::cout<<"create msg dir error"<<std::endl;
		return;
	};
	//finish create msg file

	msgKey = ftok(id_path.c_str(),1);
	msgid = msgget(msgKey,0666|IPC_CREAT);

	if(-1 == msgid)
	{
		std::cout<<"msg id get failed"<<std::endl;
		return;
	}
	//monitoring the msg quene
	std::cout<<"monitoring:"<<id_path.c_str()<<std::endl;
	while(true)
	{
		if( msgrcv(msgid,(void*)&rb_msg,MSG_BUFSIZE,0,0)==-1 ) //wait until client send a msg to me.
		{
			std::cout<<"receive error"<<std::endl;
		}else
		{
			std::cout<<app<<" msg data is:"<<rb_msg.msg_status<<std::endl;
			if(0 == app.compare("yolo"))
			{
				if(RBCAM_CONNECT == rb_msg.msg_status){
					*pWorking_bits |= YOLO_WORKING_BIT; //set working bit
				}else{
					*pWorking_bits &= ~YOLO_WORKING_BIT; //clear working bit
				}
			}else if(0 == app.compare("ocr"))
			{
				if(RBCAM_CONNECT == rb_msg.msg_status){
					*pWorking_bits |= OCR_WORKING_BIT; //set working bit
				}else{
					*pWorking_bits &= ~OCR_WORKING_BIT; //clear working bit
				}
			}else if(0 == app.compare("face"))
			{
				if(RBCAM_CONNECT == rb_msg.msg_status){
					*pWorking_bits |= FACE_WORKING_BIT; //set working bit
				}else{
					*pWorking_bits &= ~FACE_WORKING_BIT; //clear working bit
				}
			}else
			{
				std::cout<<"msg error"<<std::endl;
			}
		}
	}
}

int create_msg_dir(string app)
{
	if(access(app.c_str(),0) == -1)
	{
		if(mkdir(app.c_str(),0766))
		{
			std::cout<<"msg id can not create"<<std::endl;
			return -1;
		}
	}
	return 0;
}