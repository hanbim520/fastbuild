// FBuildCoordinatorOptions
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "FBuildCoordinatorOptions.h"

// FBuild
#include "Tools/FBuild/FBuildCore/FBuildVersion.h"

// Core
#include "Core/Containers/Array.h"
#include "Core/Strings/AStackString.h"

// system
#include <stdio.h>

// CONSTRUCTOR
//------------------------------------------------------------------------------
FBuildCoordinatorOptions::FBuildCoordinatorOptions() = default;

// ProcessCommandLine
//------------------------------------------------------------------------------
bool FBuildCoordinatorOptions::ProcessCommandLine( const AString & commandLine )
{
    Array<AString> tokens;
    commandLine.Tokenize( tokens );

    if ( tokens.IsEmpty() )
    {
        return true;
    }

    ShowUsageError();
    return false;
}

// ShowUsageError
//------------------------------------------------------------------------------
void FBuildCoordinatorOptions::ShowUsageError()
{
    AStackString msg;
    msg.Format( "FBuildCoordinator.exe - %s\n", GetVersionString() );
    msg.Append( "Copyright 2012-2026 Franta Fulin - https://www.fastbuild.org\n"
                "\n"
                "Command Line Options:\n"
                "---------------------------------------------------------------------------\n"
                " No options are currently supported.\n"
                "---------------------------------------------------------------------------\n" );
    printf( "%s", msg.Get() );
}

//------------------------------------------------------------------------------
