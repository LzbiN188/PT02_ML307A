/*
 * app_server.c
 *
 *  Created on: Jul 14, 2022
 *      Author: idea
 */
#include "app_server.h"
#include "app_kernal.h"
#include "app_net.h"
#include "app_protocol.h"
#include "app_param.h"
#include "app_sys.h"
#include "app_socket.h"
#include "app_gps.h"
#include "app_db.h"
#include "app_jt808.h"
#include "app_task.h"

static netConnectInfo_s privateServConn, bleServConn, hiddenServConn, ble2ServConn;
static jt808_Connect_s jt808ServConn, bleJt808SerConn;
static bleInfo_s *bleHead = NULL;
static int8_t timeOutId = -1;
static int8_t hbtTimeOutId = -1;

/**************************************************
@bref		ģ��ظ���ֹͣ��ʱ������ֹģ��������
@param
@return
@note
**************************************************/

void moduleRspSuccess(void)
{
    if (timeOutId != -1)
    {
        stopTimer(timeOutId);
        timeOutId = -1;
    }
}
/**************************************************
@bref		�����ظ���ֹͣ��ʱ������ֹģ��������
@param
@return
@note
**************************************************/

void hbtRspSuccess(void)
{
    if (hbtTimeOutId != -1)
    {
        stopTimer(hbtTimeOutId);
        hbtTimeOutId = -1;
    }
}


/**************************************************
@bref		ִ�и�λģ��
@param
@return
@note
**************************************************/

static void moduleRspTimeout(void)
{
    timeOutId = -1;
    LogMessage(DEBUG_ALL, "Modlue rsp timeout");
    moduleReset();
}

static void hbtRspTimeOut(void)
{
    LogMessage(DEBUG_ALL, "heartbeat timeout");
    hbtTimeOutId = -1;
    if (sysparam.protocol == ZT_PROTOCOL_TYPE)
    {
        socketDel(NORMAL_LINK);
    }
    else
    {
        socketDel(JT808_LINK);
    }
    moduleSleepCtl(0);
}


/**************************************************
@bref		����״̬���л�
@param
@return
@note
**************************************************/

static void privateServerChangeFsm(NetWorkFsmState state)
{
    privateServConn.fsmstate = state;
}


/**************************************************
@bref		ֹͣ��ʱ������ֹģ��������
@param
@return
@note
**************************************************/

void privateServerReconnect(void)
{
    LogMessage(DEBUG_ALL, "private reconnect server");
    socketDel(NORMAL_LINK);
    moduleSleepCtl(0);
}






/**************************************************
@bref		�������Ͽ�
@param
@return
@note
**************************************************/

void privateServerDisconnect(void)
{
    privateServerChangeFsm(SERV_LOGIN);
}
/**************************************************
@bref		����������¼����
@param
@return
@note
**************************************************/

void privateServerLoginSuccess(void)
{
    privateServConn.loginCount = 0;
    privateServConn.heartbeattick = 0;
    moduleSleepCtl(1);
    ledStatusUpdate(SYSTEM_LED_NETOK, 1);
    privateServerChangeFsm(SERV_READY);
}
/**************************************************
@bref		socket���ݽ���
@param
@return
@note
**************************************************/

