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

static volatile bool terminate = false;

void sig_handler(int signum)
{
    trace_log_printf(LOG_INFO, "terminating...");
    terminate = true;
}

void print_cfg_error(cfg_t *cfg, const char *fmt, va_list ap)
{
    (void)cfg;
    trace_vlog(LOG_WARNING, "libconfuse", fmt, ap);
}

void change_sate(cfg_t * cfg, bool is_online)
{
    if(is_online)
    {
        trace_log_printf(LOG_DEBUG, "transition to on-line");
        system(cfg_getstr(cfg, "online_cmd"));
    }
    else
    {
        trace_log_printf(LOG_DEBUG, "transition to off-line");
        system(cfg_getstr(cfg, "offline_cmd"));
    }
}

int main()
{
    trace_log_setlevel(LOG_INFO);
    trace_log_printf(LOG_INFO, "application started");

    cfg_opt_t opts[] = {
        CFG_STR_LIST("hosts", "{}", CFGF_NONE),
        CFG_STR("device", "", CFGF_NONE),
        CFG_STR("online_cmd", "", CFGF_NONE),
        CFG_STR("offline_cmd", "", CFGF_NONE),
        CFG_INT("up_cnt", 10, CFGF_NONE),
        CFG_INT("down_cnt", 10, CFGF_NONE),
        CFG_INT("start_cnt", 10, CFGF_NONE),
        CFG_INT("interval", 2000, CFGF_NONE),
        CFG_INT("ptimeout", 1000, CFGF_NONE),
        CFG_END()
    };

    cfg_t * cfg = cfg_init(opts, 0);
    cfg_set_error_function(cfg, print_cfg_error);
    (void)cfg_parse(cfg, config_filename);
    for(unsigned int i = 0; i < cfg_size(cfg, "hosts"); i++)
    {
        trace_log_printf(LOG_DEBUG, "host[%d]: %s", i, cfg_getnstr(cfg, "hosts", i));
    }
    trace_log_printf(LOG_DEBUG, "device: %s", cfg_getstr(cfg, "device"));
    trace_log_printf(LOG_DEBUG, "online_cmd: %s", cfg_getstr(cfg, "online_cmd"));
    trace_log_printf(LOG_DEBUG, "offline_cmd: %s", cfg_getstr(cfg, "offline_cmd"));
    trace_log_printf(LOG_DEBUG, "up_cnt: %u", cfg_getint(cfg, "up_cnt"));
    trace_log_printf(LOG_DEBUG, "down_cnt: %u", cfg_getint(cfg, "down_cnt"));
    trace_log_printf(LOG_DEBUG, "start_cnt: %u", cfg_getint(cfg, "start_cnt"));
    trace_log_printf(LOG_DEBUG, "interval: %u", cfg_getint(cfg, "interval"));
    trace_log_printf(LOG_DEBUG, "ptimeout: %u", cfg_getint(cfg, "ptimeout"));

    if( cfg_size(cfg, "hosts")                                                     &&
        strcmp(cfg_getstr(cfg, "device"), "")                                      &&
        strcmp(cfg_getstr(cfg, "online_cmd"), "")                                  &&
        strcmp(cfg_getstr(cfg, "offline_cmd"), "")                                 &&
        cfg_getint(cfg, "up_cnt")    > 0 &&  cfg_getint(cfg, "up_cnt")    < 100    &&
        cfg_getint(cfg, "down_cnt")  > 0 &&  cfg_getint(cfg, "down_cnt")  < 100    &&
        cfg_getint(cfg, "start_cnt") > 0 &&  cfg_getint(cfg, "start_cnt") < 100    &&
        cfg_getint(cfg, "interval")  > 0 &&  cfg_getint(cfg, "interval")  < 100000 &&
        cfg_getint(cfg, "ptimeout")  > 0 &&  cfg_getint(cfg, "ptimeout")  < 100000 )
    {
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = sig_handler;
        sigemptyset(&act.sa_mask);
        (void)sigaction(SIGINT, &act, NULL);

        pingobj_t * ping = ping_construct();
        if(ping)
        {
            trace_log_printf(LOG_INFO, "ping object created");
            double timeout = (double)cfg_getint(cfg, "ptimeout") / 1000;
            if(!ping_setopt(ping, PING_OPT_TIMEOUT, &timeout))
            {
                if(!ping_setopt(ping, PING_OPT_DEVICE, cfg_getstr(cfg, "device")))
                {
                    for(unsigned int i = 0; i < cfg_size(cfg, "hosts"); i++)
                    {
                        if(ping_host_add(ping, cfg_getnstr(cfg, "hosts", i)))
                        {
                            trace_log_printf(LOG_ERROR, "can't add host %s : %s", cfg_getnstr(cfg, "hosts", i), ping_get_error(ping));
                        }
                    }

                    unsigned int success = 0, unsuccess = 0;
                    for(unsigned int  i = 0; !terminate && i < cfg_getint(cfg, "start_cnt"); i++)
                    {
                        if(ping_send(ping) > 0)
                        {
                            success++;
                        }
                        else
                        {
                            unsuccess++;
                        }
                        usleep(cfg_getint(cfg, "interval") * 1000u);
                    }

                    bool on_line = (success > unsuccess);
                    if(!terminate)
                    {
                        change_sate(cfg, on_line);
                    }
                    unsigned int conter = 0;
                    while(!terminate)
                    {
                        int result = ping_send(ping);
                        //trace_log_printf(LOG_INFO, "ping response count %d", result);
                        if(on_line)
                        {
                            if(result > 0)
                            {
                                conter = 0;
                            }
                            else
                            {
                                conter++;
                            }
                            if(conter >= (unsigned)cfg_getint(cfg, "down_cnt"))
                            {
                                on_line = false;
                                conter  = 0;
                                change_sate(cfg, on_line);
                            }
                        }
                        else
                        {
                            if(result > 0)
                            {
                                conter++;
                            }
                            else
                            {
                                conter = 0;
                            }
                            if(conter >= (unsigned)cfg_getint(cfg, "up_cnt"))
                            {
                                on_line = true;
                                conter  = 0;
                                change_sate(cfg, on_line);
                            }
                        }
                        usleep(cfg_getint(cfg, "interval") * 1000u);
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

    trace_log_printf(LOG_INFO, "application stopped");

    return 0;
}