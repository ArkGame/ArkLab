// -------------------------------------------------------------------------
//    @FileName         ：    NFIPacket.h
//    @Author           ：    Nick Yang
//    @Date             ：    2013-12-15
//    @Module           ：    NFIPacket
//    @Desc             :     Net Packet
// -------------------------------------------------------------------------

#ifndef __NFI_PACKET_H__
#define __NFI_PACKET_H__

#include <cstring>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>
#include <assert.h>

#ifdef _MSC_VER

#include <WinSock2.h>
#include <winsock.h>
#else

#include <arpa/inet.h>
#include <netinet/in.h>

#endif

#include <string>
#include <iostream>
//#include "NFComm/NFCore/NFGUID.h"
#include "common/NFPlatform.h"
//#include "NFComm/NFCore/NFIDataList.h"

#pragma pack(push, 1)

struct IMsgHead
{
    enum NF_Head
    {
        NF_HEAD_LENGTH      = 8,
        NF_SS_HEAD_LENGTH   = 11,
        NF_IDIP_HEAD_LENGTH = 172,

        NF_MSG_MAX_LENGTH   = 512 * 1024, // 协议长度最大限制为512KB
    };

    virtual ~IMsgHead() {}

    virtual int EnCode(char* strData) = 0;
    virtual int DeCode(const char* strData) = 0;

    virtual size_t GetHeadLength() const = 0;

    virtual uint32_t GetMsgID() const = 0;
    virtual void SetMsgID(uint32_t nMsgID) = 0;

    virtual size_t GetMsgLength() const = 0;
    virtual void SetMsgLength(size_t nLength) = 0;

    int64_t NF_HTONLL(int64_t nData)
    {
#ifdef _MSC_VER
        return htonll(nData);
#else
        return htobe64(nData);
#endif
    }

    int64_t NF_NTOHLL(int64_t nData)
    {
#ifdef _MSC_VER
        return ntohll(nData);
#else
        return be64toh(nData);
#endif
    }

    int32_t NF_HTONL(int32_t nData)
    {
#ifdef _MSC_VER
        return htonl(nData);
#else
        return htobe32(nData);
#endif
    }

    int32_t NF_NTOHL(int32_t nData)
    {
#ifdef _MSC_VER
        return ntohl(nData);
#else
        return be32toh(nData);
#endif
    }

    int16_t NF_HTONS(int16_t nData)
    {
#ifdef _MSC_VER
        return htons(nData);
#else
        return htobe16(nData);
#endif
    }

    int16_t NF_NTOHS(int16_t nData)
    {
#ifdef _MSC_VER
        return ntohs(nData);
#else
        return be16toh(nData);
#endif
    }
};

// server <-> server msg head
class MsgHead : public IMsgHead
{
public:
    MsgHead()
    {
        mnSize = 0;
        mnMsgID = 0;
        mnData1 = 0;
        mnData2 = 0;
    }



    virtual size_t GetHeadLength() const
    {
        return NF_SS_HEAD_LENGTH;
    }

    virtual int EnCode(char* strData)
    {
        uint16_t nOffset = 0;

        uint32_t nSize = NF_HTONL(mnSize);
        memcpy(strData + nOffset, (void*)(&nSize), sizeof(nSize));
        nOffset += sizeof(nSize);

        uint16_t nType = NF_HTONS(mnMsgID);
        memcpy(strData + nOffset, (void*)(&nType), sizeof(nType));
        nOffset += sizeof(nType);

        memcpy(strData + nOffset, (void*)(&mnData1), sizeof(mnData1));
        nOffset += sizeof(mnData1);

        uint32_t nSrcID = NF_HTONL(mnData2);
        memcpy(strData + nOffset, (void*)(&nSrcID), sizeof(nSrcID));
        nOffset += sizeof(nSrcID);

        if(nOffset != GetHeadLength())
        {
            assert(0);
        }

        return nOffset;
    }

    virtual int DeCode(const char* strData)
    {
        uint16_t nOffset = 0;

        uint32_t nSize = 0;
        memcpy(&nSize, strData + nOffset, sizeof(nSize));
        mnSize = NF_NTOHL(nSize);
        nOffset += sizeof(nSize);

        uint16_t nType = 0;
        memcpy(&nType, strData + nOffset, sizeof(nType));
        mnMsgID = NF_NTOHS(nType);
        nOffset += sizeof(nType);

        uint8_t nData1 = 0;
        memcpy(&nData1, strData + nOffset, sizeof(nData1));
        mnData1 = nData1;
        nOffset += sizeof(nData1);

        uint32_t nData2 = 0;
        memcpy(&nData2, strData + nOffset, sizeof(nData2));
        mnData2 = NF_NTOHL(nData2);
        nOffset += sizeof(nData2);

        if(nOffset != GetHeadLength())
        {
            assert(0);
        }

        return nOffset;
    }

