// WorkerBrokerage - Manage worker discovery
//------------------------------------------------------------------------------
#pragma once

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Array.h"
#include "Core/Strings/AString.h"

// Forward Declarations
//------------------------------------------------------------------------------
class ConnectionInfo;
class WorkerConnectionPool;

// WorkerBrokerage
//------------------------------------------------------------------------------
class WorkerBrokerage
{
public:
    WorkerBrokerage();
    ~WorkerBrokerage();

    const AString & GetBrokerageRootPaths() const { return m_BrokerageRootPaths; }
    void UpdateWorkerList( Array<uint32_t> & workerListUpdate );

protected:
    void InitBrokerage();
    bool ConnectToCoordinator();
    void DisconnectFromCoordinator();
    bool IsCoordinatorConfigured() const { return m_CoordinatorAddress.IsEmpty() == false; }

    Array<AString> m_BrokerageRoots;
    AString m_BrokerageRootPaths;
    AString m_CoordinatorAddress;
    WorkerConnectionPool * m_ConnectionPool;
    const ConnectionInfo * m_Connection;
    Array<uint32_t> m_WorkerListUpdate;
    bool m_WorkerListUpdateReady;
    bool m_BrokerageInitialized;
};

//------------------------------------------------------------------------------
