/*
 * app_bleprotocol.h
 *
 *  Created on: Oct 28, 2023
 *      Author: nimo
 */

#ifndef TASK_INC_APP_BLEPROTOCOL_H_
#define TASK_INC_APP_BLEPROTOCOL_H_

#include <config.h>

#define BLE_CONNECT_LIST_SIZE   2

/* ����ͨѶЭ�� */
#define CMD_DEV_INFO_PARAM                  0x30  //�����ϱ���Ϣ
#define CMD_DEV_HEARTBEAT					0x31  //����


/* ����������Э�� */
#define BLE_SEND_INFO_EVENT                 0x00000001 //�豸�ϱ���Ϣ



void bleProtoclRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);

#endif /* TASK_INC_APP_BLEPROTOCOL_H_ */