    virtual uint32_t GetMsgID() const
    {
        return mnMsgID;
    }

    virtual void SetMsgID(uint32_t nMsgID)
    {
        mnMsgID = nMsgID;
    }

    virtual size_t GetMsgLength() const
    {
        return mnSize;
    }

    virtual void SetMsgLength(size_t nLength)
    {
        mnSize = nLength;
    }

public:
    uint32_t mnSize;    //消息长度
    uint16_t mnMsgID;   //枚举见MsgID.proto
    uint8_t  mnData1;   //如果是服务器类型,枚举见NFDefine.proto的ServerType; 如果是Client发往网关，则为序列号
    uint32_t mnData2;   //如果是Client发送过来则是CRC32校验码; 如果是服务器内部发往内部，则为服务器ID
};

// idip <-> server msg head
class MsgIDIPHead : public IMsgHead
{
public:
    MsgIDIPHead()
    {
        uiPacketLen = 0;                            /* 包长 *///消息总长度(含消息头及消息体)
        uiCmdid = 0;                                /* 命令ID */
        uiSeqid = 0;                                /* 流水号 */
        memset(szServiceName, 0, 16);               /* 服务名 */
        uiSendTime = 0;                             /* 发送时间*/
        uiVersion = 0;                              /* 版本号 */
        memset(ucAuthenticate, 0, 32);              /* 加密串 */
        uiResult = 0;                               /* 错误码 */
        memset(szRetErrMsg, 0, 100);                /* 错误信息 */
    }

    virtual int EnCode(char* strData)
    {
        uint16_t nOffset = 0;

        uint32_t nSize = NF_HTONL(uiPacketLen);
        memcpy(strData + nOffset, &nSize, sizeof(nSize));
        nOffset += sizeof(nSize);

        uint32_t nType = NF_HTONL(uiCmdid);
        memcpy(strData + nOffset, &nType, sizeof(nType));
        nOffset += sizeof(nType);

        uint32_t nSeq = NF_HTONL(uiSeqid);
        memcpy(strData + nOffset, &nSeq, sizeof(nSeq));
        nOffset += sizeof(nSeq);

        memcpy(strData + nOffset, szServiceName, 16);
        nOffset += 16;

        uint32_t nSendTime = NF_HTONL(uiSendTime);
        memcpy(strData + nOffset, &nSendTime, sizeof(nSendTime));
        nOffset += sizeof(nSendTime);

        uint32_t nVersion = NF_HTONL(uiVersion);
        memcpy(strData + nOffset, &nVersion, sizeof(nVersion));
        nOffset += sizeof(nVersion);

        memcpy(strData + nOffset, ucAuthenticate, 32);
        nOffset += 32;

        uint32_t nResult = NF_HTONL(uiResult);
        memcpy(strData + nOffset, &nResult, sizeof(nResult));
        nOffset += sizeof(nResult);

        memcpy(strData + nOffset, szRetErrMsg, 100);
        nOffset += 100;

        if(nOffset != GetHeadLength())
        {
            assert(0);
        }

        return nOffset;
    }

    virtual int DeCode(const char* strData)
    {
        uint16_t nOffset = 0;

        uint32_t nSize = 0;
        memcpy(&nSize, strData + nOffset, sizeof(nSize));
        uiPacketLen = NF_NTOHL(nSize);
        nOffset += sizeof(nSize);

        uint32_t nType = 0;
        memcpy(&nType, strData + nOffset, sizeof(nType));
        uiCmdid = NF_NTOHL(nType);
        nOffset += sizeof(nType);

        uint32_t nSeq = 0;
        memcpy(&nSeq, strData + nOffset, sizeof(nSeq));
        uiSeqid = NF_NTOHL(nSeq);
        nOffset += sizeof(nSeq);

        memcpy(szServiceName, strData + nOffset, 16);
        nOffset += 16;

        uint32_t nSendTime = 0;
        memcpy(&nSendTime, strData + nOffset, sizeof(nSendTime));
        uiSendTime = NF_NTOHL(nSendTime);
        nOffset += sizeof(nSendTime);

        uint32_t nVersion = 0;
        memcpy(&nVersion, strData + nOffset, sizeof(nVersion));
        uiVersion = NF_NTOHL(nVersion);
        nOffset += sizeof(nVersion);

        memcpy(ucAuthenticate, strData + nOffset, 32);
        nOffset += 32;

        uint32_t nResult = 0;
        memcpy(&nResult, strData + nOffset, sizeof(nResult));
        uiResult = NF_NTOHL(nResult);
        nOffset += sizeof(nResult);

        memcpy(szRetErrMsg, strData + nOffset, 100);
        nOffset += 100;

        if(nOffset != GetHeadLength())
        {
            assert(0);
        }

        return nOffset;

        return 0;
    }

