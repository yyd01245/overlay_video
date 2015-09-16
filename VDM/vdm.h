#ifndef _H_VDM_
#define _H_VDM_

#include<pthread.h>
#include"logger.h"

#ifdef __cplusplus
extern "C"{
#endif
#include"logger.h"

#ifdef __cplusplus
}
#endif

//pthread_mutex_t glock=PTHREAD_MUTEX_INITIALIZER;

extern logger_t* g_log;



#define _VDM_VERSION "1.0.00.1"



#define VDM_LOG_DEBUG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_DEBUG,"VDM",fmt,##arg)

#define VDM_LOG_MSG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_MSG,"VDM",fmt,##arg)

#define VDM_LOG_WARN(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_WARN,"VDM",fmt,##arg)

#define VDM_LOG_ERROR(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_ERROR,"VDM",fmt,##arg)



#define VDR_LOG_DEBUG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_DEBUG,"VDR",fmt,##arg)

#define VDR_LOG_MSG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_MSG,"VDR",fmt,##arg)

#define VDR_LOG_WARN(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_WARN,"VDR",fmt,##arg)

#define VDR_LOG_ERROR(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_ERROR,"VDR",fmt,##arg)



#define VR_LOG_DEBUG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_DEBUG,"VR",fmt,##arg)

#define VR_LOG_MSG(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_MSG,"VR",fmt,##arg)

#define VR_LOG_WARN(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_WARN,"VR",fmt,##arg)

#define VR_LOG_ERROR(fmt,arg...) \
    logger_log(g_log,LOG_LEVEL_ERROR,"VR",fmt,##arg)








#endif
