/*
 * Jz OnChip Real Time Clock interface for Linux
 *
 */

#include <jz4740.h>


#define RTC_ALM_READ 0
#define RTC_ALM_SET 1
#define RTC_RD_TIME 2
#define RTC_SET_TIME 3
#define RTC_EPOCH_READ 4
#define RTC_EPOCH_SET 5

#define EINVAL 1
#define EACCES 2
#define EFAULT 3

#include <rtc.h>
//extern spinlock_t rtc_lock;
static __uint64_t st_time_atom;//get time base from RTC(unit second) 
static __uint64_t st_time_atom_base;//get time base from RTC(unit second) 
static ST_datetime st_EU_time;
const ST_datetime st_EU_base={1900,1,0,1,0,0,0};//2008.02.13
const ST_datetime st_EU_start={1921,2,0,8,0,0,0};//2021.02.8
const ST_datetime st_EU_sample={2008,3,0,20,0,0,0};//2008.03.20

static unsigned int u32_RTC_time_l;
static unsigned int u32_RTC_time_h;

const unsigned int sum_monthday[13] = {
0,
31,
31+28,
31+28+31,
31+28+31+30,
31+28+31+30+31,
31+28+31+30+31+30,
31+28+31+30+31+30+31,
31+28+31+30+31+30+31+31,
31+28+31+30+31+30+31+31+30,
31+28+31+30+31+30+31+31+30+31,
31+28+31+30+31+30+31+31+30+31+30,
365
}; //the total day of the last month

const unsigned char monthday[12] = 
{31,28,31,30,31,30,31,31,30,31,30,31};
//1 2  3  4  5  6  7  8  9  10 11 12
//=====================================================================
//function script get the month from yeardays
//input yeardays y(years)
//output return ST_monthday(month and day)
//create date 2008.03.13
//=====================================================================
static void Get_month_from_yeardays(unsigned int yeardays,unsigned short int y,ST_monthday *st_md)
{
unsigned char m_ret=0,m_f,m_i;
unsigned int tmpd;
//ST_monthday st_md; 
//u8_month;
//u8_day;
st_md->u8_month=1;
st_md->u8_day=1;
m_f=Is_double_year(y);
for(m_i=1;m_i<13;m_i++)
   {
   tmpd=sum_monthday[m_i];
   if(m_i>1&&m_f) tmpd+=1;
   if(yeardays<=tmpd) 
       {
       st_md->u8_month=m_i;
       tmpd=sum_monthday[m_i-1];  
       if(m_i>2&&m_f) tmpd+=1;
       st_md->u8_day=yeardays-tmpd;
       break;
       }  
   }//for
if(13==m_i)
    {
    st_md->u8_month=12;
    tmpd=sum_monthday[12];  
    if(m_f) tmpd+=1;
    st_md->u8_day=yeardays-tmpd;
    }
}
//=====================================================================
//function script get atom time from RTC
//input
//output return a format time
//create date 2008.03.13
//=====================================================================
void atom_time_to_ymd(__uint64_t atom_time ,ST_datetime *st_time)
{
ST_datetime tmpt;
unsigned char tmp_i;  //tmpw,
__uint64_t tmpday,tmphour,tmpminute,tmpd,tmpdd,tmpy;
unsigned short int tmpdt[4];
ST_monthday tmpmdd;
//tmpw=Get_week_from_date(st_EU_base.u16_year,st_EU_base.u8_month,st_EU_base.u8_day);
tmpday=atom_time/86400;//24*60*60
tmphour=atom_time%86400;
tmpminute=tmphour%3600;//60*60
tmpt.u8_hour=tmphour/3600;
tmpt.u8_minute=tmpminute/60;
tmpt.u8_second=tmpminute%(60);

tmpd=tmpday/1461; //n 4 year //365*4+1
tmpdd=tmpday%1461;   //double year counter
tmpy=st_EU_base.u16_year+tmpd*4;
if(0==tmpdd) {tmpy-=1;tmpt.u8_month=12;tmpt.u8_day=31;}
else
    {
    for(tmp_i=0;tmp_i<4;tmp_i++)
        {
        tmpdt[tmp_i]=365*(tmp_i+1);
        if(Is_double_year(tmpy+tmp_i)) {tmpdt[tmp_i]+=1;break;}
        }
    for(;tmp_i<4;tmp_i++) tmpdt[tmp_i]=365*(tmp_i+1);
    for(tmp_i=0;tmp_i<4;tmp_i++)
        {
        if(tmpdd<=tmpdt[tmp_i]) {
            tmpy+=tmp_i;
            if(tmp_i) tmpdd-=tmpdt[tmp_i-1];
            break;
            }
        }
    if(4==tmp_i) {tmpy+=3;tmpdd=tmpdt[3]-tmpdt[2];}
    Get_month_from_yeardays(tmpdd,tmpy,&tmpmdd);
    tmpt.u8_month=tmpmdd.u8_month;
    if(tmpmdd.u8_day) tmpt.u8_day=tmpmdd.u8_day;
    else tmpt.u8_day=1;
    }
tmpt.u16_year=tmpy;
tmpt.u8_week=Get_week_from_date(tmpt.u16_year,tmpt.u8_month,tmpt.u8_day);
st_time->u16_year=tmpt.u16_year;
st_time->u8_month=tmpt.u8_month;
st_time->u8_week=tmpt.u8_week;
st_time->u8_day=tmpt.u8_day;
st_time->u8_hour=tmpt.u8_hour;
st_time->u8_minute=tmpt.u8_minute;
st_time->u8_second=tmpt.u8_second;
}