    virtual size_t GetHeadLength() const
    {
        return NF_IDIP_HEAD_LENGTH;
    }

    virtual uint32_t GetMsgID() const
    {
        return uiCmdid;
    }
    virtual void SetMsgID(uint32_t nMsgID)
    {
        uiCmdid = nMsgID;
    }

    virtual void SetSeqId(uint32_t nSeqid)
    {
        uiSeqid = nSeqid;
    }

    virtual void SetServiceName(const std::string& strName)
    {
        NFSPRINTF(szServiceName, sizeof(szServiceName) - 1, "%s", strName.data());
    }

    virtual void SetAuthenticate(const std::string& strValue)
    {
        NFSPRINTF(ucAuthenticate, sizeof(ucAuthenticate) - 1, "%s", strValue.data());
    }

    virtual void SetVersion(uint32_t nVersion)
    {
        uiVersion = nVersion;
    }

    virtual void SetSendTime(uint32_t nTime)
    {
        uiSendTime = nTime;
    }

    virtual void SetResult(int32_t nResult)
    {
        uiResult = nResult;
    }

    virtual void SetRetErrMsg(const std::string& strMsg)
    {
        NFSPRINTF(szRetErrMsg, sizeof(szRetErrMsg) - 1, "%s", strMsg.data());
    }

    virtual size_t GetMsgLength() const
    {
        return uiPacketLen;
    }

    virtual void SetMsgLength(size_t nLength)
    {
        uiPacketLen = nLength;
    }

    virtual std::string GetServiceName() const
    {
        return std::string(szServiceName, 16);
    }

    virtual uint32_t GetVersion() const
    {
        return uiVersion;
    }

    virtual uint32_t GetSeqId() const
    {
        return uiSeqid;
    }

    virtual uint32_t GetSendTime() const
    {
        return uiSendTime;
    }

    virtual std::string GetAuthenticate() const
    {
        return std::string(ucAuthenticate, 32);
    }

    virtual int32_t GetResult()
    {
        return uiResult;
    }

    virtual std::string GetRetErrMsg()
    {
        return std::string(szRetErrMsg);
    }

private:
    uint32_t  uiPacketLen;          /* 包长 *///消息总长度(含消息头及消息体)
    uint32_t  uiCmdid;              /* 命令ID */
    uint32_t  uiSeqid;              /* 流水号 */
    char        szServiceName[16];  /* 服务名 */
    uint32_t  uiSendTime;           /* 发送时间*/
    uint32_t  uiVersion;            /* 版本号 */
    char        ucAuthenticate[32]; /* 加密串 */
    int32_t    uiResult;            /* 错误码 */
    char        szRetErrMsg[100];   /* 错误信息 */
};

class NFIPacket
{
public:
    void operator = (const NFIPacket& packet)
    {

        this->Construction(packet);
    }

    virtual ~NFIPacket()
    {

    }

    virtual size_t EnCode(char* pHeadBuffer, const char* strData, const size_t unLen) = 0;
    virtual size_t DeCode(const char* strData, const size_t unLen) = 0;

    virtual void Construction(const NFIPacket& packet) = 0;
    virtual const IMsgHead* GetMsgHead() const = 0;
    virtual const char* GetPacketData() const = 0;
    virtual const size_t GetPacketLen() const = 0;
    virtual const size_t GetDataLen() const = 0;
    virtual const char* GetData() const = 0;
    virtual const int GetFd() const = 0;
    virtual void SetFd(const int nFd) = 0;

    virtual void Reset(IMsgHead::NF_Head head) = 0;
};

#pragma pack(pop)

#endif
