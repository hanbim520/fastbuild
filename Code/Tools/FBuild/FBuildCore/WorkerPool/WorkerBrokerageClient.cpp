// WorkerBrokerageClient - Client-side worker discovery
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "WorkerBrokerageClient.h"

// FBuildCore
#include "Tools/FBuild/FBuildCore/FLog.h"
#include "Tools/FBuild/FBuildCore/Protocol/Protocol.h"

// Core
#include "Core/Env/Env.h"
#include "Core/FileIO/FileIO.h"
#include "Core/FileIO/PathUtils.h"
#include "Core/Network/Network.h"
#include "Core/Network/TCPConnectionPool.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Time/Timer.h"

// CONSTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::WorkerBrokerageClient() = default;

// DESTRUCTOR
//------------------------------------------------------------------------------
WorkerBrokerageClient::~WorkerBrokerageClient() = default;

// FindWorkers
//------------------------------------------------------------------------------
void WorkerBrokerageClient::FindWorkers( Array<AString> & outWorkerList )
{
    PROFILE_FUNCTION;

    // Check for workers for the FASTBUILD_WORKERS environment variable
    // which is a list of worker addresses separated by a semi-colon.
    AStackString workersEnv;
    if ( Env::GetEnvVariable( "FASTBUILD_WORKERS", workersEnv ) )
    {
        // If we find a valid list of workers, we'll use that
        workersEnv.Tokenize( outWorkerList, ';' );
        if ( outWorkerList.IsEmpty() == false )
        {
            return;
        }
    }

    // check for workers through brokerage

    // Init the brokerage
    InitBrokerage();
    if ( m_BrokerageRoots.IsEmpty() && ( IsCoordinatorConfigured() == false ) )
    {
        FLOG_WARN( "No brokerage root or coordinator; did you set FASTBUILD_BROKERAGE_PATH or FASTBUILD_COORDINATOR?" );
        return;
    }

    // Try coordinator first, falling back to brokerage folders if configured.
    if ( ConnectToCoordinator() )
    {
        m_WorkerListUpdate.Clear();
        m_WorkerListUpdateReady = false;

        Protocol::MsgRequestWorkerList msg;
        msg.Send( m_Connection );

        Timer timer;
        while ( ( m_WorkerListUpdateReady == false ) &&
                ( timer.GetElapsedMS() < 5000.0f ) )
        {
            Thread::Sleep( 1 );
        }

        DisconnectFromCoordinator();

        if ( m_WorkerListUpdateReady == false )
        {
            FLOG_WARN( "Timed out waiting for worker list from FASTBuild coordinator" );
        }
        else
        {
            if ( ( outWorkerList.GetSize() + m_WorkerListUpdate.GetSize() ) > outWorkerList.GetCapacity() )
            {
                outWorkerList.SetCapacity( outWorkerList.GetSize() + m_WorkerListUpdate.GetSize() );
            }

            StackArray<AString> localAddresses;
            Network::GetIPv4Addresses( localAddresses );

            for ( const uint32_t workerAddress : m_WorkerListUpdate )
            {
                AStackString workerName;
                TCPConnectionPool::GetAddressAsString( workerAddress, workerName );
                if ( localAddresses.Find( workerName ) == nullptr )
                {
                    outWorkerList.Append( workerName );
                }
            }

            m_WorkerListUpdate.Clear();
            if ( outWorkerList.IsEmpty() == false )
            {
                return;
            }
        }
    }

    if ( m_BrokerageRoots.IsEmpty() )
    {
        return;
    }

    Array<AString> results;
    results.SetCapacity( 256 );
    for ( AString & root : m_BrokerageRoots )
    {
        const size_t filesBeforeSearch = results.GetSize();
        if ( !FileIO::GetFiles( root,
                                AStackString( "*" ),
                                false,
                                &results ) )
        {
            FLOG_WARN( "No workers found in '%s'", root.Get() );
        }
        else
        {
            FLOG_WARN( "%zu workers found in '%s'", results.GetSize() - filesBeforeSearch, root.Get() );
        }
    }

    // pre-size
    if ( ( outWorkerList.GetSize() + results.GetSize() ) > outWorkerList.GetCapacity() )
    {
        outWorkerList.SetCapacity( outWorkerList.GetSize() + results.GetSize() );
    }

    // Get addresses for the local host
    StackArray<AString> localAddresses;
    Network::GetIPv4Addresses( localAddresses );

    // convert worker strings
    for ( const AString & fileName : results )
    {
        const char * lastSlash = fileName.FindLast( NATIVE_SLASH );
        AStackString workerName( lastSlash + 1 );

        // Filter out local addresses
        if ( localAddresses.Find( workerName ) )
        {
            continue;
        }

        outWorkerList.Append( workerName );
    }
}

//------------------------------------------------------------------------------
