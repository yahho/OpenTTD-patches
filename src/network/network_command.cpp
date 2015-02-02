/* $Id$ */

/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file network_command.cpp Command handling over network connections. */

#ifdef ENABLE_NETWORK

#include "../stdafx.h"
#include "../string.h"
#include "network_command.h"
#include "network_admin.h"
#include "network_client.h"
#include "network_server.h"
#include "../command_func.h"
#include "../company_func.h"
#include "../settings_type.h"

/**
 * Append a CommandPacket at the end of the queue.
 * @param p The packet to append to the queue.
 * @note A new instance of the CommandPacket will be made.
 */
void CommandQueue::Append(CommandPacket *p)
{
	CommandPacket *add = xmalloct<CommandPacket>();
	*add = *p;
	add->text = add->textdata;
	add->next = NULL;
	if (this->first == NULL) {
		this->first = add;
	} else {
		this->last->next = add;
	}
	this->last = add;
	this->count++;
}

/**
 * Return the first item in the queue and remove it from the queue.
 * @param ignore_paused Whether to ignore commands that may not be executed while paused.
 * @return the first item in the queue.
 */
CommandPacket *CommandQueue::Pop(bool ignore_paused)
{
	CommandPacket **prev = &this->first;
	CommandPacket *ret = this->first;
	CommandPacket *prev_item = NULL;
	if (ignore_paused && _pause_mode != PM_UNPAUSED) {
		while (ret != NULL && !IsCommandAllowedWhilePaused(ret->cmd)) {
			prev_item = ret;
			prev = &ret->next;
			ret = ret->next;
		}
	}
	if (ret != NULL) {
		if (ret == this->last) this->last = prev_item;
		*prev = ret->next;
		this->count--;
	}
	return ret;
}

/**
 * Return the first item in the queue, but don't remove it.
 * @param ignore_paused Whether to ignore commands that may not be executed while paused.
 * @return the first item in the queue.
 */
CommandPacket *CommandQueue::Peek(bool ignore_paused)
{
	if (!ignore_paused || _pause_mode == PM_UNPAUSED) return this->first;

	for (CommandPacket *p = this->first; p != NULL; p = p->next) {
		if (IsCommandAllowedWhilePaused(p->cmd)) return p;
	}
	return NULL;
}

/** Free everything that is in the queue. */
void CommandQueue::Free()
{
	CommandPacket *cp;
	while ((cp = this->Pop()) != NULL) {
		free(cp);
	}
	assert(this->count == 0);
}

/** Local queue of packets waiting for handling. */
static CommandQueue _local_wait_queue;
/** Local queue of packets waiting for execution. */
static CommandQueue _local_execution_queue;

/**
 * Prepare a DoCommand to be send over the network
 * @param cc The command to send
 * @param company The company that wants to send the command
 * @param cmdsrc Source of the command
 */
void NetworkSendCommand(const Command *cc, CompanyID company, CommandSource cmdsrc)
{
	assert(cmdsrc_is_local(cmdsrc));

	/* Clients send their command to the server and forget all about the packet */
	if (!_network_server) {
		MyClient::SendCommand (company, cc);
		return;
	}

	CommandPacket c;
	c.company  = company;
	c.tile     = cc->tile;
	c.p1       = cc->p1;
	c.p2       = cc->p2;
	c.cmd      = cc->cmd;
	c.text     = c.textdata;

	if (cc->text != NULL) {
		bstrcpy (c.textdata, cc->text);
	} else {
		c.textdata[0] = '\0';
	}

	/* If we are the server, we queue the command in our 'special' queue.
	 *   In theory, we could execute the command right away, but then the
	 *   client on the server can do everything 1 tick faster than others.
	 *   So to keep the game fair, we delay the command with 1 tick
	 *   which gives about the same speed as most clients.
	 */
	c.frame = _frame_counter_max + 1;
	c.cmdsrc = cmdsrc;

	_local_wait_queue.Append(&c);
}

/**
 * Sync our local command queue to the command queue of the given
 * socket. This is needed for the case where we receive a command
 * before saving the game for a joining client, but without the
 * execution of those commands. Not syncing those commands means
 * that the client will never get them and as such will be in a
 * desynced state from the time it started with joining.
 * @param cs The client to sync the queue to.
 */
void NetworkSyncCommandQueue(NetworkClientSocket *cs)
{
	for (CommandPacket *p = _local_execution_queue.Peek(); p != NULL; p = p->next) {
		CommandPacket c = *p;
		c.cmdsrc = CMDSRC_NETWORK_OTHER;
		cs->outgoing_queue.Append(&c);
	}
}

/**
 * Execute all commands on the local command queue that ought to be executed this frame.
 */
void NetworkExecuteLocalCommandQueue()
{
	assert(IsLocalCompany());

	CommandQueue &queue = (_network_server ? _local_execution_queue : ClientNetworkGameSocketHandler::my_client->incoming_queue);

	CommandPacket *cp;
	while ((cp = queue.Peek()) != NULL) {
		/* The queue is always in order, which means
		 * that the first element will be executed first. */
		if (_frame_counter < cp->frame) break;

		if (_frame_counter > cp->frame) {
			/* If we reach here, it means for whatever reason, we've already executed
			 * past the command we need to execute. */
			error("[net] Trying to execute a packet in the past!");
		}

		/* We can execute this command */
		_current_company = cp->company;
		cp->execp (cp->cmdsrc);

		queue.Pop();
		free(cp);
	}

	/* Local company may have changed, so we should not restore the old value */
	_current_company = _local_company;
}

