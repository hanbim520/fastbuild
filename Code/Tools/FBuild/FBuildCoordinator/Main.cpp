// Main
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildCoordinatorOptions.h"
#include "Coordinator/Coordinator.h"

// Core
#include "Core/Env/Assert.h"
#include "Core/Process/SystemMutex.h"
#include "Core/Process/Thread.h"
#include "Core/Profile/Profile.h"
#include "Core/Strings/AStackString.h"
#include "Core/Time/Timer.h"

// system
#include <stdio.h>

// Global Data
//------------------------------------------------------------------------------
static SystemMutex g_OneProcessMutex( "Global\\FBuildCoordinator" );

// Return Codes
//------------------------------------------------------------------------------
enum ReturnCodes
{
    FBUILD_OK = 0,
    FBUILD_BAD_ARGS = -1,
    FBUILD_ALREADY_RUNNING = -2
};

// Functions
//------------------------------------------------------------------------------
int Main( const AString & args );

// main
//------------------------------------------------------------------------------
int main( int argc, char * argv[] )
{
    AStackString args;
    for ( int i = 1; i < argc; ++i ) // NOTE: Skip argv[0] exe name
    {
        if ( i > 0 )
        {
            args += ' ';
        }
        args += argv[ i ];
    }

    const int result = Main( args );
    PROFILE_SYNCHRONIZE;
    return result;
}

// Main
//------------------------------------------------------------------------------
int Main( const AString & args )
{
    VERIFY( setvbuf( stdout, nullptr, _IONBF, 0 ) == 0 );
    VERIFY( setvbuf( stderr, nullptr, _IONBF, 0 ) == 0 );

    FBuildCoordinatorOptions options;
    if ( options.ProcessCommandLine( args ) == false )
    {
        return FBUILD_BAD_ARGS;
    }

    const Timer t;
    while ( g_OneProcessMutex.TryLock() == false )
    {
        if ( t.GetElapsed() > 5.0f )
        {
            printf( "An FBuildCoordinator is already running!\n" );
            return FBUILD_ALREADY_RUNNING;
        }
        Thread::Sleep( 100 );
    }

    Coordinator coordinator( args );
    return coordinator.Start();
}

//------------------------------------------------------------------------------
