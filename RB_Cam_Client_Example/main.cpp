#include "common.hpp"
#include "rb-common.hpp"

int main(int argc, char* argv[])
{
    static MatData data_frame;//make sure this can along with the app's whole life
    std::string appname = "face";
    int sockfd = -1;

    //get the parameters
    if(2 == argc)
    {
        appname.clear();
        appname = argv[1];
    }

    sockfd = init_and_connect_cam(appname);
    if(-1 == sockfd)
    {
        std::cout<<"connect error"<<std::endl;
        return 1;
    }

    std::string name = get_shm_path(appname,SHM_PATH);
    ShmRingBuffer<MatData> buffer(CAPACITY,false,name.c_str()); //make sure keep this buffer along with the app's whole life.

    while(true)
    {
        Mat cam_img = shm_get_image(&buffer,&data_frame,appname);
        if(cam_img.cols <= 0)
        {
            sleep(1);
        }else
        {
            imshow(appname.c_str(),cam_img);
            waitKey(1);
        }
        usleep(20000);
    }

    close_cam(sockfd);
    return 1;
}