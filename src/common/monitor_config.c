/*
* Copyright 2019-present Open Networking Foundation
*
* SPDX-License-Identifier: Apache-2.0
*
*
*/

#include <stdio.h>
#include <sys/inotify.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
#include <monitor_config.h>
#include <log.h>
#include "time.h"

#define MAX_FILE_PATH 128
typedef struct config_watch_entry 
{
  configCbk    callback;
  char         config_file[MAX_FILE_PATH]; 
  bool         always;
  bool         handled; /* only 1 event per thread life is provided to application */
}config_watch_t;

pthread_t  config_monitor_tid;

static bool
handle_events(int fd, int *wd, config_watch_t *config)
{
    time_t t = time(NULL);
    bool handled = false;
    /* Some systems cannot read integer variables if they are not
       properly aligned. On other systems, incorrect alignment may
       decrease performance. Hence, the buffer used for reading from
       the inotify file descriptor should have the same alignment as
       struct inotify_event. */

    char buf[4096]
        __attribute__ ((aligned(__alignof__(struct inotify_event))));
    const struct inotify_event *event;
    ssize_t len;
    char *ptr;

    /* Loop while events can be read from inotify file descriptor. */
    for (;;) {

        /* Read some events. */

        len = read(fd, buf, sizeof buf);
        if (len == -1 && errno != EAGAIN) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        /* If the nonblocking read() found no events to read, then
           it returns -1 with errno set to EAGAIN. In that case,
           we exit the loop. */

        if (len <= 0)
            break;

        /* Loop over all events in the buffer */

        for (ptr = buf; ptr < buf + len;
                ptr += sizeof(struct inotify_event) + event->len) {

            event = (const struct inotify_event *) ptr;

            /* Print event type */

            if (event->mask & IN_ACCESS)
            {
                //log_msg(LOG_DEBUG,"IN_ACCESS : ");
                continue;
            }
            if (event->mask & IN_OPEN)
            {
                //log_msg(LOG_DEBUG,"IN_OPEN: skip ");
                continue;
            }
            if (event->mask & IN_CLOSE_NOWRITE)
            {
                //log_msg(LOG_DEBUG,"IN_CLOSE_NOWRITE: ");
                continue;
            }
#if 0
            if (event->mask & IN_ATTRIB)
                log_msg(LOG_DEBUG,"IN_ATTRIB: ");
            if (event->mask & IN_CLOSE_WRITE)
                log_msg(LOG_DEBUG,"IN_CLOSE_WRITE: ");
            if (event->mask & IN_CREATE)
                log_msg(LOG_DEBUG,"IN_CREATE: ");
            if (event->mask & IN_DELETE)
                log_msg(LOG_DEBUG,"IN_DELETE: ");
            if (event->mask & IN_DELETE_SELF)
                log_msg(LOG_DEBUG,"IN_DELETE_SELF: ");
            if (event->mask & IN_MODIFY)
                log_msg(LOG_DEBUG,"IN_MODIFY: ");
            if (event->mask & IN_MOVE_SELF)
                log_msg(LOG_DEBUG,"IN_MOVE_SELF: ");
            if (event->mask & IN_MOVED_FROM)
                log_msg(LOG_DEBUG,"IN_MOVED_FROM: ");
            if (event->mask & IN_MOVED_TO)
                log_msg(LOG_DEBUG,"IN_MOVED_TO: ");
           	if (event->mask & IN_IGNORED)
                log_msg(LOG_DEBUG,"IN_IGNORE: file deleted ");
#endif

            if (wd[0] == event->wd && config->handled == false) {
                handled = true;
                uint32_t flags=0;
				config->handled=true;
                log_msg(LOG_INFO,"Config - %p - Received file event for %s at time %lu ",config, config->config_file, t);
                config->callback(config->config_file, flags); 
                break;
            }
        }
    }
    return handled;
}

  


#ifdef TEST_LOCALLY
void config_change_callback(char *configFile, uint32_t flags)
{
  printf("Received config change callback %s",__FUNCTION__);
  return;
}

int main()
{
  watch_config_change("/etc/mme/my_test_config.cfg", config_change_callback, true);
  while(1);
  return 0;
}
#endif

void * 
config_thread_handler(void *config)
{
  nfds_t nfds;
  int poll_num;
  struct pollfd fds[1];
  int wd;
  config_watch_t *cfg = (config_watch_t *)config;
  int fd = 0 ; 

  log_msg(LOG_INFO, "Thread started for monitoring config file %s - Monitoring config object  %p ",cfg->config_file, cfg);

  fd = inotify_init1(IN_NONBLOCK);
  if (fd == -1) 
  {
      perror("inotify_init1");
      exit(EXIT_FAILURE);
  }

  wd = inotify_add_watch(fd, cfg->config_file, IN_ALL_EVENTS); //OPEN | IN_CLOSE);
  if (wd == -1) 
  {
      log_msg(LOG_INFO, "Can not watch file. File does not exist - %s ",cfg->config_file);
      return NULL;
  }
  log_msg(LOG_INFO, "add_watch return %d ", wd);

  /* Prepare for polling */
  nfds = 1;

  /* Inotify input */
  fds[0].fd = fd;
  fds[0].events = POLLIN;

  /* Wait for events and/or terminal input */

  while (1) 
  {
    // -1 timeout means we wait till event received.
    // That also means no tight looping 
    poll_num = poll(fds, nfds, 5000);
    if (poll_num == -1) 
    {
        if (errno == EINTR)
            continue;
        perror("poll");
        exit(1);
    }
    else if (poll_num > 0) 
    {
      if (fds[0].revents & POLLIN) 
      {
        /* Inotify events are available */
        bool handled = handle_events(fd, &wd, cfg);
        if(handled == true)
        {
          log_msg(LOG_DEBUG, "FILE change detected");
          if(cfg->always == true)
          {
            wd = inotify_add_watch(fd, cfg->config_file, IN_ALL_EVENTS); //OPEN | IN_CLOSE);
            if (wd == -1) 
            {
              fprintf(stderr, "Cannot watch ");
              perror("inotify_add_watch");
              exit(1);
            }
          }
          else 
	      {
	        break;
	      }
        }
      }
    }
  }

  log_msg(LOG_INFO, "Thread closing for monitoring config file %s Config-%p ",cfg->config_file, cfg);
  free(cfg);
  return NULL;
}

/* Create fd and add it into the link list */
void watch_config_change(char *config_file, configCbk cbk, bool always)
{
  pthread_attr_t attr;


  config_watch_t *config_entry = (config_watch_t *) calloc(1, sizeof(config_watch_t));
  strncpy(config_entry->config_file, config_file, strlen(config_file));
  config_entry->callback = cbk;
  config_entry->always = always;
  config_entry->handled = false;

  pthread_attr_init(&attr);
  /* Create a thread which will monitor changes in the config file */
  pthread_create(&config_monitor_tid, &attr, &config_thread_handler, config_entry);
  pthread_attr_destroy(&attr);
  return; 
}
