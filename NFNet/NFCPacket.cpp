#include "NFCPacket.h"

size_t NFCPacket::DeCode(const char* strData, const size_t unLen)
{
    //����--unLenΪbuff�ܳ���,���ʱ���ö����Ƕ���
    if(unLen < pHead->GetHeadLength())
    {
        //���Ȳ���
        return -1;
    }


    if(pHead->GetHeadLength() != pHead->DeCode(strData))
    {
        //ȡ��ͷʧ��
        return -2;
    }

    //printf("Recv - MsgID:%d \n", pHead->GetMsgID());

    if(pHead->GetMsgLength() > unLen)
    {
        //�ܳ��Ȳ���
        return 0;
    }

    //copy��ͷ+����
    mstrPackData.append(strData, pHead->GetMsgLength());

    //����ʹ�ù�����
    return pHead->GetMsgLength();
}

size_t NFCPacket::EnCode(char* pHeadBuffer, const char* strData, const size_t unLen)
{
    //����
    //���һ��copy�����ǿ�������ӽ��ܶ�����Ͽ�
    if(unLen + pHead->GetHeadLength() > MsgHead::NF_MSG_MAX_LENGTH)
    {
        // ���ȹ���
        return 0;
    }

    mstrPackData.clear();
    mstrPackData.append(pHeadBuffer, pHead->GetHeadLength());
    mstrPackData.append(strData, unLen);

    return unLen + pHead->GetHeadLength();
}
