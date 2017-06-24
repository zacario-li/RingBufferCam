#include "common.hpp"
using namespace boost;
using namespace boost::uuids;
using boost::asio::ip::tcp;

//for other app use
//this will return the sockfd for other purpose
int init_and_connect_cam(std::string appname)
{
    std::string fullname = "/tmp";
    fullname += "/sock_"+appname+"_id";
    int sockfd = shm_connect_camera(fullname);

    if(-1 == sockfd)
    {
        std::cout<<"camera connect error, please try again later"<<std::endl;
        return -1;
    }

    //send "RBCAM_CONNECT" to server, and wait for resp "OK". This means you connected to the server successfully
    std::string resp = shm_send_and_wait_rsp(sockfd,CMD_RBCAM_CONNECT);
    if(0 == resp.compare("OK"))
    {
        std::cout<<"It's OK now, please go on"<<std::endl;
    }else
    {
        std::cout<<"cmd error"<<std::endl;
        close(sockfd);
        return -1;
    }

    return sockfd;
}

//if client doesn't need camera anymore, stop it.
void close_cam(int sockfd)
{
    std::string resp = shm_send_and_wait_rsp(sockfd,CMD_RBCAM_DISCONNECT);
    close(sockfd);
    std::cout<<"camera closed"<<std::endl;
}
Mat shm_get_image(ShmRingBuffer<MatData> * pSHM, MatData *pSD,std::string appname)
{
    memset(pSD,0,sizeof(MatData));
    *pSD = pSHM->dump_front();
    if(pSD->rows > 0 && pSD->cols > 0 && pSD->length > 0)
    {
        Mat img(pSD->rows,pSD->cols,(3 == pSD->channels?CV_8UC3:CV_8UC1));
        uchar* pData = img.ptr<uchar>(0);
        memcpy(pData,pSD->data,pSD->length);
        return img;
    }else
    {
        Mat img(0,0,CV_8UC3);
        return img;
    }
}
//end

int shm_connect_camera(std::string appname)
{
    std::cout<<"Current Lib version,  COMM Ver:"<<VERSION_NUM_COMM<<std::endl;
    int sockfd,len;
    struct sockaddr_un address;
    int result;
    sockfd = socket(AF_UNIX,SOCK_STREAM,0);

    address.sun_family = AF_UNIX;
    strcpy(address.sun_path,appname.c_str());
    len = sizeof(address);

    result =0;
    while(connect(sockfd,(struct sockaddr *)&address,len) == -1)
    {
        std::cout<<"oops: Client, sleep awhile"<<std::endl;
        sleep(1);
        if(++result == 3) //try every 1 sec to connect the server, totally 3 times. If no, tell user oops.
        {
            std::cout<<"still can not connect to server, exit"<<std::endl;
            result = -1;
            break;
        }
    }

    if(-1 == result)
    {
        return -1;
    }

    std::cout<<"get sockfd :"<<sockfd<<std::endl;
    return sockfd;
}

std::vector<int> shm_init_cam_server(std::string server_name)
{
    int server_sockfd, client_sockfd;
    std::vector<int> fds;
    socklen_t server_len, client_len;
    struct sockaddr_un server_address;
    struct sockaddr_un client_address;

    unlink(server_name.c_str());
    server_sockfd = socket(AF_UNIX,SOCK_STREAM,0);

    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path,server_name.c_str());
    server_len = sizeof(server_address);
    bind(server_sockfd,(struct sockaddr*)&server_address,server_len);

    listen(server_sockfd,5);

    //start listening
    std::cout<<server_name<<" server is waiting..."<<std::endl;

        /**
     * data frame struct
     * 0        123         4 5 6 7 8 9 A B ............
     * Tag      Length      Value
     * tag,length,value
     * tag> C:cmd; D:data;
     * length> number {000 ~ 999}
     * value> characters
     */

    client_len = sizeof(client_address);
    client_sockfd = accept(server_sockfd,(struct sockaddr*)&client_address,&client_len);

    fds.push_back(server_sockfd), fds.push_back(client_sockfd);
    return fds;

}