/**
 * Free the local command queues.
 */
void NetworkFreeLocalCommandQueue()
{
	_local_wait_queue.Free();
	_local_execution_queue.Free();
}

/**
 * "Send" a particular CommandPacket to all clients.
 * @param cp    The command that has to be distributed.
 * @param owner The client that owns the command,
 */
static void DistributeCommandPacket(CommandPacket &cp, const NetworkClientSocket *owner)
{
	/*
	 * Commands in distribution queues are always local.
	 * For client commands, they are implicitly local.
	 * For commands from the server, they must have a valid local source.
	 */
	if (owner == NULL) assert(cmdsrc_is_local(cp.cmdsrc));

	CommandSource cmdsrc = cp.cmdsrc;
	cp.frame = _frame_counter_max + 1;

	NetworkClientSocket *cs;
	FOR_ALL_CLIENT_SOCKETS(cs) {
		if (cs->status >= NetworkClientSocket::STATUS_MAP) {
			/* Callbacks are only send back to the client who sent them in the
			 *  first place. This filters that out. */
			cp.cmdsrc = (cs == owner) ? CMDSRC_NETWORK_SELF : CMDSRC_NETWORK_OTHER;
			cs->outgoing_queue.Append(&cp);
		}
	}

	assert(cs == NULL);

	cp.cmdsrc = (owner == NULL) ? cmdsrc_make_network (cmdsrc) : CMDSRC_NETWORK_OTHER;

	_local_execution_queue.Append(&cp);
}

/**
 * "Send" a particular CommandQueue to all clients.
 * @param queue The queue of commands that has to be distributed.
 * @param owner The client that owns the commands,
 */
static void DistributeQueue(CommandQueue *queue, const NetworkClientSocket *owner)
{
#ifdef DEBUG_DUMP_COMMANDS
	/* When replaying we do not want this limitation. */
	int to_go = UINT16_MAX;
#else
	int to_go = _settings_client.network.commands_per_frame;
#endif

	CommandPacket *cp;
	while (--to_go >= 0 && (cp = queue->Pop(true)) != NULL) {
		DistributeCommandPacket(*cp, owner);
		NetworkAdminCmdLogging(owner, cp);
		free(cp);
	}
}

/** Distribute the commands of ourself and the clients. */
void NetworkDistributeCommands()
{
	/* First send the server's commands. */
	DistributeQueue(&_local_wait_queue, NULL);

	/* Then send the queues of the others. */
	NetworkClientSocket *cs;
	FOR_ALL_CLIENT_SOCKETS(cs) {
		DistributeQueue(&cs->incoming_queue, cs);
	}
}

/**
 * Receives a command from the network.
 * @param p the packet to read from.
 * @param from_server whether the packet comes from the server
 * @param err pointer to store an error message
 * @return whether reception succeeded
 */
bool CommandPacket::ReceiveFrom (Packet *p, bool from_server, const char **err)
{
	this->company = (CompanyID)p->Recv_uint8();
	this->cmd     = (CommandID)p->Recv_uint8();
	if (!IsValidCommand(this->cmd)) {
		*err = "invalid command";
		return false;
	}
	if (GetCommandFlags(this->cmd) & CMDF_OFFLINE) {
		*err = "offline-only command";
		return false;
	}

	this->p1      = p->Recv_uint32();
	this->p2      = p->Recv_uint32();
	this->tile    = p->Recv_uint32();
	assert (this->text == this->textdata);
	p->Recv_string (this->textdata, lengthof(this->textdata), (!_network_server && GetCommandFlags(this->cmd) & CMDF_STR_CTRL) != 0 ? SVS_ALLOW_CONTROL_CODE | SVS_REPLACE_WITH_QUESTION_MARK : SVS_REPLACE_WITH_QUESTION_MARK);

	if (from_server) {
		this->frame  = p->Recv_uint32();
		this->cmdsrc = p->Recv_bool() ? CMDSRC_NETWORK_SELF : CMDSRC_NETWORK_OTHER;
	}

	return true;
}

/**
 * Sends a command over the network.
 * @param company the company that issued the command
 * @param c the command.
 * @param p the packet to send it in.
 * @param from_server whether we are the server sending the packet
 */
void CommandPacket::SendTo (uint8 company, const Command *c, Packet *p)
{
	assert_compile (CMD_END <= UINT8_MAX);

	p->Send_uint8  (company);
	p->Send_uint8  (c->cmd);
	p->Send_uint32 (c->p1);
	p->Send_uint32 (c->p2);
	p->Send_uint32 (c->tile);
	p->Send_string (c->text != NULL ? c->text : "");
}

/**
 * Sends a command over the network.
 * @param p the packet to send it in.
 */
void CommandPacket::SendTo (Packet *p, bool from_server) const
{
	CommandPacket::SendTo (this->company, this, p);

	if (from_server) {
		p->Send_uint32 (this->frame);
		p->Send_bool (this->cmdsrc == CMDSRC_NETWORK_SELF);
	}
}

#endif /* ENABLE_NETWORK */
