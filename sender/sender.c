#include <linux/kernel.h>   
#include <linux/module.h>   
#include <linux/string.h>   
#include <linux/socket.h>   
#include <linux/net.h>   
#include <net/sock.h>   
#include <asm/processor.h>   
#include <asm/uaccess.h>   
   
#include <net/ksocket.h>   
   
static int port = 4444;   
module_param(port, int, 0444);   
   
int tcp_srv(void *arg)   
{   
    ksocket_t sockfd_srv, sockfd_cli;   
    struct sockaddr_in addr_srv;   
    struct sockaddr_in addr_cli;   
    char buf[1024];   
    int addr_len, len;   
       
    sprintf(current->comm, "ksocket"); /* kernel thread name*/   
//  lock_kernel();   /* This seems to be required for exit_mm */   
//  exit_mm(current);   
//  /* close open files too (stdin/out/err are open) */   
//  exit_files(current);       
       
    sockfd_srv = sockfd_cli = NULL;   
    memset(&addr_cli, 0, sizeof(addr_cli));   
    memset(&addr_srv, 0, sizeof(addr_srv));   
    addr_srv.sin_family = AF_INET;   
    addr_srv.sin_port = htons(port);   
    addr_srv.sin_addr.s_addr = INADDR_ANY;   
    addr_len = sizeof(struct sockaddr_in);   
       
    sockfd_srv = ksocket(AF_INET, SOCK_STREAM, 0);   
    ksocket_debug("sockfd_srv = 0x%p\n", sockfd_srv);   
    if (sockfd_srv == NULL)   
    {   
        printk("socket failed\n");   
        return -1;   
    }   
    if (kbind(sockfd_srv, (struct sockaddr *)&addr_srv, addr_len) < 0)   
    {   
        printk("bind failed\n");   
        return -1;   
    }   
   
    if (klisten(sockfd_srv, 10) < 0)   
    {   
        printk("listen failed\n");   
        return -1;   
    }   
   
    sockfd_cli = kaccept(sockfd_srv, (struct sockaddr *)&addr_cli, &addr_len);   
    if (sockfd_cli == NULL)   
    {   
        printk("accept failed\n");   
        return -1;   
    }   
    else   
        printk("sockfd_cli = 0x%p\n", sockfd_cli);   
    printk("got connected from : %s %d\n", inet_ntoa(addr_cli.sin_addr), ntohs(addr_cli.sin_port));   
   
    len = sprintf(buf, "%s", "Hello, this is from a kernel thread");   
    ksend(sockfd_cli, buf, len, 0);   
       
    while (1)   
    {   
        memset(buf, 0, sizeof(buf));   
        len = krecv(sockfd_cli, buf, sizeof(buf), 0);   
        if (len > 0)   
        {   
            printk("got message : %s\n", buf);   
            ksend(sockfd_cli, buf, len, 0);   
            if (memcmp(buf, "quit", 4) == 0)   
                break;   
        }   
    }   
   
    kclose(sockfd_cli);   
    kclose(sockfd_srv);   
       
    return 0;   
}   
   
static int my_module_init(void)   
{   
    kernel_thread(tcp_srv, NULL, 0);   
       
    printk("ksocket tcp srv init ok\n");   
    return 0;   
}   
   
static void my_module_exit(void)   
{   
    printk("ksocket tcp srv exit\n");   
}   
   
module_init(my_module_init);   
module_exit(my_module_exit);   
   
MODULE_LICENSE("GPL");