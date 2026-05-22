// WorkerConnectionPool
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
// Core
#include "Core/Network/TCPConnectionPool.h"

// Forward Declarations
//------------------------------------------------------------------------------
namespace Protocol
{
    class IMessage;
    class MsgRequestWorkerList;
    class MsgSetWorkerStatus;
    class MsgWorkerList;
}

// WorkerInfo
//------------------------------------------------------------------------------
struct WorkerInfo
{
    WorkerInfo( uint32_t address, uint32_t protocolVersionMajor, uint8_t protocolVersionMinor, uint8_t platform )
        : m_Address( address )
        , m_ProtocolVersionMajor( protocolVersionMajor )
        , m_ProtocolVersionMinor( protocolVersionMinor )
        , m_Platform( platform )
    {
    }

    bool operator==( uint32_t address ) const { return address == m_Address; }

    uint32_t m_Address;
    uint32_t m_ProtocolVersionMajor;
    uint8_t m_ProtocolVersionMinor;
    uint8_t m_Platform;
};

// WorkerConnectionPool
//------------------------------------------------------------------------------
class WorkerConnectionPool : public TCPConnectionPool
{
public:
    WorkerConnectionPool();
    virtual ~WorkerConnectionPool();

private:
    // Network events happen on connection threads, but TCPConnectionPool
    // serializes callbacks for us.
    virtual void OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory ) override;
    virtual void OnConnected( const ConnectionInfo * connection ) override;
    virtual void OnDisconnected( const ConnectionInfo * connection ) override;

    void Process( const ConnectionInfo * connection, const Protocol::MsgRequestWorkerList * msg );
    void Process( const ConnectionInfo * connection, const Protocol::MsgWorkerList * msg, const void * payload, size_t payloadSize );
    void Process( const ConnectionInfo * connection, const Protocol::MsgSetWorkerStatus * msg );

    Mutex m_Mutex;
    Array<WorkerInfo> m_Workers;
    const Protocol::IMessage * m_CurrentMessage;
};

//------------------------------------------------------------------------------
