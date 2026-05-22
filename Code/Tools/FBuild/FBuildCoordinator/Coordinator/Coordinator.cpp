// Coordinator
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "Coordinator.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"
#include "Tools/FBuild/FBuildCore/WorkerPool/WorkerConnectionPool.h"

// Core
#include "Core/Mem/Mem.h"
#include "Core/Profile/Profile.h"
#include "Core/Tracing/Tracing.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
Coordinator::Coordinator( const AString & args )
    : m_BaseArgs( args )
    , m_ConnectionPool( nullptr )
{
    m_ConnectionPool = FNEW( WorkerConnectionPool );
}

// DESTRUCTOR
//------------------------------------------------------------------------------
Coordinator::~Coordinator()
{
    FDELETE m_ConnectionPool;
}

// Start
//------------------------------------------------------------------------------
int32_t Coordinator::Start()
{
    m_WorkThread.Start( &WorkThreadWrapper,
                        "CoordinatorThread",
                        this,
                        ( 256 * KILOBYTE ) );

    return static_cast<int32_t>( m_WorkThread.Join() );
}

// WorkThreadWrapper
//------------------------------------------------------------------------------
/*static*/ uint32_t Coordinator::WorkThreadWrapper( void * userData )
{
    Coordinator * coordinator = reinterpret_cast<Coordinator *>( userData );
    return coordinator->WorkThread();
}

// WorkThread
//------------------------------------------------------------------------------
uint32_t Coordinator::WorkThread()
{
    OUTPUT( "FBuildCoordinator - %s\n", GetVersionString() );
    OUTPUT( "Listening on port %u\n", Protocol::kCoordinatorPort );

    if ( m_ConnectionPool->Listen( Protocol::kCoordinatorPort ) == false )
    {
        OUTPUT( "Failed to listen on port %u. Check that the port is not in use.\n", Protocol::kCoordinatorPort );
        return static_cast<uint32_t>( -3 );
    }

    for ( ;; )
    {
        PROFILE_SYNCHRONIZE;
        Thread::Sleep( 500 );
    }
}

//------------------------------------------------------------------------------
