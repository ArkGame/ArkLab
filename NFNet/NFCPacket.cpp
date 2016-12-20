#include "NFCPacket.h"

size_t NFCPacket::DeCode(const char* strData, const size_t unLen)
{
    //解密--unLen为buff总长度,解包时能用多少是多少
    if(unLen < pHead->GetHeadLength())
    {
        //长度不够
        return -1;
    }


    if(pHead->GetHeadLength() != pHead->DeCode(strData))
    {
        //取包头失败
        return -2;
    }

    //printf("Recv - MsgID:%d \n", pHead->GetMsgID());

    if(pHead->GetMsgLength() > unLen)
    {
        //总长度不够
        return 0;
    }

    //copy包头+包体
    mstrPackData.append(strData, pHead->GetMsgLength());

    //返回使用过的量
    return pHead->GetMsgLength();
}

size_t NFCPacket::EnCode(char* pHeadBuffer, const char* strData, const size_t unLen)
{
    //加密
    //虽多一次copy，但是可以在外加解密而不耦合库
    if(unLen + pHead->GetHeadLength() > MsgHead::NF_MSG_MAX_LENGTH)
    {
        // 长度过大
        return 0;
    }

    mstrPackData.clear();
    mstrPackData.append(pHeadBuffer, pHead->GetHeadLength());
    mstrPackData.append(strData, unLen);

    return unLen + pHead->GetHeadLength();
}
