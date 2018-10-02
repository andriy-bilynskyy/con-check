/*******************************************
1.
See documentation: 
https://noping.cc/
Debian install:
sudo apt-get install liboping-dev
2.
See documentation: 
http://www.nongnu.org/confuse/tutorial-html/index.html
Debian install:
sudo apt-get install libconfuse-dev
********************************************/

#include "trace_log.h"
#include <signal.h>
#include <string.h>
#include <oping.h>
#include <unistd.h>
#include <confuse.h>

static const char config_filename[] = "/etc/con-check/con-check.conf";

static char * host                  = NULL;      // host to ping
static char * device                = NULL;      // interface to send pings
static char * online_cmd            = NULL;      // on on-line script
static char * offline_cmd           = NULL;      // on off-line script
static unsigned long int up_cnt     = 10;        // consistent success pings to set connection up
static unsigned long int down_cnt   = 10;        // consistent unsuccess pings to set connection down
static unsigned long int start_cnt  = 10;        // check initial state pings count
static unsigned long int interval   = 2000;      // interval between pings in ms
static unsigned long int ptimeout   = 1000;      // ping timeout in ms

static volatile bool terminate = false;

void sig_handler(int signum)
{
    trace_log_printf(LOG_INFO, "terminating...");
    terminate = true;
}

void change_sate(bool is_online)
{
    if(is_online)
    {
        trace_log_printf(LOG_DEBUG, "transition to on-line");
        system(online_cmd);
    }
    else
    {
        trace_log_printf(LOG_DEBUG, "transition to off-line");
        system(offline_cmd);
    }
}

int main()
{
    trace_log_setlevel(LOG_INFO);
    trace_log_printf(LOG_INFO, "application started");

    cfg_opt_t opts[] = {
        CFG_SIMPLE_STR("host", &host),
        CFG_SIMPLE_STR("device", &device),
        CFG_SIMPLE_STR("online_cmd", &online_cmd),
        CFG_SIMPLE_STR("offline_cmd", &offline_cmd),
        CFG_SIMPLE_INT("up_cnt", &up_cnt),
        CFG_SIMPLE_INT("down_cnt", &down_cnt),
        CFG_SIMPLE_INT("start_cnt", &start_cnt),
        CFG_SIMPLE_INT("interval", &interval),
        CFG_SIMPLE_INT("ptimeout", &ptimeout),
        CFG_END()
    };

    cfg_t * cfg = cfg_init(opts, 0);
    (void)cfg_parse(cfg, config_filename);

    if(host && device && online_cmd && offline_cmd && up_cnt && down_cnt && start_cnt && interval && ptimeout)
    {
        trace_log_printf(LOG_DEBUG, "host: %s", host);
        trace_log_printf(LOG_DEBUG, "device: %s", device);
        trace_log_printf(LOG_DEBUG, "online_cmd: %s", online_cmd);
        trace_log_printf(LOG_DEBUG, "offline_cmd: %s", offline_cmd);
        trace_log_printf(LOG_DEBUG, "up_cnt: %u", up_cnt);
        trace_log_printf(LOG_DEBUG, "down_cnt: %u", down_cnt);
        trace_log_printf(LOG_DEBUG, "start_cnt: %u", start_cnt);
        trace_log_printf(LOG_DEBUG, "interval: %u", interval);
        trace_log_printf(LOG_DEBUG, "ptimeout: %u", ptimeout);

        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = sig_handler;
        sigemptyset(&act.sa_mask);
        (void)sigaction(SIGINT, &act, NULL);

        pingobj_t * ping = ping_construct();
        if(ping)
        {
            trace_log_printf(LOG_INFO, "ping object created");
            double timeout = (double)ptimeout / 1000;
            if(!ping_setopt(ping, PING_OPT_TIMEOUT, &timeout))
            {
                if(!ping_setopt(ping, PING_OPT_DEVICE, (void*)device))
                {
                    if(!ping_host_add(ping, host))
                    {
                        trace_log_printf(LOG_INFO, "ping host added");
                        unsigned long int success = 0, unsuccess = 0;
                        for(unsigned long  i = 0; !terminate && i < start_cnt; i++)
                        {
                            if(ping_send(ping) > 0)
                            {
                                success++;
                            }
                            else
                            {
                                unsuccess++;
                            }
                        }

                        bool on_line = false;
                        if(success > unsuccess)
                        {
                            on_line = true;
                        }
                        change_sate(on_line);
                        unsigned long int conter = 0;
                        while(!terminate)
                        {
                            int result = ping_send(ping);
                            if(on_line)
                            {
                                if(result >= 0)
                                {
                                    conter = 0;
                                }
                                else
                                {
                                    conter++;
                                }
                                if(conter >= down_cnt)
                                {
                                    on_line = false;
                                    conter  = 0;
                                    change_sate(on_line);
                                }
                            }
                            else
                            {
                                if(result >= 0)
                                {
                                    conter++;
                                }
                                else
                                {
                                    conter = 0;
                                }
                                if(conter >= up_cnt)
                                {
                                    on_line = true;
                                    conter  = 0;
                                    change_sate(on_line);
                                }
                            }
                            usleep(interval * 1000u);
                        }
                    }
                    else
                    {
                        trace_log_printf(LOG_ERROR, "can't add host %s : %s", host, ping_get_error(ping));
                        change_sate(false);
                    }
                }
                else
                {
                    trace_log_printf(LOG_ERROR, "can't set PING_OPT_DEVICE");
                }
            }
            else
            {
                trace_log_printf(LOG_ERROR, "can't set PING_OPT_TIMEOUT");
            }
            ping_destroy(ping);
        }
        else
        {
            trace_log_printf(LOG_ERROR, "can't create ping object");
        }
    }
    else
    {
        trace_log_printf(LOG_ERROR, "invalid configuration");
    }

    cfg_free(cfg);

    return 0;
}