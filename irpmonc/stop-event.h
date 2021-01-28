
#ifndef __STOP_EVENT_H__
#define __STOP_EVENT_H__



#ifdef __cplusplus
extern "C" {
#endif

int stop_event_create(void);
int stop_event_oepn(int ProcessId);
int stop_event_wait(unsigned int Timeout);
void stop_event_set(void);
void stop_event_finit(void);


#ifdef __cplusplus
}
#endif


#endif
