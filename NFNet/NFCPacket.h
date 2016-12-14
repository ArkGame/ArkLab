// -------------------------------------------------------------------------
//    @FileName         £º    NFCPacket.h
//    @Author           £º    Nick Yang
//    @Date             £º    2013-12-15
//    @Module           £º    NFIPacket
//    @Desc             :     Net Packet
// -------------------------------------------------------------------------

#ifndef __NFC_PACKET_H__
#define __NFC_PACKET_H__


#include "NFIPacket.h"
#include <string>

#pragma pack(push, 1)

class NFCPacket : public NFIPacket
{
public:
    NFCPacket(MsgHead::NF_Head head)
    {
        munFD = 0;
        switch(head)
        {
        case MsgHead::NF_IDIP_HEAD_LENGTH:
            pHead = new MsgIDIPHead();
            break;
        case MsgHead::NF_SS_HEAD_LENGTH:
            pHead = new MsgHead();
            break;
        default:
            break;
        }
    }

    virtual ~NFCPacket()
    {
        if(pHead)
        {
            delete pHead;
            pHead = NULL;
        }
    }

    virtual void Reset(MsgHead::NF_Head head)
    {
        if(pHead)
        {
            delete pHead;
            pHead = NULL;
        }

        munFD = 0;
        mstrPackData.clear();

        switch(head)
        {
        case MsgHead::NF_IDIP_HEAD_LENGTH:
            pHead = new MsgIDIPHead();
            break;
        case MsgHead::NF_SS_HEAD_LENGTH:
            pHead = new MsgHead();
            break;
        default:
            break;
        }
    }

    virtual const IMsgHead* GetMsgHead() const
    {
        return pHead;
    }

    virtual const uint32_t GetMsgID() const
    {
        return pHead->GetMsgID();
    }

    virtual const char* GetPacketData() const
    {
        return this->mstrPackData.data();//include head
    }

    virtual const size_t GetPacketLen() const
    {
        return pHead->GetMsgLength();
    }

    virtual const size_t GetDataLen() const
    {
        return pHead->GetMsgLength() - pHead->GetHeadLength();
    }

    virtual const char* GetData() const
    {
        return this->mstrPackData.data() + pHead->GetHeadLength();//not include head
    }

    virtual void Construction(const NFIPacket& packet)
    {
        this->mstrPackData.assign(packet.GetPacketData(), this->GetPacketLen());
    }

    virtual const int GetFd() const
    {
        return munFD;
    }

    virtual void SetFd(const int nFd)
    {
        munFD = nFd;
    }

    virtual size_t GetHeadSize()
    {
        return pHead->GetHeadLength();
    }

    //const uint32_t unLen: length of data, not include head
    virtual size_t EnCode(char* pHeadBuffer, const char* strData, const size_t unLen);

    //const uint32_t unLen: length of buff
    virtual size_t DeCode(const char* strData, const size_t unLen);

protected:
private:
    IMsgHead* pHead;
    //char strPackData[NF_MAX_SERVER_PACKET_SIZE];//include head
    std::string mstrPackData;//include head
    int munFD;;
};


#pragma pack(pop)

#endif
