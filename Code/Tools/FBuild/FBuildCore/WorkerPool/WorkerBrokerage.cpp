// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerage.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"

// Core
#include "Core/Env/Env.h"
#include "Core/Mem/Mem.h"
#include "Core/Network/Network.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::WorkerBrokerage()
    : m_ConnectionPool( nullptr )
    , m_Connection( nullptr )
    , m_WorkerListUpdateReady( false )
    , m_BrokerageInitialized( false )
{
}

// InitBrokerage
//------------------------------------------------------------------------------
void WorkerBrokerage::InitBrokerage()
{
    PROFILE_FUNCTION;

    if ( m_BrokerageInitialized )
    {
        return;
    }

    // brokerage path includes version to reduce unnecessary comms attempts
    const uint32_t protocolVersion = Protocol::kVersionMajor;

    if ( m_CoordinatorAddress.IsEmpty() )
    {
        AStackString coordinator;
        if ( Env::GetEnvVariable( "FASTBUILD_COORDINATOR", coordinator ) )
        {
            m_CoordinatorAddress = coordinator;
        }
    }

    // root folder
    AStackString brokeragePath;
    if ( Env::GetEnvVariable( "FASTBUILD_BROKERAGE_PATH", brokeragePath ) )
    {
        // FASTBUILD_BROKERAGE_PATH can contain multiple paths separated by semi-colon. The worker will register itself into the first path only but
        // the additional paths are paths to additional broker roots allowed for finding remote workers (in order of priority)
        const char * start = brokeragePath.Get();
        const char * end = brokeragePath.GetEnd();
        AStackString pathSeparator( ";" );
        while ( true )
        {
            AStackString root;
            AStackString brokerageRoot;

            const char * separator = brokeragePath.Find( pathSeparator, start, end );
            if ( separator != nullptr )
            {
                root.Append( start, (size_t)( separator - start ) );
            }
            else
            {
                root.Append( start, (size_t)( end - start ) );
            }
            root.TrimStart( ' ' );
            root.TrimEnd( ' ' );
            // <path>/<group>/<version>/
#if defined( __WINDOWS__ )
            brokerageRoot.Format( "%s\\main\\%u.windows\\", root.Get(), protocolVersion );
#elif defined( __OSX__ )
            brokerageRoot.Format( "%s/main/%u.osx/", root.Get(), protocolVersion );
#else
            brokerageRoot.Format( "%s/main/%u.linux/", root.Get(), protocolVersion );
#endif

            m_BrokerageRoots.Append( brokerageRoot );
            if ( !m_BrokerageRootPaths.IsEmpty() )
            {
                m_BrokerageRootPaths.Append( pathSeparator );
            }

            m_BrokerageRootPaths.Append( brokerageRoot );

            if ( separator != nullptr )
            {
                start = separator + 1;
            }
            else
            {
                break;
            }
        }
    }

    m_BrokerageInitialized = true;
}

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerage::~WorkerBrokerage()
{
    DisconnectFromCoordinator();
}

// UpdateWorkerList
//------------------------------------------------------------------------------
void WorkerBrokerage::UpdateWorkerList( Array<uint32_t> & workerListUpdate )
{
    m_WorkerListUpdate.Swap( workerListUpdate );
    m_WorkerListUpdateReady = true;
}

// ConnectToCoordinator
//------------------------------------------------------------------------------
bool WorkerBrokerage::ConnectToCoordinator()
{
    if ( m_CoordinatorAddress.IsEmpty() )
    {
        return false;
    }

    ASSERT( m_ConnectionPool == nullptr );
    ASSERT( m_Connection == nullptr );

    m_ConnectionPool = FNEW( WorkerConnectionPool );
    m_Connection = m_ConnectionPool->Connect( m_CoordinatorAddress, Protocol::kCoordinatorPort, 2000, this ); // 2000ms connection timeout
    if ( m_Connection == nullptr )
    {
        FLOG_WARN( "Failed to connect to FASTBuild coordinator at '%s'", m_CoordinatorAddress.Get() );
        FDELETE m_ConnectionPool;
        m_ConnectionPool = nullptr;
        return false;
    }

    return true;
}

// DisconnectFromCoordinator
//------------------------------------------------------------------------------
void WorkerBrokerage::DisconnectFromCoordinator()
{
    if ( m_ConnectionPool )
    {
        FDELETE m_ConnectionPool;
        m_ConnectionPool = nullptr;
        m_Connection = nullptr;
    }
}

//------------------------------------------------------------------------------