static void privateServerSocketRecv(char *data, uint16_t len)
{
    uint16_t i, beginindex, contentlen, lastindex;
    //������Ѱ��Э��ͷ
    for (i = 0; i < len; i++)
    {
        beginindex = i;
        if (data[i] == 0x78)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x78)
            {
                continue ;
            }
            if (i + 2 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2];
            if ((i + 5 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 3 + contentlen] == 0x0D && data[i + 4 + contentlen] == 0x0A)
            {
                i += (4 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7878[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(NORMAL_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
        else if (data[i] == 0x79)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x79)
            {
                continue ;
            }
            if (i + 3 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2] << 8 | data[i + 3];
            if ((i + 6 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 4 + contentlen] == 0x0D && data[i + 5 + contentlen] == 0x0A)
            {
                i += (5 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7979[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(NORMAL_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
    }
}
/**************************************************
@bref		����������������
@param
@return
@note
**************************************************/

void privateServerConnTask(void)
{
    static uint16_t unLoginTick = 0;
    if (isModuleRunNormal() == 0)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        return ;
    }
    if (socketGetUsedFlag(NORMAL_LINK) != 1)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        privateServerChangeFsm(SERV_LOGIN);
        socketAdd(NORMAL_LINK, sysparam.Server, sysparam.ServerPort, privateServerSocketRecv);
        return;
    }
    if (socketGetConnStatus(NORMAL_LINK) != SOCKET_CONN_SUCCESS)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        privateServerChangeFsm(SERV_LOGIN);
        if (unLoginTick++ >= 900)
        {
            unLoginTick = 0;
            moduleReset();
        }
        return;
    }
    switch (privateServConn.fsmstate)
    {
        case SERV_LOGIN:
            unLoginTick = 0;
            if (strncmp(dynamicParam.SN, "888888887777777", 15) == 0)
            {
                LogMessage(DEBUG_ALL, "no Sn");
                return;
            }

            LogMessage(DEBUG_ALL, "Login to server...");
            protocolSnRegister(dynamicParam.SN);
            protocolSend(NORMAL_LINK, PROTOCOL_01, NULL);
            protocolSend(NORMAL_LINK, PROTOCOL_F1, NULL);
            protocolSend(NORMAL_LINK, PROTOCOL_8A, NULL);
            privateServerChangeFsm(SERV_LOGIN_WAIT);
            privateServConn.logintick = 0;
            break;
        case SERV_LOGIN_WAIT:
            privateServConn.logintick++;
            if (privateServConn.logintick >= 60)
            {
                privateServerChangeFsm(SERV_LOGIN);
                privateServConn.loginCount++;
                privateServerReconnect();
                if (privateServConn.loginCount >= 3)
                {
                    privateServConn.loginCount = 0;
                    moduleReset();
                }
            }
            break;
        case SERV_READY:
            if (privateServConn.heartbeattick % (sysparam.heartbeatgap - 2) == 0)
            {
                queryBatVoltage();
                moduleGetCsq();
            }
            if (privateServConn.heartbeattick % sysparam.heartbeatgap == 0)
            {
                privateServConn.heartbeattick = 0;
                if (timeOutId == -1)
                {
                    timeOutId = startTimer(120, moduleRspTimeout, 0);
                }
                if (hbtTimeOutId == -1)
                {
                    hbtTimeOutId = startTimer(1800, hbtRspTimeOut, 0);
                }
                protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                                    dynamicParam.startUpCnt, dynamicParam.runTime);
                protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
            }
            privateServConn.heartbeattick++;
            if (getTcpNack())
            {
                querySendData(NORMAL_LINK);
            }
            if (privateServConn.heartbeattick % 2 == 0)
            {
                //����ram�ٴ��ļ�ϵͳ
                if (getTcpNack() == 0)
                {
                    if (dbUpload() == 0)
                    {
                        gpsRestoreUpload();
                    }
                }
            }
            break;
        default:
            privateServConn.fsmstate = SERV_LOGIN;
            privateServConn.heartbeattick = 0;
            break;
    }
}


static uint8_t hiddenServCloseChecking(void)
{
    if (sysparam.hiddenServOnoff == 0)
    {
        return 1;
    }
    if (sysparam.protocol == ZT_PROTOCOL_TYPE)
    {
        if (sysparam.ServerPort == sysparam.hiddenPort)
        {
            if (strcmp(sysparam.Server, sysparam.hiddenServer) == 0)
            {
                //if use the same server and port ,abort use hidden server.
                return	1;
            }
        }
    }
    if (sysinfo.hiddenServCloseReq)
    {
        //it is the system request of close hidden server,maybe the socket error.
        return 1;
    }
    return 0;
}

static void hiddenServerChangeFsm(NetWorkFsmState state)
{
    hiddenServConn.fsmstate = state;
}

/**************************************************
@bref		socket���ݽ���
@param
@return
@note
**************************************************/

static void hiddenServerSocketRecv(char *data, uint16_t len)
{
    uint16_t i, beginindex, contentlen, lastindex;
    //������Ѱ��Э��ͷ
    for (i = 0; i < len; i++)
    {
        beginindex = i;
        if (data[i] == 0x78)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x78)
            {
                continue ;
            }
            if (i + 2 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2];
            if ((i + 5 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 3 + contentlen] == 0x0D && data[i + 4 + contentlen] == 0x0A)
            {
                i += (4 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7878[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(HIDDEN_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
        else if (data[i] == 0x79)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x79)
            {
                continue ;
            }
            if (i + 3 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2] << 8 | data[i + 3];
            if ((i + 6 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 4 + contentlen] == 0x0D && data[i + 5 + contentlen] == 0x0A)
            {
                i += (5 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7979[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(HIDDEN_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
    }

}

/**************************************************
@bref		���ط�������¼�ظ�
@param
	none
@return
	none
@note
**************************************************/

void hiddenServerLoginSuccess(void)
{
    hiddenServConn.loginCount = 0;
    hiddenServConn.heartbeattick = 0;
    hiddenServerChangeFsm(SERV_READY);
}

/**************************************************
@bref		����ر�������·
@param
@return
@note
**************************************************/

void hiddenServerCloseRequest(void)
{
    sysinfo.hiddenServCloseReq = 1;
    LogMessage(DEBUG_ALL, "hidden serv close request");
}


/**************************************************
@bref		����ر�������·������
@param
@return
@note
**************************************************/

void hiddenServerCloseClear(void)
{
    sysinfo.hiddenServCloseReq = 0;
    LogMessage(DEBUG_ALL, "hidden serv close clear");
}

/**************************************************
@bref		���ط�������������
@param
@return
@note
**************************************************/

static void hiddenServerConnTask(void)
{
    if (isModuleRunNormal() == 0)
    {
        return ;
    }

    if (hiddenServCloseChecking())
    {
        if (socketGetUsedFlag(HIDDEN_LINK) == 1)
        {
            LogMessage(DEBUG_ALL, "hidden server abort");
            socketDel(HIDDEN_LINK);
        }
        hiddenServerChangeFsm(SERV_LOGIN);
        return;
    }
    if (socketGetUsedFlag(HIDDEN_LINK) != 1)
    {
        hiddenServerChangeFsm(SERV_LOGIN);
        socketAdd(HIDDEN_LINK, sysparam.hiddenServer, sysparam.hiddenPort, hiddenServerSocketRecv);
        return;
    }
    if (socketGetConnStatus(HIDDEN_LINK) != SOCKET_CONN_SUCCESS)
    {
        hiddenServerChangeFsm(SERV_LOGIN);
        return;
    }
    switch (hiddenServConn.fsmstate)
    {
        case SERV_LOGIN:
            LogMessage(DEBUG_ALL, "Login to server...");
            protocolSnRegister(dynamicParam.SN);
            protocolSend(HIDDEN_LINK, PROTOCOL_01, NULL);
            hiddenServerChangeFsm(SERV_LOGIN_WAIT);
            hiddenServConn.logintick = 0;
            break;
        case SERV_LOGIN_WAIT:
            hiddenServConn.logintick++;
            if (hiddenServConn.logintick >= 60)
            {
                hiddenServerChangeFsm(SERV_LOGIN);
                hiddenServConn.loginCount++;
                privateServerReconnect();
                if (hiddenServConn.loginCount >= 3)
                {
                    hiddenServConn.loginCount = 0;
                    hiddenServerCloseRequest();
                }
            }
            break;
        case SERV_READY:
            if (hiddenServConn.heartbeattick % (sysparam.heartbeatgap - 2) == 0)
            {
                queryBatVoltage();
                moduleGetCsq();
            }
            if (hiddenServConn.heartbeattick % sysparam.heartbeatgap == 0)
            {
                hiddenServConn.heartbeattick = 0;
                if (timeOutId == -1)
                {
                    timeOutId = startTimer(80, moduleRspTimeout, 0);
                }
                protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                                    dynamicParam.startUpCnt, dynamicParam.runTime);
                protocolSend(HIDDEN_LINK, PROTOCOL_13, NULL);
            }
            hiddenServConn.heartbeattick++;

            break;
        default:
            hiddenServConn.fsmstate = SERV_LOGIN;
            hiddenServConn.heartbeattick = 0;
            break;
    }
}



/**************************************************
@bref		jt808״̬���л�״̬
@param
	nfsm	��״̬
@return
	none
@note
**************************************************/

static void jt808ServerChangeFsm(jt808_connfsm_s nfsm)
{
    jt808ServConn.connectFsm = nfsm;
    jt808ServConn.runTick = 0;
}

/**************************************************
@bref		jt808���ݽ��ջص�
@param
	none
@return
	none
@note
**************************************************/

static void jt808ServerSocketRecv(char *rxbuf, uint16_t len)
{
    jt808ReceiveParser(JT808_LINK, (uint8_t *)rxbuf, len);
}

/**************************************************
@bref		���ݷ��ͽӿ�
@param
	none
@return
	1		���ͳɹ�
	!=1		����ʧ��
@note
**************************************************/

static int jt808ServerSocketSend(uint8_t link, uint8_t *data, uint16_t len)
{
    int ret;
    ret = socketSendData(link, data, len);
    return 1;
}

/**************************************************
@bref		jt808����״̬��
@param
@return
@note
**************************************************/

void jt808ServerReconnect(void)
{
    LogMessage(DEBUG_ALL, "jt808 reconnect server");
    socketDel(JT808_LINK);
    moduleSleepCtl(0);
}

/**************************************************
@bref		jt808��Ȩ�ɹ��ظ�
@param
@return
@note
**************************************************/

void jt808ServerAuthSuccess(void)
{
    jt808ServConn.authCnt = 0;
    moduleSleepCtl(1);
    jt808ServerChangeFsm(JT808_NORMAL);
    ledStatusUpdate(SYSTEM_LED_NETOK, 1);
}

/**************************************************
@bref		jt808��������������
@param
@return
@note
**************************************************/

void jt808ServerConnTask(void)
{
    static uint8_t ret = 1;
    static uint16_t unLoginTick = 0;
    if (isModuleRunNormal() == 0)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        return;
    }
    if (socketGetUsedFlag(JT808_LINK) != 1)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        jt808ServerChangeFsm(JT808_REGISTER);
        jt808RegisterTcpSend(JT808_LINK, jt808ServerSocketSend);
        jt808RegisterManufactureId(JT808_LINK, (uint8_t *)"ZT");
        jt808RegisterTerminalType(JT808_LINK, (uint8_t *)"06");
        jt808RegisterTerminalId(JT808_LINK, (uint8_t *)"01");
        socketAdd(JT808_LINK, sysparam.jt808Server, sysparam.jt808Port, jt808ServerSocketRecv);
        return;
    }
    if (socketGetConnStatus(JT808_LINK) != SOCKET_CONN_SUCCESS)
    {
        ledStatusUpdate(SYSTEM_LED_NETOK, 0);
        jt808ServerChangeFsm(JT808_REGISTER);
        if (unLoginTick++ >= 900)
        {
            unLoginTick = 0;
            moduleReset();
        }
        return;
    }


    switch (jt808ServConn.connectFsm)
    {
        case JT808_REGISTER:

            if (strcmp((char *)dynamicParam.jt808sn, "888777") == 0)
            {
                LogMessage(DEBUG_ALL, "no JT808SN");
                return;
            }

            if (dynamicParam.jt808isRegister)
            {
                //��ע������豸�����ظ�ע��
                jt808ServerChangeFsm(JT808_AUTHENTICATION);
                jt808ServConn.regCnt = 0;
            }
            else
            {
                //ע���豸
                if (jt808ServConn.runTick % 60 == 0)
                {
                    if (jt808ServConn.regCnt++ > 3)
                    {
                        LogMessage(DEBUG_ALL, "Terminal register timeout");
                        jt808ServConn.regCnt = 0;
                        jt808ServerReconnect();
                    }
                    else
                    {
                        LogMessage(DEBUG_ALL, "Terminal register");
                        jt808RegisterLoginInfo(JT808_LINK, dynamicParam.jt808sn, dynamicParam.jt808isRegister, dynamicParam.jt808AuthCode, dynamicParam.jt808AuthLen);
                        jt808SendToServer(JT808_LINK, TERMINAL_REGISTER, NULL);
                    }
                }
                break;
            }
        case JT808_AUTHENTICATION:

            if (jt808ServConn.runTick % 60 == 0)
            {
                ret = 1;
                if (jt808ServConn.authCnt++ > 3)
                {
                    jt808ServConn.authCnt = 0;
                    dynamicParam.jt808isRegister = 0;
                    dynamicParamSaveAll();
                    jt808ServerReconnect();
                    LogMessage(DEBUG_ALL, "Authentication timeout");
                }
                else
                {
                    LogMessage(DEBUG_ALL, "Terminal authentication");
                    jt808ServConn.hbtTick = sysparam.heartbeatgap;
                    jt808RegisterLoginInfo(JT808_LINK, dynamicParam.jt808sn, dynamicParam.jt808isRegister, dynamicParam.jt808AuthCode, dynamicParam.jt808AuthLen);
                    jt808SendToServer(JT808_LINK, TERMINAL_AUTH, NULL);
                }
            }
            break;
        case JT808_NORMAL:
            if (++jt808ServConn.hbtTick >= sysparam.heartbeatgap)
            {
                jt808ServConn.hbtTick = 0;
                queryBatVoltage();
                LogMessage(DEBUG_ALL, "Terminal heartbeat");
                jt808SendToServer(JT808_LINK, TERMINAL_HEARTBEAT, NULL);
                if (timeOutId == -1)
                {
                    timeOutId = startTimer(80, moduleRspTimeout, 0);
                }

                if (hbtTimeOutId == -1)
                {
                    hbtTimeOutId = startTimer(1800, hbtRspTimeOut, 0);
                }
            }
            if (getTcpNack())
            {
                querySendData(JT808_LINK);
            }
            if (jt808ServConn.runTick % 3 == 0)
            {
                //����ram�ٴ��ļ�ϵͳ
                if (getTcpNack() == 0)
                {
                    if (dbUpload() == 0)
                    {
                        gpsRestoreUpload();
                    }
                }
            }
            break;
        default:
            jt808ServerChangeFsm(JT808_AUTHENTICATION);
            break;
    }
    jt808ServConn.runTick++;
}

/**************************************************
@bref		��Ӵ���¼�Ĵ��豸��Ϣ
@param
@return
@note
	SN:999913436051195,292,3.77,46
**************************************************/

void bleServerAddInfo(bleInfo_s dev)
{
    bleInfo_s *next;
    if (bleHead == NULL)
    {
        bleHead = malloc(sizeof(bleInfo_s));
        if (bleHead != NULL)
        {
            strncpy(bleHead->imei, dev.imei, 15);
            bleHead->imei[15] = 0;
            bleHead->startCnt = dev.startCnt;
            bleHead->vol = dev.vol;
            bleHead->batLevel = dev.batLevel;
            bleHead->next = NULL;
        }
        return;
    }
    next = bleHead;
    while (next != NULL)
    {
        if (next->next == NULL)
        {
            next->next = malloc(sizeof(bleInfo_s));
            if (next->next != NULL)
            {
                next = next->next;

                strncpy(next->imei, dev.imei, 15);
                next->imei[15] = 0;
                next->startCnt = dev.startCnt;
                next->vol = dev.vol;
                next->batLevel = dev.batLevel;
                next->next = NULL;
                next = next->next;
            }
            else
            {
                break;
            }
        }
        else
        {
            next = next->next;
        }
    }
}
/**************************************************
@bref		��ʾ�����Ӷ���
@param
@return
@note
**************************************************/

void showBleList(void)
{
    uint8_t cnt;
    bleInfo_s *dev;
    dev = bleHead;
    cnt = 0;
    while (dev != NULL)
    {
        LogPrintf(DEBUG_ALL, "Dev[%d]:%s", ++cnt, dev->imei);
        dev = dev->next;
    }
}

/**************************************************
@bref		������������·��¼��
@param
@return
@note
**************************************************/

void bleSerLoginReady(void)
{
    bleServConn.fsmstate = SERV_READY;
    bleServConn.heartbeattick = 0;
}
/**************************************************
@bref		socket���ݽ���
@param
@return
@note
**************************************************/

static void bleServerSocketRecv(char *data, uint16_t len)
{
    uint16_t i, beginindex, contentlen, lastindex;
    //������Ѱ��Э��ͷ
    for (i = 0; i < len; i++)
    {
        beginindex = i;
        if (data[i] == 0x78)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x78)
            {
                continue ;
            }
            if (i + 2 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2];
            if ((i + 5 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 3 + contentlen] == 0x0D && data[i + 4 + contentlen] == 0x0A)
            {
                i += (4 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7878[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(BLE_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
        else if (data[i] == 0x79)
        {
            if (i + 1 >= len)
            {
                continue ;
            }
            if (data[i + 1] != 0x79)
            {
                continue ;
            }
            if (i + 3 >= len)
            {
                continue ;
            }
            contentlen = data[i + 2] << 8 | data[i + 3];
            if ((i + 6 + contentlen) > len)
            {
                continue ;
            }
            if (data[i + 4 + contentlen] == 0x0D && data[i + 5 + contentlen] == 0x0A)
            {
                i += (5 + contentlen);
                lastindex = i + 1;
                //LogPrintf(DEBUG_ALL, "Fint it ====>Begin:7979[%d,%d]", beginindex, lastindex - beginindex);
                protocolRxParser(BLE_LINK, (char *)data + beginindex, lastindex - beginindex);
            }
        }
    }
}

/**************************************************
@bref		ble������λ
@param
@return
@note
**************************************************/

void bleServerReconnect(void)
{
	LogMessage(DEBUG_ALL, "ble reconnect server");
    socketDel(BLE_LINK);
    moduleSleepCtl(0);
}

/**************************************************
@bref		�������ӷ���������
@param
@return
@note
**************************************************/

void bleServerConnTask(void)
{
    static uint8_t tick = 0;
    bleInfo_s *next;
    gpsinfo_s *gpsinfo;
    if (isModuleRunNormal() == 0 || bleHead == NULL)
    {
        if (gpsRequestGet(GPS_REQUEST_BLE))
        {
            gpsRequestClear(GPS_REQUEST_BLE);
        }
        return ;
    }
    if (socketGetUsedFlag(BLE_LINK) != 1)
    {
        bleServConn.loginCount = 0;
        bleServConn.fsmstate = SERV_LOGIN;
        socketAdd(BLE_LINK, sysparam.bleServer, sysparam.bleServerPort, bleServerSocketRecv);
        return;
    }
    if (socketGetConnStatus(BLE_LINK) != SOCKET_CONN_SUCCESS)
    {
    	bleServConn.fsmstate = SERV_LOGIN;
        return;
    }

    switch (bleServConn.fsmstate)
    {
        case SERV_LOGIN:
            tick = 0;
            LogMessage(DEBUG_ALL, "ble try login...");
            gpsRequestSet(GPS_REQUEST_BLE);
            protocolSnRegister(bleHead->imei);
            protocolSend(BLE_LINK, PROTOCOL_01, NULL);
            bleServConn.fsmstate = SERV_LOGIN_WAIT;
            bleServConn.logintick = 0;
            break;
        case SERV_LOGIN_WAIT:
            bleServConn.logintick++;
            if (bleServConn.logintick >= 30)
            {
                bleServConn.fsmstate = SERV_LOGIN;
                bleServConn.loginCount++;
                if (bleServConn.loginCount >= 2)
                {
                    bleServConn.fsmstate = SERV_END;
                }
            }
            break;
        case SERV_READY:
            if (bleServConn.heartbeattick++ == 0)
            {
                protocolInfoResiter(bleHead->batLevel, sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage, bleHead->startCnt, 0);
                protocolSend(BLE_LINK, PROTOCOL_13, NULL);
                lbsRequestSet(DEV_EXTEND_OF_BLE);
                
            }
            gpsinfo = getCurrentGPSInfo();

            if (gpsinfo->fixstatus)
            {
                if (tick++ >= 10)
                {
                    protocolSend(BLE_LINK, PROTOCOL_12, gpsinfo);
                    bleServConn.fsmstate = SERV_END;
                    break;
                }
            }
            else
            {
                tick = 0;
                if (bleServConn.heartbeattick == 30)
                {
					wifiRequestSet(DEV_EXTEND_OF_BLE);
                }
            }
            if (bleServConn.heartbeattick >= 180)
            {
                protocolSend(BLE_LINK, PROTOCOL_12, getLastFixedGPSInfo());
                bleServConn.fsmstate = SERV_END;
            }
            break;
        case SERV_END:
            next = bleHead->next;
            free(bleHead);
            bleHead = next;
            socketDel(BLE_LINK);
            bleServConn.fsmstate = SERV_LOGIN;
            LogPrintf(DEBUG_ALL, "ble server done");
            break;
        default:
            bleServConn.fsmstate = SERV_LOGIN;
            bleServConn.heartbeattick = 0;
            break;
    }
}

/**************************************************
@bref		jt808���ݽ��ջص�
@param
	none
@return
	none
@note
**************************************************/

static void bleJt808ServerSocketRecv(char *rxbuf, uint16_t len)
{
    jt808ReceiveParser(BLE_LINK, (uint8_t *)rxbuf, len);
}


/**************************************************
@bref		blejt808״̬���л�״̬
@param
	nfsm	��״̬
@return
	none
@note
**************************************************/

static void bleJt808ServerChangeFsm(jt808_connfsm_s nfsm)
{
    bleJt808SerConn.connectFsm = nfsm;
    bleJt808SerConn.runTick = 0;
}

/**************************************************
@bref       blejt808��Ȩ�ɹ��ظ�
@param
@return
@note
**************************************************/

void bleJt808ServerAuthSuccess(void)
{
    bleJt808SerConn.authCnt = 0;
    bleJt808SerConn.hbtTick = 0;
    bleJt808ServerChangeFsm(JT808_NORMAL);
}


/**************************************************
@bref		��������808����������
@param
@return
@note
**************************************************/
void bleJt808ServerConnTask(void)
{
	static uint16_t unConnTick = 0;
	static uint8_t tick = 0;
	gpsinfo_s *gpsinfo;
	bleInfo_s *next;
    if (isModuleRunNormal() == 0 || bleHead == NULL)
    {
        if (gpsRequestGet(GPS_REQUEST_BLE))
        {
            gpsRequestClear(GPS_REQUEST_BLE);
        }
        return ;
    }
    if (socketGetUsedFlag(BLE_LINK) != 1)
    {
    	bleJt808InfoInit();
    	unConnTick = 0;
    	tick = 0;
    	bleJt808ServerChangeFsm(JT808_REGISTER);
        jt808RegisterTcpSend(BLE_LINK, jt808ServerSocketSend);
        jt808RegisterManufactureId(BLE_LINK, (uint8_t *)"ZT");
        jt808RegisterTerminalType(BLE_LINK, (uint8_t *)"02");
        jt808RegisterTerminalId(BLE_LINK, (uint8_t *)"01");
       
        socketAdd(BLE_LINK, sysparam.jt808Server, sysparam.jt808Port, bleJt808ServerSocketRecv);
        return;
    }
    if (socketGetConnStatus(BLE_LINK) != SOCKET_CONN_SUCCESS)
    {
		bleJt808ServerChangeFsm(JT808_REGISTER);
		if (unConnTick++ >= 900)
		{
			/*���bleHead*/
		}
		return;
    }
    switch (bleJt808SerConn.connectFsm)
    {
    	//ÿ�ζ�ע��
		case JT808_REGISTER:
			if (bleJt808RegisterGet())
			{
				bleJt808ServerChangeFsm(JT808_AUTHENTICATION);
				bleJt808SerConn.regCnt = 0;
			}
			else
			{
				if (bleJt808SerConn.runTick % 60 == 0)
				{
					if (bleJt808SerConn.regCnt++ > 3)
					{
						LogMessage(DEBUG_ALL, "Ble terminal register timeout");
						bleJt808SerConn.regCnt = 0;
						bleServerReconnect();
					}
					else
					{
						LogMessage(DEBUG_ALL, "Ble terminal register");
						jt808RegisterLoginInfo(BLE_LINK, bleHead->imei, 0, NULL, 0);
						jt808RegisterBleInfo(BLE_LINK, bleHead->batLevel, bleHead->vol, bleHead->startCnt);
						jt808SendToServer(BLE_LINK, TERMINAL_REGISTER, NULL);
						gpsRequestSet(GPS_REQUEST_BLE);
					}
				}
				break;
			}
		
		case JT808_AUTHENTICATION:
			if (bleJt808SerConn.runTick % 60 == 0)
			{
				if (bleJt808SerConn.authCnt++ > 3)
				{
					bleJt808SerConn.authCnt = 0;
					LogMessage(DEBUG_ALL, "Ble terminal authentication timeout");
					bleServerReconnect();
				}
				else
				{
					LogMessage(DEBUG_ALL, "Ble terminal authentication");
					jt808SendToServer(BLE_LINK, TERMINAL_AUTH, NULL);
				}
			}
			break;
		case JT808_NORMAL:
			if (bleJt808SerConn.hbtTick++ == 0)
			{
				lbsRequestSet(DEV_EXTEND_OF_BLE);
                
				LogMessage(DEBUG_ALL, "Ble Terminal heartbeat");
                jt808SendToServer(BLE_LINK, TERMINAL_HEARTBEAT, NULL);
			}
			gpsinfo = getCurrentGPSInfo();
			if (gpsinfo->fixstatus)
			{
				if (tick++ >= 10)
				{
					jt808SendToServer(BLE_LINK, TERMINAL_POSITION, gpsinfo);
					bleJt808ServerChangeFsm(JT808_END);
					break;
				}
			}
			else
			{
				tick = 0;
				if (bleJt808SerConn.hbtTick == 30)
                {
					wifiRequestSet(DEV_EXTEND_OF_BLE);
                }
			}
			if (bleJt808SerConn.hbtTick >= 180)
			{
				jt808SendToServer(BLE_LINK, TERMINAL_POSITION, getLastFixedGPSInfo());
				bleJt808ServerChangeFsm(JT808_END);
			}
			break;
		case JT808_END:
			next = bleHead->next;
			free(bleHead);
			bleHead = next;
			socketDel(BLE_LINK);
			bleJt808ServerChangeFsm(JT808_REGISTER);
			LogMessage(DEBUG_ALL, "ble server done");
			break;
		default:
			bleJt808ServerChangeFsm(JT808_REGISTER);
			bleJt808SerConn.hbtTick = 0;
			break;
    }
	bleJt808SerConn.runTick++;
}


void ble2ServerConnTask(void)
{
	deviceConnInfo_s *devInfo = bleDevGetInfoBySockid(BLE2_LINK);
	
	if (devInfo == NULL || devInfo->use == 0)
	{
		if (socketGetUsedFlag(BLE2_LINK) == 1)
        {
            LogMessage(DEBUG_ALL, "Ble[2] server lost");
            socketDel(BLE2_LINK);
        }
		return;
	}
	if (socketGetUsedFlag(BLE2_LINK) == 0)
	{
		ble2ServConn.fsmstate = SERV_LOGIN;
		ble2ServConn.loginCount = 0;
		socketAdd(BLE2_LINK, sysparam.bleServer, sysparam.bleServerPort, bleServerSocketRecv);
		return;
	}

	if (socketGetConnStatus(BLE2_LINK) != SOCKET_CONN_SUCCESS)
	{
		ble2ServConn.fsmstate = SERV_LOGIN;
		return;
	}
	switch (ble2ServConn.fsmstate)
	{
		case SERV_LOGIN:
		    protocolSnRegister(bleHead->imei);
            protocolSend(BLE_LINK, PROTOCOL_01, NULL);
			break;
		case SERV_LOGIN_WAIT:
			ble2ServConn.logintick++;
            if (ble2ServConn.logintick >= 30)
            {
                ble2ServConn.fsmstate = SERV_LOGIN;
                ble2ServConn.loginCount++;
                if (ble2ServConn.loginCount >= 2)
                {
                    ble2ServConn.fsmstate = SERV_LOGIN;
                }
            }
			break;
		case SERV_READY:
            if (ble2ServConn.heartbeattick % sysparam.heartbeatgap == 0)
            {
                ble2ServConn.heartbeattick = 0;

                protocolInfoResiter(getBatteryLevel(), sysinfo.outsidevoltage > 5.0 ? sysinfo.outsidevoltage : sysinfo.insidevoltage,
                                    dynamicParam.startUpCnt, dynamicParam.runTime);
                protocolSend(NORMAL_LINK, PROTOCOL_13, NULL);
            }
            ble2ServConn.heartbeattick++;

            break;

	}
}

/**************************************************
@bref		agps����
@param
	none
@return
	none
@note
**************************************************/

void agpsRequestSet(void)
{
    sysinfo.agpsRequest = 1;
    LogMessage(DEBUG_ALL, "agpsRequestSet==>OK");
}

void agpsRequestClear(void)
{
    sysinfo.agpsRequest = 0;
    LogMessage(DEBUG_ALL, "agpsRequestClear==>OK");
}

/**************************************************
@bref		socket���ݽ���
@param
@return
@note
**************************************************/

static void agpsSocketRecv(char *data, uint16_t len)
{
    LogPrintf(DEBUG_ALL, "Agps Reject %d Bytes", len);
    //LogMessageWL(DEBUG_ALL, data, len);
    portUartSend(&usart3_ctl, data, len);
}



static void agpsServerConnTask(void)
{
    static uint8_t agpsFsm = 0;
    static uint8_t runTick = 0;
    char agpsBuff[150] = {0};
    uint16_t agpsLen;
    int ret;
    gpsinfo_s *gpsinfo;

    if (sysparam.agpsen == 0)
    {
		sysinfo.agpsRequest = 0;
		agpsFsm = 0;
    	if (socketGetUsedFlag(AGPS_LINK))
    	{
			socketDel(AGPS_LINK);
    	}
		return;
    }
    if (sysinfo.agpsRequest == 0)
    {
    	agpsFsm = 0;
    	if (socketGetUsedFlag(AGPS_LINK))
    	{
			socketDel(AGPS_LINK);
    	}
        return;
    }

    gpsinfo = getCurrentGPSInfo();

    if (isModuleRunNormal() == 0)
    {
        return ;
    }
    if (gpsinfo->fixstatus || sysinfo.gpsOnoff == 0)
    {
        socketDel(AGPS_LINK);
        agpsRequestClear();
        return;
    }
    if (socketGetUsedFlag(AGPS_LINK) != 1)
    {
        agpsFsm = 0;
        ret = socketAdd(AGPS_LINK, sysparam.agpsServer, sysparam.agpsPort, agpsSocketRecv);
        if (ret != 1)
        {
            LogPrintf(DEBUG_ALL, "agps add socket err[%d]", ret);
            agpsRequestClear();
        }
        return;
    }
    if (socketGetConnStatus(AGPS_LINK) != SOCKET_CONN_SUCCESS)
    {
    	agpsFsm = 0;
        LogMessage(DEBUG_ALL, "wait agps server ready");
        return;
    }
    switch (agpsFsm)
    {
        case 0:
//            if (gpsinfo->fixstatus == 0)
//            {
//                sprintf(agpsBuff, "user=%s;pwd=%s;cmd=full;", sysparam.agpsUser, sysparam.agpsPswd);
//                socketSendData(AGPS_LINK, (uint8_t *) agpsBuff, strlen(agpsBuff));
//            }
			createProtocolA0(agpsBuff, &agpsLen);
			socketSendData(AGPS_LINK, (uint8_t *) agpsBuff, agpsLen);
            agpsFsm = 1;
            runTick = 0;
            break;
        case 1:
            if (++runTick >= 30)
            {
            	if (isAgpsDataRecvComplete() == 0)
            	{
	                agpsFsm = 0;
	                runTick = 0;
	                socketDel(AGPS_LINK);
	                wifiRequestSet(DEV_EXTEND_OF_MY);
	                agpsRequestClear();
                }
            }
            break;
        default:
            agpsFsm = 0;
            break;
    }


}

/**************************************************
@bref		���������ӹ�������
@param
@return
@note
**************************************************/

void serverManageTask(void)
{
    if (sysparam.protocol == JT808_PROTOCOL_TYPE)
    {
        jt808ServerConnTask();
        bleJt808ServerConnTask();
    }
    else
    {
        privateServerConnTask();
        bleServerConnTask();
    }   
    hiddenServerConnTask();
    agpsServerConnTask();
}

/**************************************************
@bref		�ж���Ҫ�������Ƿ��¼����
@param
@return
@note
**************************************************/

uint8_t primaryServerIsReady(void)
{
    if (isModuleRunNormal() == 0)
        return 0;
    if (sysparam.protocol == JT808_PROTOCOL_TYPE)
    {
        if (socketGetConnStatus(JT808_LINK) != SOCKET_CONN_SUCCESS)
            return 0;
        if (jt808ServConn.connectFsm != JT808_NORMAL)
            return 0;
    }
    else
    {
        if (socketGetConnStatus(NORMAL_LINK) != SOCKET_CONN_SUCCESS)
            return 0;
        if (privateServConn.fsmstate != SERV_READY)
            return 0;
    }
    return 1;
}

/**************************************************
@bref		�ж����ط������Ƿ��¼����
@param
@return
@note
**************************************************/

uint8_t hiddenServerIsReady(void)
{
    if (isModuleRunNormal() == 0)
        return 0;
    if (socketGetConnStatus(HIDDEN_LINK) != SOCKET_CONN_SUCCESS)
        return 0;
    if (hiddenServConn.fsmstate != SERV_READY)
        return 0;
    return 1;
}

