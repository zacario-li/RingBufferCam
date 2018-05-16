# RingBufferCam
### our purpose is to design a service which can provide camera resource simultaneously for more than one client apps
### for example: we have only one webcam in a laptop, now we run serveral apps, and they all need to access the camera resource. Here we designed a service use share memory to provide camera frames. Client apps can access share memory to retrive camera frames simultaneously without collision.
>>>
* In this code, we designed a service for 3 clients. You can read  RingBufferCam/SHM_Cam_Server/cfg.txt for details.
* #name:path:sockid:msgid     --->   name[client app name], path[share memory path], sockid[socket ipc name], msgid[message ipc name]
>>>
