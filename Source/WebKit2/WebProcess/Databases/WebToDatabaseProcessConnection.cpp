/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "WebToDatabaseProcessConnection.h"

#include "DatabaseToWebProcessConnectionMessages.h"
#include "WebIDBConnectionToServerMessages.h"
#include "WebIDBServerConnection.h"
#include "WebIDBServerConnectionMessages.h"
#include "WebProcess.h"
#include <wtf/RunLoop.h>

#if ENABLE(DATABASE_PROCESS)

using namespace WebCore;

namespace WebKit {

WebToDatabaseProcessConnection::WebToDatabaseProcessConnection(IPC::Connection::Identifier connectionIdentifier)
{
    m_connection = IPC::Connection::createClientConnection(connectionIdentifier, *this);
    m_connection->open();
}

WebToDatabaseProcessConnection::~WebToDatabaseProcessConnection()
{
}

void WebToDatabaseProcessConnection::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
#if ENABLE(INDEXED_DATABASE)
    if (decoder.messageReceiverName() == Messages::WebIDBConnectionToServer::messageReceiverName()) {
        auto iterator = m_webIDBConnections.find(decoder.destinationID());
        if (iterator != m_webIDBConnections.end())
            iterator->value->didReceiveMessage(connection, decoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::WebIDBServerConnection::messageReceiverName()) {
        HashMap<uint64_t, WebIDBServerConnection*>::iterator connectionIterator = m_webIDBServerConnections.find(decoder.destinationID());
        if (connectionIterator != m_webIDBServerConnections.end())
            connectionIterator->value->didReceiveWebIDBServerConnectionMessage(connection, decoder);
        return;
    }
#endif
    
    ASSERT_NOT_REACHED();
}

void WebToDatabaseProcessConnection::didClose(IPC::Connection& connection)
{
    WebProcess::singleton().webToDatabaseProcessConnectionClosed(this);
}

void WebToDatabaseProcessConnection::didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference messageReceiverName, IPC::StringReference messageName)
{
}

#if ENABLE(INDEXED_DATABASE)
void WebToDatabaseProcessConnection::registerWebIDBServerConnection(WebIDBServerConnection& connection)
{
    ASSERT(!m_webIDBServerConnections.contains(connection.messageSenderDestinationID()));
    m_webIDBServerConnections.set(connection.messageSenderDestinationID(), &connection);

}

void WebToDatabaseProcessConnection::removeWebIDBServerConnection(WebIDBServerConnection& connection)
{
    ASSERT(m_webIDBServerConnections.contains(connection.messageSenderDestinationID()));

    send(Messages::DatabaseToWebProcessConnection::RemoveDatabaseProcessIDBConnection(connection.messageSenderDestinationID()));

    m_webIDBServerConnections.remove(connection.messageSenderDestinationID());
}

WebIDBConnectionToServer& WebToDatabaseProcessConnection::idbConnectionToServerForSession(const SessionID& sessionID)
{
    auto result = m_webIDBConnections.add(sessionID.sessionID(), nullptr);
    if (result.isNewEntry)
        result.iterator->value = WebIDBConnectionToServer::create();

    return *result.iterator->value;
}
#endif

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
