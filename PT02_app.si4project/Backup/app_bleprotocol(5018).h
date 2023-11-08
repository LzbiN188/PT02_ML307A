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
#define CMD_DEV_INFO_PARAM                  0x30  //�����ϱ���Ϣ
#define CMD_DEV_HEARTBEAT					0x31  //����


/* ����������Э�� */
#define BLE_SEND_INFO_EVENT                 0x00000001 //�豸�ϱ���Ϣ



void bleProtoclRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);
void bleProtocolSendEventTask(void);

void bleSendDataReqSet(uint8_t ind, uint32_t req);
void bleSendDataReqClear(uint8_t ind, uint32_t req);
void bleProtocolSendPeriod(void);


#endif /* TASK_INC_APP_BLEPROTOCOL_H_ */