void shm_start_server(std::string server_name,int *pWorking_bits,int working_bit)
{
    std::cout<<"  ****start_server:"<<server_name<<"****  "<<std::endl;

    int server_sockfd, client_sockfd;
    std::vector<int> fds;
    std::string uuid_str;
    RESTART:
    //if client stopped the connection, reset the working bit
    *pWorking_bits &= ~working_bit;
    //end
    fds.clear(),uuid_str.clear();
    uuid_str = shm_gen_uuid_string();//use for next time
    fds = shm_init_cam_server(server_name);
    server_sockfd = fds.at(0), client_sockfd = fds.at(1);

    while(true)
    {   
        std::string _temp = shm_receive_and_send_rsp(client_sockfd,"OK");
        std::cout<<"Server: _temp :"<<_temp<<std::endl;
        
        if(0 == _temp.compare("SOCKET_CLOSED"))
        {
            std::cout<<"SOCKET_CLOSED, stop"<<std::endl;
            close(client_sockfd);
            close(server_sockfd);
            goto RESTART;
        }else
        {
            if(0 == _temp.compare(CMD_RBCAM_CONNECT))
            {
                *pWorking_bits |= working_bit;
            }else if(0 == _temp.compare(CMD_RBCAM_DISCONNECT))
            {
                *pWorking_bits &= ~working_bit;
            }
        }
    }
}

void shm_start_client(std::string server_name)
{
    std::cout<<"start_client"<<std::endl;
    int sockfd;

    sockfd = shm_connect_camera(server_name);

    for(int i =0; i< 2; i++)
    {
        std::string _temp = shm_send_and_wait_rsp(sockfd,"Hi,I'm Yolo, need to register!");
        std::cout<<"Client: _temp :"<<_temp<<std::endl; 
        sleep(1);
    }
    close(sockfd);
    return;
}

void shm_camera_status_monitor(VideoCapture* pCam, int camIndex, int * pWorkingBit)
{
    std::cout<<"start camera status monitor"<<std::endl;
    while(true)
    {
        usleep(100000); //sleep 100ms, so the cpu occupancy will be low.
        bool t = pCam->isOpened();
        int wb = *pWorkingBit & (YOLO_WORKING_BIT|FACE_WORKING_BIT|OCR_WORKING_BIT);

        if(wb == 0 && t == true)
        {
            std::cout<<"all clients' connections are lost, close the camera"<<std::endl;
            pCam->release();
            continue;
        }

        if(wb > 0 && t == false)
        {
            std::cout<<"open camera"<<std::endl;
            pCam->open(camIndex);
            continue;
        }
    }
}

std::string shm_prepare_cmd_frame(std::string str)
{
    std::string _cmd = "C";
    int _len = str.length();
    if(_len <10)
    {
        _cmd += "00";
        _cmd += std::to_string(_len);
    }else if(_len >=10 && _len <100)
    {
        _cmd += "0";
        _cmd += std::to_string(_len);
    }else if(_len >=100 && _len <1000)
    {
        _cmd += std::to_string(_len);
    }else
    {
        return "ERR";
    }
    _cmd += str;
    return _cmd;
}

std::string shm_parse_data_frame(std::string data)
{
    if(data.length()>4)
    {
        std::string tag = data.substr(0,1);
        std::string len = data.substr(1,3);
        int _len = std::stoi(len);
        std::string val = data.substr(4,_len);

        if(0 == tag.compare("C"))
        {
            //process cmd
            return val;

        }else if(0 == tag.compare("D"))
        {
            //process data
            return val;
        }else
        {
            std::cout<<"cmd error"<<std::endl;
            return "ERR";
        }
    }else
    {
        return "ERR";
    }
}

std::string shm_gen_uuid_string()
{
    random_generator uuid_gen;
    uuid id = uuid_gen();
    return boost::uuids::to_string(id);
}

std::string shm_receive_and_send_rsp(int sockfd,std::string str)//todo fix str = NULL
{
    char data[1000] = {0};
    int socket_read_status = -1;
    std::string _temp,cmd;
    _temp.clear(),cmd.clear();

    socket_read_status = read(sockfd,&data,1000);
    if(0 != socket_read_status)
    {
        cmd = data;
        _temp = shm_parse_data_frame(cmd);

        cmd.clear();
        if(0 != _temp.compare("ERR"))
        {
            cmd = shm_prepare_cmd_frame(str);
        }else
        {
            cmd = shm_prepare_cmd_frame(_temp);
        }
        write(sockfd,cmd.c_str(),cmd.length());
    }else
    {
        _temp = "SOCKET_CLOSED";
    }

    return _temp;
}
std::string shm_send_and_wait_rsp(int sockfd, std::string str)
{
    char data[1000] = {0};
    int socket_write_status = -1 , socket_read_status = 0;
    std::string cmd_temp,tk;

    cmd_temp.clear();
    tk.clear();

    cmd_temp = shm_prepare_cmd_frame(str);
    socket_write_status = write(sockfd,cmd_temp.c_str(), cmd_temp.length());

    if(0 < socket_write_status)
    {
        socket_read_status = read(sockfd,&data,1000);
        cmd_temp.clear();
        cmd_temp = data;
        tk = shm_parse_data_frame(cmd_temp);
    }else
    {
        tk = "SOCKET_CLOSED";
    }

    return tk;
}