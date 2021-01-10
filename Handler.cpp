
#include <StdFuncs.h>
#include "ClientCommands.h"
#include "StdSocket.h"

/**
 * Reads the command's payload.
 * A convenience method that will allocate memory for the command's payload, and read the payload into that
 * memory.  If the command has no payload, no action will be performed but success will still be returned.
 *
 * @date	Sunday 10-Jan-2021 6:39 am, Code HQ Bergmannstrasse
 * @return	true if the payload was read successfully, else false
 */

bool CHandler::readPayload()
{
	bool retVal = true;

	// If the command has a payload, allocate an appropriately sized buffer and read it
	if (m_command.m_length > 0)
	{
		m_payload = new unsigned char[m_command.m_length];
		retVal = (m_socket->read(m_payload, m_command.m_length) == static_cast<int>(m_command.m_length));
	}

	return retVal;
}

/**
 * Sends a command, without payload.
 * A convenience method to send a command, ensuring that its members are in network format before sending.
 *
 * @date	Sunday 29-Nov-2020 12:17 pm, Code HQ Bergmannstrasse
 * @return	true if the command was sent successfully, else false
 */

bool CHandler::sendCommand()
{
	SWAP(&m_command.m_command);
	SWAP(&m_command.m_length);

	return (m_socket->write(&m_command, sizeof(m_command)) == sizeof(m_command));
}
