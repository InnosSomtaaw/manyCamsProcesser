#include "hikcamera.h"

void CALLBACK g_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    char tempbuf[256] = {0};
    switch(dwType)
    {
    case EXCEPTION_RECONNECT:    //预览时重连
    printf("----------reconnect--------%d\n", time(NULL));
    break;
    default:
    break;
    }
}

hikCamera::hikCamera()
{
    char cryptoPath[2048] = {0};
    sprintf(cryptoPath, "/home/HCNetSDK/lib/libcrypto.so.1.1");
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_LIBEAY_PATH, cryptoPath);

    char sslPath[2048] = {0};
    sprintf(sslPath, "/home/HCNetSDK/lib/libssl.so.1.1");
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SSLEAY_PATH, sslPath);

    NET_DVR_LOCAL_SDK_PATH struComPath = {0};
    sprintf(struComPath.sPath, "/home/HCNetSDK/lib");
    //HCNetSDKCom文件夹所在的路径
    NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struComPath);

}

void hikCamera::startCamera()
{
    //---------------------------------------
    // 初始化
    if(!NET_DVR_Init())
    {    
        printf("Ini failed, error code: %d\n", NET_DVR_GetLastError());
//        return;
    }
    NET_DVR_SetLogToFile(3,"/mnt/zhangshaoyue/");
    //设置连接时间与重连时间
    NET_DVR_SetConnectTime(75000, 1000);
    NET_DVR_SetReconnect(10000, true);

    //---------------------------------------
    //设置异常消息回调函数
    NET_DVR_SetExceptionCallBack_V30(0, NULL,g_ExceptionCallBack,(void*)this);

    //---------------------------------------
    // 获取控制台窗口句柄
//    HMODULE hKernel32 = GetModuleHandle("kernel32");
//    GetConsoleWindowAPI = (PROCGETCONSOLEWINDOW)GetProcAddress(hKernel32,"GetConsoleWindow");

    //---------------------------------------
    // 注册设备

    QByteArray qba;
    //登录参数，包括设备地址、登录用户、密码等
    NET_DVR_USER_LOGIN_INFO struLoginInfo = {0};
    struLoginInfo.bUseAsynLogin = 0; //同步登录方式
    qba=hikLogin[0].toLocal8Bit();
    strcpy(struLoginInfo.sDeviceAddress, qba.data()); //设备IP地址
    struLoginInfo.wPort = hikLogin[1].toInt(); //设备服务端口
    qba.clear();
    qba=hikLogin[2].toLocal8Bit();
    strcpy(struLoginInfo.sUserName, qba.data()); //设备登录用户名
    qba.clear();
    qba=hikLogin[3].toLocal8Bit();
    strcpy(struLoginInfo.sPassword, qba.data()); //设备登录密码

    //设备信息, 输出参数
    NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = {0};

    lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
    if (lUserID < 0)
    {
        printf("Login failed, error code: %d\n", NET_DVR_GetLastError());
        NET_DVR_Cleanup();
        return;
    }

    //---------------------------------------
    //启动预览并设置回调数据流

    NET_DVR_PREVIEWINFO struPlayInfo = {0};
    struPlayInfo.hPlayWnd     = NULL;         //需要SDK解码时句柄设为有效值，仅取流不解码时可设为空
    struPlayInfo.lChannel     = hikLogin[4].toInt();       //预览通道号+32
    struPlayInfo.dwStreamType = 0;       //0-主码流，1-子码流，2-码流3，3-码流4，以此类推
    struPlayInfo.dwLinkMode   = 0;       //0- TCP方式，1- UDP方式，2- 多播方式，3- RTP方式，4-RTP/RTSP，5-RSTP/HTTP
    struPlayInfo.bBlocked     = 0;       //0- 非阻塞取流，1- 阻塞取流

}

void hikCamera::getOneFrame()
{

}

void hikCamera::run()
{

}
