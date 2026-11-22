/**
 * @file sntp_time_sync.h
 * @author Daniel Albu (albud456@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2025-12-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef SNTP_TIME_SYNC_H_
#define SNTP_TIME_SYNC_H_

/**
 * @brief starts the NTP server syncronization
 * 
 */
void sntp_time_sync_task_start(void);

/**
 * @brief returns local time if set, produce a local-time string for the web page
 * @return local time buffer
 */
char* sntp_time_sync_get_time(void);

/**
 * @brief checks if the device time is has been synced
 * 
 * @return true 
 * @return false 
 */
bool sntp_time_sync_is_synced(void);

#endif //SNTP_TIME_SYNC_H_