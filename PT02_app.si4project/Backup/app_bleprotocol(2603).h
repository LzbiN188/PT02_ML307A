/*
 * app_bleprotocol.h
 *
 *  Created on: Oct 28, 2023
 *      Author: nimo
 */

#ifndef TASK_INC_APP_BLEPROTOCOL_H_
#define TASK_INC_APP_BLEPROTOCOL_H_

#include <config.h>


/**
 * �豸���Ϻ�����ͨ��CMD_DEV_LOGIN_INFOѯ�ʴӻ���SN��
 * �������յ�sn�ź��Ȳ鿴��·���Ƿ��������ø�sn�ŵ���·
 * �оͱ�ʾ��·����������ֻ����������·�Ͽ���
   ����socksuccess�Ƿ�==0��0��ʾû���ӣ�1��ʾ��ͨ�����PT02����
 * ����һ��ȷ��:1.��·���޸�SN����������2.�п�����·����ѱ���SN��ͨ��CMD_DEV_MASTER_INFO���͸�PT13����socksuccess��1��������·�Ƿ�ɹ���¼��
 * �����������������û�յ�PT13�Ļظ����Ͽ���·���������·����
 * 
 */

/* ����ͨѶЭ�� */
#define CMD_DEV_LOGIN_INFO                  0x30  //��ȡ������¼��Ϣ
#define CMD_DEV_HEARTBEAT					0x31
#define CMD_DEV_MASTER_INFO					0x32  //����״̬


/* ����������Э�� */
#define BLE_SEND_LOGININFO_EVENT            0x00000001 //�豸�ϱ���Ϣ
#define BLE_SEND_HBT_EVNET					0x00000002
#define BLE_SEND_MASTERINFO_EVENT			0x00000004

#define BLE_SEND_ALL_EVENT					0xFFFFFFFF



void bleProtoclRecvParser(uint8_t connHandle, uint8_t *data, uint8_t len);
void bleProtocolSendEventTask(void);

void bleSendDataReqSet(uint8_t ind, uint32_t req);
void bleSendDataReqClear(uint8_t ind, uint32_t req);
void bleProtocolSendPeriod(void);


#endif /* TASK_INC_APP_BLEPROTOCOL_H_ */
