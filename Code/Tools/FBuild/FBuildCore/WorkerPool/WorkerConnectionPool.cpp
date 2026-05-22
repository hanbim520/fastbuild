// WorkerConnectionPool
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerConnectionPool.h"
#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/FileIO/ConstMemoryStream.h"
#include "Core/FileIO/MemoryStream.h"
#include "Core/Mem/Mem.h"
#include "Core/Strings/AStackString.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerConnectionPool::WorkerConnectionPool()
    : TCPConnectionPool()
    , m_CurrentMessage( nullptr )
{
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerConnectionPool::~WorkerConnectionPool()
{
    ShutdownAllConnections();
}

// OnReceive
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnReceive( const ConnectionInfo * connection, void * data, uint32_t size, bool & keepMemory )
{
    keepMemory = true; // We take ownership of data and free it below.

    void * payload = nullptr;
    size_t payloadSize = 0;
    if ( m_CurrentMessage == nullptr )
    {
        m_CurrentMessage = static_cast<const Protocol::IMessage *>( data );
        if ( m_CurrentMessage->HasPayload() )
        {
            return;
        }
    }
    else
    {
        ASSERT( m_CurrentMessage->HasPayload() );
        payload = data;
        payloadSize = size;
    }

    const Protocol::IMessage * imsg = m_CurrentMessage;
    const Protocol::MessageType messageType = imsg->GetType();

    PROTOCOL_DEBUG( "Coordinator : %u (%s)\n", messageType, GetProtocolMessageDebugName( messageType ) );

    switch ( messageType )
    {
        case Protocol::MSG_REQUEST_WORKER_LIST:
        {
            const Protocol::MsgRequestWorkerList * msg = static_cast<const Protocol::MsgRequestWorkerList *>( imsg );
            Process( connection, msg );
            break;
        }
        case Protocol::MSG_WORKER_LIST:
        {
            const Protocol::MsgWorkerList * msg = static_cast<const Protocol::MsgWorkerList *>( imsg );
            Process( connection, msg, payload, payloadSize );
            break;
        }
        case Protocol::MSG_SET_WORKER_STATUS:
        {
            const Protocol::MsgSetWorkerStatus * msg = static_cast<const Protocol::MsgSetWorkerStatus *>( imsg );
            Process( connection, msg );
            break;
        }
        default:
        {
            ASSERT( false ); // Protocol bug.
            Disconnect( connection );
            break;
        }
    }

    FREE( const_cast<Protocol::IMessage *>( m_CurrentMessage ) );
    FREE( payload );
    m_CurrentMessage = nullptr;
}

// OnConnected
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnConnected( const ConnectionInfo * connection )
{
    AStackString remoteAddr;
    TCPConnectionPool::GetAddressAsString( connection->GetRemoteAddress(), remoteAddr );
    OUTPUT( "Connected: %s\n", remoteAddr.Get() );
}

// OnDisconnected
//------------------------------------------------------------------------------
void WorkerConnectionPool::OnDisconnected( const ConnectionInfo * )
{
}

// Process
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgRequestWorkerList * msg )
{
    MutexHolder mh( m_Mutex );

    uint32_t numWorkers = 0;
    for ( const WorkerInfo & worker : m_Workers )
    {
        if ( ( worker.m_ProtocolVersionMajor == msg->GetProtocolVersionMajor() ) &&
             ( worker.m_ProtocolVersionMinor == msg->GetProtocolVersionMinor() ) &&
             ( worker.m_Platform == msg->GetPlatform() ) )
        {
            ++numWorkers;
        }
    }

    MemoryStream ms;
    ms.Write( numWorkers );
    for ( const WorkerInfo & worker : m_Workers )
    {
        if ( ( worker.m_ProtocolVersionMajor == msg->GetProtocolVersionMajor() ) &&
             ( worker.m_ProtocolVersionMinor == msg->GetProtocolVersionMinor() ) &&
             ( worker.m_Platform == msg->GetPlatform() ) )
        {
            ms.Write( worker.m_Address );
        }
    }

    Protocol::MsgWorkerList resultMsg;
    resultMsg.Send( connection, ms );
}

// Process
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgWorkerList * /*msg*/, const void * payload, size_t payloadSize )
{
    ConstMemoryStream ms( payload, payloadSize );

    uint32_t numWorkers = 0;
    ms.Read( numWorkers );

    Array<uint32_t> workers;
    workers.SetCapacity( numWorkers );

    for ( uint32_t i = 0; i < numWorkers; ++i )
    {
        uint32_t workerAddress = 0;
        ms.Read( workerAddress );
        workers.Append( workerAddress );
    }

    WorkerBrokerage * brokerage = static_cast<WorkerBrokerage *>( connection->GetUserData() );
    ASSERT( brokerage );
    brokerage->UpdateWorkerList( workers );
}

// Process
//------------------------------------------------------------------------------
void WorkerConnectionPool::Process( const ConnectionInfo * connection, const Protocol::MsgSetWorkerStatus * msg )
{
    MutexHolder mh( m_Mutex );

    const uint32_t workerAddress = connection->GetRemoteAddress();
    if ( msg->IsAvailable() )
    {
        WorkerInfo * worker = m_Workers.Find( workerAddress );
        if ( worker )
        {
            worker->m_ProtocolVersionMajor = msg->GetProtocolVersionMajor();
            worker->m_ProtocolVersionMinor = msg->GetProtocolVersionMinor();
            worker->m_Platform = msg->GetPlatform();
        }
        else
        {
            AStackString remoteAddr;
            TCPConnectionPool::GetAddressAsString( workerAddress, remoteAddr );
            OUTPUT( "Worker available: %s\n", remoteAddr.Get() );
            m_Workers.Append( WorkerInfo( workerAddress, msg->GetProtocolVersionMajor(), msg->GetProtocolVersionMinor(), msg->GetPlatform() ) );
        }
    }
    else
    {
        m_Workers.FindAndErase( workerAddress );
    }
}

//------------------------------------------------------------------------------