//=====================================================================
//function script current time convert to ABS atom time
//input current time
//output return a atom time
//create date 2008.03.13
//=====================================================================
__uint64_t ymd_time_to_atom(ST_datetime *currentime)
{
__uint64_t tempam_ret=0;
unsigned char f_d,tmp_i;
unsigned short int tmpdt[4],temp_p;
unsigned int tempam;
tempam=currentime->u16_year;
f_d=Is_double_year(tempam);
if(tempam<st_EU_base.u16_year) return 0;
tempam-=st_EU_base.u16_year;
//if(tempam<0) return 0;
temp_p=tempam%4;
tempam=(tempam/4);
tempam_ret=(unsigned int)tempam*1461; //365*4+1 day
tempam=tempam*4+st_EU_base.u16_year;
tmp_i=0;
tmpdt[tmp_i]=0;
for(;tmp_i<4;tmp_i++)
    {
    tmpdt[tmp_i]=365*(tmp_i);
    if(Is_double_year(tempam+tmp_i)) {tmpdt[tmp_i]+=1;break;}
    }
for(;tmp_i<4;tmp_i++) tmpdt[tmp_i]=365*(tmp_i);
tempam_ret+=(__uint64_t)tmpdt[temp_p]; //
if(f_d &&currentime->u8_month>2) {tempam_ret+=(__uint64_t)(sum_monthday[currentime->u8_month-1]+1);tempam_ret+=(__uint64_t)currentime->u8_day;}
else {tempam_ret+=(__uint64_t)sum_monthday[currentime->u8_month-1];tempam_ret+=(__uint64_t)currentime->u8_day;}
tempam_ret*=(__uint64_t)86400;//24*60*60
tempam_ret+=(__uint64_t)(currentime->u8_hour*3600) ;//60*60
tempam_ret+=(__uint64_t)(currentime->u8_minute*60) ;//60
tempam_ret+=(__uint64_t)(currentime->u8_second) ;//
return tempam_ret;
}
//=====================================================================
//function script double year
//input year
//output return yes or no
//create date 2008.03.12
//=====================================================================
unsigned char Is_double_year(unsigned short int year)
{
unsigned char is_ret=0;
unsigned short int y;
y=year%400;
if(0==y||(y!=0&&0==(year%4)))
    is_ret=1;
return is_ret;
}
//=====================================================================
//function script get week from date
//input year month day
//output return week
//create date 2008.03.12
//=====================================================================
unsigned char Get_week_from_date(unsigned short int year,unsigned char month,unsigned char day)
{
unsigned char w=1; //because 1901-01-01 is monday
unsigned short int y,d;
y=year-1+(year-1)/4-(year-1)/100+(year-1)/400;
d=day+sum_monthday[month-1];
if(month>2&&Is_double_year(year))
    {
    d+=1;
    }
y+=d;
w=y%7;
return w;
}
//=====================================================================
//function script Get urtime
//input
//output current urtime
//create date 2008.03.15
//=====================================================================
void Get_curUrtime(ST_datetime *st_sd)
{
unsigned long flags;
ST_datetime *tmp_st=&st_EU_time;
__uint64_t tempam_cur=st_time_atom_base;
flags = spin_lock_irqsave();
tempam_cur += REG_RTC_RSR;
st_time_atom=tempam_cur;
atom_time_to_ymd(tempam_cur,st_sd);
memcpy(tmp_st,st_sd,sizeof(ST_datetime));
//REG_RTC_RSAR = lval;
spin_unlock_irqrestore(flags);
//return tmp_st;
}
//=====================================================================
//function script Get abs time
//input
//output current urtime
//create date 2008.03.15
//=====================================================================
__uint64_t Get_atom_time(void)
{
unsigned long flags;
flags = spin_lock_irqsave();
__uint64_t tempam_cur =st_time_atom_base+REG_RTC_RSR;
st_time_atom=tempam_cur;
spin_unlock_irqrestore(flags);
return tempam_cur;
}
//=====================================================================
//function script Get urtime
//input
//output current urtime
//create date 2008.03.15
//=====================================================================
void Set_RTC_datetime(ST_datetime * st_time)
{
//st_time_atom_base
unsigned long flags;
flags = spin_lock_irqsave();
__uint64_t tempam_cur=ymd_time_to_atom(st_time);
st_time_atom=tempam_cur;
REG_RTC_RSR = tempam_cur-st_time_atom_base;
spin_unlock_irqrestore(flags);
}
//=====================================================================
//function script Set RTC from abs time
//input
//output current urtime
//create date 2008.03.19
//=====================================================================
void Set_RTC_ABS(__uint64_t u64_time)
{
//st_time_atom_base
unsigned long flags;
flags = spin_lock_irqsave();
st_time_atom=u64_time;
REG_RTC_RSR = u64_time-st_time_atom_base;
spin_unlock_irqrestore(flags);
}
//=====================================================================
//function script Get base urtime
//input
//output current urtime
//create date 2008.03.16
//=====================================================================
ST_datetime * Get_baseUrtime(void)
{
return &st_EU_base;
}
//=====================================================================
//function script Get base urtime
//input
//output current urtime
//create date 2008.03.16
//=====================================================================
ST_datetime * Get_basestartUrtime(void)
{
return &st_EU_start;
}
//=====================================================================
//function script RTC initial
//input
//output current urtime
//create date 2008.03.19
//=====================================================================
void Jz_rtc_init(void)
{
unsigned int tmpx=__rtc_get_scratch_pattern();
//printf("%x\n",tmpx);
if(tmpx!= 0x12345678)//&&0==(REG_CPM_RSR&CPM_RSR_HR)
   {
   u32_RTC_time_l=0;
   u32_RTC_time_h=0;
   st_time_atom=ymd_time_to_atom(&st_EU_start); //set base time
   atom_time_to_ymd(st_time_atom,&st_EU_time);  //get base time info
   st_time_atom_base=st_time_atom;

   printf("%x\n",REG_RTC_RSR);
   while(!(REG_RTC_RCR & 0x80));
   REG_RTC_RCR = 1;
   udelay(70);
   while(!(REG_RTC_RCR & 0x80));
   REG_RTC_RGR=0x7fff; 
   udelay(70);
   __rtc_set_scratch_pattern(0x12345678);
   udelay(70);
   REG_RTC_RSR = 0;
   Set_RTC_ABS(st_time_atom_base);
   Set_RTC_datetime(&st_EU_sample);
   }
/*
ST_datetime st_tmpdt;
//Set_RTC_datetime(&st_EU_sample);
st_tmpdt.u16_year=st_EU_sample.u16_year;
st_tmpdt.u8_month=st_EU_sample.u8_month;
st_tmpdt.u8_day=st_EU_sample.u8_day;
st_tmpdt.u8_week=Get_week_from_date(st_EU_sample.u16_year,st_EU_sample.u8_month,st_EU_sample.u8_day);
st_tmpdt.u8_hour=st_EU_sample.u8_hour;
st_tmpdt.u8_minute=st_EU_sample.u8_minute;
st_tmpdt.u8_second=st_EU_sample.u8_second;
Set_RTC_datetime(&st_tmpdt);
*/
}



