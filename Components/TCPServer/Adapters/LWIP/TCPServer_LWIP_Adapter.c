//==============================================================================
//module enable:

#include "TCPServer/Adapters/TCPServer_AdapterConfig.h"
#ifdef TCP_SERVER_LWIP_ADAPTER_ENABLE
//==============================================================================
//includes:

#include "TCPServer_LWIP_Adapter.h"
#include "TCPServer_LWIP_TxAdapter.h"
#include "TCPServer_LWIP_RxAdapter.h"
#include "Common/xMemory.h"
//==============================================================================
//functions:

static void PrivateHandler(TCPServerT* server)
{
	MX_LWIP_Process();
}
//------------------------------------------------------------------------------

static void PrivateIRQListener(TCPServerT* server, void* reg)
{

}
//------------------------------------------------------------------------------

static void PrivateEventListener(TCPServerT* server, TCPServerAdapterEventSelector selector, void* args, ...)
{
	switch ((int)selector)
	{
		default: return;
	}
}
//------------------------------------------------------------------------------

static xResult PrivateRequestListener(TCPServerT* server, TCPServerAdapterRequestSelector selector, void* arg, ...)
{
	switch ((int)selector)
	{
		default: return xResultRequestIsNotFound;
	}
	
	return xResultAccept;
}
//------------------------------------------------------------------------------
/**
  * @brief  This functions closes the tcp connection
  * @param  tcp_pcb: pointer on the tcp connection
  * @param  es: pointer on echo_state structure
  * @retval None
  */
static void private_tcp_server_connection_close(TCPServerT* server)
{
	server->Status.State = TCPServerClosed;

	if (!server)
	{
		return;
	}

	TCPServerLWIPAdapterT* adapter = server->Adapter.Child;

	if (adapter->Data.Socket)
	{
		/* remove all callbacks */
		tcp_arg(adapter->Data.Socket, NULL);
		tcp_sent(adapter->Data.Socket, NULL);
		tcp_recv(adapter->Data.Socket, NULL);
		tcp_err(adapter->Data.Socket, NULL);
		tcp_poll(adapter->Data.Socket, NULL, 0);

		/* close tcp connection */
		tcp_close(adapter->Data.Socket);

		adapter->Data.Socket = NULL;
	}
}
//------------------------------------------------------------------------------
/**
  * @brief  This function is the implementation for tcp_recv LwIP callback
  * @param  arg: pointer on a argument for the tcp_pcb connection
  * @param  tpcb: pointer on the tcp_pcb connection
  * @param  pbuf: pointer on the received pbuf
  * @param  err: error information regarding the reveived pbuf
  * @retval err_t: error code
  */
static err_t private_tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{

	return ERR_OK;
}
//------------------------------------------------------------------------------
/**
  * @brief  This function implements the tcp_err callback function (called
  *         when a fatal tcp_connection error occurs.
  * @param  arg: pointer on argument parameter
  * @param  err: not used
  * @retval None
  */
static void private_tcp_server_error(void *arg, err_t err)
{
	TCPServerT* server = arg;

	server->Status.Error = TCPServerErrorAborted;

	private_tcp_server_connection_close(server);
}
//------------------------------------------------------------------------------
/**
  * @brief  This function implements the tcp_poll LwIP callback function
  * @param  arg: pointer on argument passed to callback
  * @param  tpcb: pointer on the tcp_pcb for the current tcp connection
  * @retval err_t: error code
  */
static err_t tcp_server_poll(void *arg, struct tcp_pcb *tpcb)
{
	TCPServerT* server = arg;
	err_t ret_err;

	if (server != NULL)
	{
		ret_err = ERR_OK;
	}
	else
	{
		/* nothing to be done */
		tcp_abort(tpcb);
		ret_err = ERR_ABRT;
	}

	return ret_err;
}
//------------------------------------------------------------------------------
/**
  * @brief  This function is the implementation of tcp_accept LwIP callback
  * @param  arg: not used
  * @param  newpcb: pointer on tcp_pcb struct for the newly created tcp connection
  * @param  err: not used
  * @retval err_t: error status
  */
static err_t private_tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
	TCPServerT* server = arg;

	err_t ret_err;

	LWIP_UNUSED_ARG(arg);
	LWIP_UNUSED_ARG(err);

	/* set priority for the newly accepted tcp connection newpcb */
	tcp_setprio(newpcb, TCP_PRIO_MIN);

	if (arg != NULL)
	{
		server->Status.State = TCPServerIsOpen;

		//pass newly allocated es structure as argument to newpcb
		tcp_arg(newpcb, arg);

		//initialize lwip tcp_recv callback function for newpcb
		tcp_recv(newpcb, private_tcp_server_recv);

		//initialize lwip tcp_err callback function for newpcb
		tcp_err(newpcb, private_tcp_server_error);

		//initialize lwip tcp_poll callback function for newpcb
		tcp_poll(newpcb, tcp_server_poll, 0);

		ret_err = ERR_OK;
	}
	else
	{
		//close tcp connection
		//tcp_echoserver_connection_close(newpcb, es);

		//return memory error/
		ret_err = ERR_MEM;
	}

  return ret_err;
}
//==============================================================================
//interfaces:

static TCPServerAdapterInterfaceT TCPServerAdapterInterface =
{
	INITIALIZATION_HANDLER(TCPServerAdapter, PrivateHandler),

	INITIALIZATION_IRQ_LISTENER(TCPServerAdapter, PrivateIRQListener),
	INITIALIZATION_EVENT_LISTENER(TCPServerAdapter, PrivateEventListener),
	INITIALIZATION_REQUEST_LISTENER(TCPServerAdapter, PrivateRequestListener),
};
//------------------------------------------------------------------------------
//initialization:

xResult TCPServerLWIPAdapterInit(TCPServerT* server, TCPServerLWIPAdapterT* adapter)
{
	if (!server || !adapter)
	{
		return xResultLinkError;
	}

	server->Adapter.Object.Description = "TCPServerLWIPAdapterT";
	server->Adapter.Object.Parent = server;

	server->Adapter.Interface = &TCPServerAdapterInterface;
	server->Adapter.Child = adapter;

	TCPServerLWIPRxAdapterInit(server, adapter);
	TCPServerLWIPTxAdapterInit(server, adapter);

	adapter->Data.Socket = tcp_new();

	if (adapter->Data.Socket != NULL)
	{
		err_t err;

		//bind echo_pcb to port TCP_SERVER_DEFAULT_PORT (ECHO protocol)
		err = tcp_bind(adapter->Data.Socket, IP_ADDR_ANY, TCP_SERVER_DEFAULT_PORT);

		if (err == ERR_OK)
		{
			//tart tcp listening for echo_pcb
			adapter->Data.Socket = tcp_listen(adapter->Data.Socket);
			tcp_arg(adapter->Data.Socket, server);

			server->Status.State = TCPServerOpening;
			server->Status.Error = TCPServerErrorNo;

			//initialize LwIP tcp_accept callback function
			tcp_accept(adapter->Data.Socket, private_tcp_server_accept);

			return xResultAccept;
		}
		else
		{
			//deallocate the pcb
			memp_free(MEMP_TCP_PCB, adapter->Data.Socket);
		}
	}

  return xResultError;
}
//==============================================================================
#endif //TCP_SERVER_LWIP_ADAPTER_ENABLE