#include "wb_syslib_addon.h"
#include "hic_host.h"
#include "FTH_Int.h"
#include "RemoteFunc.h"
#include "RemoteNet.h"
#include "clk.h"
#include "pll.h"
#include "tt_thread.h"

#include "RemoteFuncTest.h"
#include "test_sendto.h"

void test_sendto(void)
{
	test_printf_begin("test_sendto");
	test_sendto_entry();
	test_printf_end("test_sendto");
}

static void test_tcp_socket_thread_join(int ithread_handle)
{
	cyg_thread_info	info;
	cyg_uint16		id = 0;
	cyg_bool		rc;
	
	cyg_handle_t	next_thread = 0;
	cyg_uint16		next_id = 0;
	
	while(cyg_thread_get_next(&next_thread, &next_id) != false)
	{
		if(ithread_handle == next_thread)
		{
			id = next_id;
			break;
		}
	}
	
	while(1)
	{
		rc = cyg_thread_get_info(ithread_handle, id, &info);
		if(rc == false)
		{
			printf("Error get thread %d info\n", ithread_handle);
			break;
		}
		else
		{
			if((info.state & EXITED) != 0)
			{
				break;
			}
		}
		cyg_thread_yield();
	}
}

static char msg[TEST_SENDTO_SERVER_NUM][TEST_SENDTO_MSG_LEN];
static TEST_SENDTO_DATA_T netdata[TEST_SENDTO_SERVER_NUM];

void test_sendto_entry(void)
{
	int iPort = TEST_SENDTO_SERVER_BEGIN_PORT;
	int i;
	
	for(i = 0; i < TEST_SENDTO_SERVER_NUM; i++)
	{
		netdata[i].iport = iPort+i;
		netdata[i].pbuf = (char*)NON_CACHE(g_RemoteNet_Buf1[i]);
		netdata[i].psendtobuf = msg[i];
		cyg_thread_create(THREAD_PRIORITY, &test_sendto_server, (cyg_addrword_t)&netdata[i], 
						NULL, thread_stack[i], STACK_SIZE, &thread_handle[i], &thread[i]);
		cyg_thread_resume(thread_handle[i]);
	}
	
	for(i = 0; i < TEST_SENDTO_SERVER_NUM; i++)
	{
		test_tcp_socket_thread_join(thread_handle[i]);
		cyg_thread_delete(thread_handle[i]);
	}
}

void test_sendto_server(cyg_addrword_t pnetdata)
{
	int s, i, len, sendtolen;
	struct sockaddr_in sa;
	
	int threadid;
		
	int port = ((TEST_SENDTO_DATA_T*)pnetdata)->iport;
	char *pbuf = ((TEST_SENDTO_DATA_T*)pnetdata)->pbuf;
	char *psendtobuf = ((TEST_SENDTO_DATA_T*)pnetdata)->psendtobuf;
	
	threadid = port;
	
    if(inet_aton(TEST_SENDTO_SERVER_ADDR, &sa.sin_addr, pbuf, RNT_BUFFER_LEN) == 0)
    {
		test_printf_error("test_sendto_server");
		cyg_thread_exit();
    }
    sa.sin_family = AF_INET;
    sa.sin_port = htons(IPPORT_USERRESERVED + port);
    
	if((s = socket(AF_INET, SOCK_DGRAM, PF_UNSPEC, pbuf, RNT_BUFFER_LEN)) == -1)
	{
		test_printf_error("test_sendto_server");
		cyg_thread_exit();
	}
	
    for(i = 0; i < TEST_SENDTO_WRITE_TIMES; i++)
	{
		len = sprintf(psendtobuf, "%s", TEST_SENDTO_MSG);
		len++;
		sendtolen = sendto(s, psendtobuf, len, 0, 
							(struct sockaddr*)&sa, sizeof(sa), pbuf, RNT_BUFFER_LEN);
		if(sendtolen < 0)
		{
			test_printf_error("test_sendto_server");
			break;
		}
	}
	
	if(i == TEST_SENDTO_WRITE_TIMES)
		test_printf_success("test_sendto_server");
	
	netclose(s, pbuf, RNT_BUFFER_LEN);
	cyg_thread_exit();
}
