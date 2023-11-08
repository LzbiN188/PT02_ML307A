/*
 * app_bleprotocol.h
 *
 *  Created on: Oct 28, 2023
 *      Author: nimo
 */

#ifndef TASK_INC_APP_BLEPROTOCOL_H_
#define TASK_INC_APP_BLEPROTOCOL_H_

#include <config.h>


/* ����ͨѶЭ�� */
#define CMD_DEV_LOGIN_INFO                  0x30  //��ȡ������¼��Ϣ
#define CMD_DEV_HEARTBEAT					0x31
#define CMD_DEV_MASTER_INFO					0x32  //����״̬


/* ����������Э�� */
#define BLE_SEND_LOGININFO_EVENT            0x00000001 //�豸�ϱ���Ϣ
#define BLE_SEND_HBT_EVNET					0x00000002
#define BLE_SEND_MASTERINFO_EVENT			0x00000004



void bleProtoclRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);
void bleProtocolSendEventTask(void);

void bleSendDataReqSet(uint8_t ind, uint32_t req);
void bleSendDataReqClear(uint8_t ind, uint32_t req);
void bleProtocolSendPeriod(void);


#endif /* TASK_INC_APP_BLEPROTOCOL_H_ */
