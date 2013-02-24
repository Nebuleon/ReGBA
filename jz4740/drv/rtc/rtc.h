#ifndef __RTC_H__
#define __RTC_H__
#include <bits/types.h>
typedef struct{
unsigned short int u16_year;
unsigned char u8_month;
unsigned char u8_week;
unsigned char u8_day;
unsigned char u8_hour;
unsigned char u8_minute;
unsigned char u8_second; 
}ST_datetime; //type define a struct of date time

typedef struct{
unsigned char u8_month;
unsigned char u8_day;
}ST_monthday;

//void atom_time_to_ymd(__uint64_t atom_time ,ST_datetime *st_time);
//__uint64_t ymd_time_to_atom(ST_datetime *currentime);
void Get_curUrtime(ST_datetime * st_dt);
__uint64_t Get_atom_time(void);
__uint64_t ymd_time_to_atom(ST_datetime *currentime);
void atom_time_to_ymd(__uint64_t atom_time ,ST_datetime *st_time);
void Set_RTC_datetime(ST_datetime * st_time);
void Set_RTC_ABS(__uint64_t u64_time);
void Jz_rtc_init(void);
unsigned char Get_week_from_date(unsigned short int year,unsigned char month,unsigned char day);
unsigned char Is_double_year(unsigned short int year);


#endif
