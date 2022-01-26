/*
Copyright 2021 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./lorawan.h"
#include "./main.h"

#include "version.h"

// Nuvonton
#include "M480.h"

// LoraWAN-node
#include "system/gpio.h"
#include "system/timer.h"
#include "system/nvmm.h"
#include "board.h"
#include "sx126x-board.h"

extern "C" {

void DelayMsMcu( uint32_t ms ) {
    size_t usec = ms * 1000;
    asm volatile (
        "1:\n\t"
#define NOP6 "nop\n\tnop\n\tnop\n\tnop\n\tnop\n\tnop\n\t"
#define NOP24 NOP6 NOP6 NOP6 NOP6
#define NOP96 NOP24 NOP24 NOP24 NOP24
        NOP96
        "subs %[usec], #1\n\t"
        "bge 1b\n\t"
    : : [usec] "r" (usec) : );
}

void BoardCriticalSectionBegin( uint32_t *mask ) {
    __disable_irq( );
}

void BoardCriticalSectionEnd( uint32_t *mask ) {
    __enable_irq( );
}

void BoardResetMcu( void ) {
    CRITICAL_SECTION_BEGIN( );
    NVIC_SystemReset( );
    CRITICAL_SECTION_END( );
}

void BoardGetUniqueId( uint8_t *id ) {
    uint32_t ID[4];

    ID[0] = SYS_ReadPDID();
    ID[1] = FMC_ReadUID(0);
    ID[2] = FMC_ReadUID(1);
    ID[3] = FMC_ReadUID(2);

    id[7] = ( ( ID[0] ) + ( ID[3] ) ) >> 24;
    id[6] = ( ( ID[0] ) + ( ID[3] ) ) >> 16;
    id[5] = ( ( ID[0] ) + ( ID[3] ) ) >>  8;
    id[4] = ( ( ID[0] ) + ( ID[3] ) ) >>  0;
    id[3] = ( ( ID[1] ) + ( ID[4] ) ) >> 24;
    id[2] = ( ( ID[1] ) + ( ID[4] ) ) >> 16;
    id[1] = ( ( ID[1] ) + ( ID[4] ) ) >>  8;
    id[0] = ( ( ID[1] ) + ( ID[4] ) ) >>  0;
}

uint8_t BoardGetBatteryLevel( void ) {
    return 0;
}

static RadioOperatingModes_t OperatingMode = MODE_SLEEP;

void SX126xIoInit( void ) {

    // RF_NSS
    GPIO_SetMode(PA, BIT3, GPIO_MODE_OUTPUT);
    GPIO_SetPullCtl(PA, BIT3, GPIO_PUSEL_DISABLE);
    PA3 = 0;
    // RF_NRESET
    GPIO_SetMode(PA, BIT4, GPIO_MODE_OUTPUT);
    GPIO_SetPullCtl(PA, BIT4, GPIO_PUSEL_DISABLE);
    PA4 = 1;
    // RF_DIO1
    GPIO_SetMode(PC, BIT0, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT0, GPIO_PUSEL_DISABLE);
    GPIO_EnableInt(PC, 0, GPIO_INT_RISING);
    PC0 = 0;
    // RF_BUSY
    GPIO_SetMode(PC, BIT1, GPIO_MODE_INPUT);
    GPIO_SetPullCtl(PC, BIT1, GPIO_PUSEL_DISABLE);
    PC1 = 0;

    SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 10000000);
}

void SX126xIoIrqInit( DioIrqHandler dioIrq ) {
    // NOP
}

void SX126xIoDeInit( void ) {
    // NOP
}

void SX126xIoDbgInit( void ) {
    // NOP
}

void SX126xIoTcxoInit( void ) {
    // NOP
}

uint32_t SX126xGetBoardTcxoWakeupTime( void ) {
    return 0;
}

void SX126xIoRfSwitchInit( void ) {
    SX126xSetDio2AsRfSwitchCtrl( true );
}

RadioOperatingModes_t SX126xGetOperatingMode( void ) {
    return OperatingMode;
}

void SX126xSetOperatingMode( RadioOperatingModes_t mode ) {
    OperatingMode = mode;
}

void SX126xReset( void ) {
    DelayMsMcu( 10 );
    PA4 = 0;
    DelayMsMcu( 20 );
    PA4 = 1;
    DelayMsMcu( 10 );
}

void SX126xWaitOnBusy( void ) {
    // RF_BUSY
    while(PC1 == 1) {}
}

void SX126xWakeup( void ) {
    CRITICAL_SECTION_BEGIN( );

    // RF_NSS
    PA3 = 0;

    while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
    SPI_WRITE_TX(SPI0, RADIO_GET_STATUS);
    while(SPI_GET_TX_FIFO_FULL_FLAG(SPI0) == 1) {}
    SPI_WRITE_TX(SPI0, 0x00);
    while(SPI_GET_TX_FIFO_EMPTY_FLAG(SPI0) == 0) {}

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );

    SX126xSetOperatingMode( MODE_STDBY_RC );

    CRITICAL_SECTION_END( );
}

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size ) {
    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, (uint8_t)command);
    while(SPI_IS_BUSY(SPI0));
    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

uint8_t SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size ) {
    uint8_t status = 0;

    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, (uint8_t)command);
    while(SPI_IS_BUSY(SPI0));

    SPI_WRITE_TX(SPI0, 0);
    while(SPI_IS_BUSY(SPI0));
    status = SPI_READ_RX(SPI0);

    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );

    return status;
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size ) {
    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;
    
    SPI_WRITE_TX(SPI0, RADIO_WRITE_REGISTER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, ( address & 0xFF00 ) >> 8);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, ( address & 0x00FF ) >> 0);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value ) {
    SX126xWriteRegisters( address, &value, 1 );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size ) {
    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;
    
    SPI_WRITE_TX(SPI0, RADIO_READ_REGISTER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, ( address & 0xFF00 ) >> 8);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, ( address & 0x00FF ) >> 0);
    while(SPI_IS_BUSY(SPI0));

    SPI_WRITE_TX(SPI0, 0);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address ) {
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, RADIO_WRITE_BUFFER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, offset);
    while(SPI_IS_BUSY(SPI0));

    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, buffer[i]);
        while(SPI_IS_BUSY(SPI0));
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size ) {
    SX126xCheckDeviceReady( );

    // RF_NSS
    PA3 = 0;

    SPI_WRITE_TX(SPI0, RADIO_READ_BUFFER);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, offset);
    while(SPI_IS_BUSY(SPI0));
    SPI_WRITE_TX(SPI0, 0);

    for(size_t i = 0; i < (size_t)size; i++ ) {
        SPI_WRITE_TX(SPI0, 0);
        while(SPI_IS_BUSY(SPI0));
        buffer[i] = SPI_READ_RX(SPI0);
    }

    // RF_NSS
    PA3 = 1;

    SX126xWaitOnBusy( );
}

void SX126xSetRfTxPower( int8_t power ) {
    SX126xSetTxParams( power, RADIO_RAMP_40_US );
}

uint8_t SX126xGetDeviceId( void ) {
    return SX1261;
}

void SX126xAntSwOn( void ) {
    // NOP
}

void SX126xAntSwOff( void ) {
    // NOP
}

bool SX126xCheckRfFrequency( uint32_t frequency ) {
    return true;
}

uint32_t SX126xGetDio1PinState( void ) {
    return PC0;
}

void RtcInit( void )
{
/*    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;

    if( RtcInitialized == false )
    {
        __HAL_RCC_RTC_ENABLE( );

        RtcHandle.Instance            = RTC;
        RtcHandle.Init.HourFormat     = RTC_HOURFORMAT_24;
        RtcHandle.Init.AsynchPrediv   = PREDIV_A;  // RTC_ASYNCH_PREDIV;
        RtcHandle.Init.SynchPrediv    = PREDIV_S;  // RTC_SYNCH_PREDIV;
        RtcHandle.Init.OutPut         = RTC_OUTPUT_DISABLE;
        RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        RtcHandle.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
        HAL_RTC_Init( &RtcHandle );

        date.Year                     = 0;
        date.Month                    = RTC_MONTH_JANUARY;
        date.Date                     = 1;
        date.WeekDay                  = RTC_WEEKDAY_MONDAY;
        HAL_RTC_SetDate( &RtcHandle, &date, RTC_FORMAT_BIN );

        time.Hours                    = 0;
        time.Minutes                  = 0;
        time.Seconds                  = 0;
        time.SubSeconds               = 0;
        time.TimeFormat               = 0;
        time.StoreOperation           = RTC_STOREOPERATION_RESET;
        time.DayLightSaving           = RTC_DAYLIGHTSAVING_NONE;
        HAL_RTC_SetTime( &RtcHandle, &time, RTC_FORMAT_BIN );

        // Enable Direct Read of the calendar registers (not through Shadow registers)
        HAL_RTCEx_EnableBypassShadow( &RtcHandle );

        HAL_NVIC_SetPriority( RTC_Alarm_IRQn, 1, 0 );
        HAL_NVIC_EnableIRQ( RTC_Alarm_IRQn );

        // Init alarm.
        HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A );

        RtcSetTimerContext( );
        RtcInitialized = true;
    }*/
}

/*!
 * \brief Sets the RTC timer reference, sets also the RTC_DateStruct and RTC_TimeStruct
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcSetTimerContext( void )
{
//    RtcTimerContext.Time = ( uint32_t )RtcGetCalendarValue( &RtcTimerContext.CalendarDate, &RtcTimerContext.CalendarTime );
//    return ( uint32_t )RtcTimerContext.Time;
    return 0;
}

/*!
 * \brief Gets the RTC timer reference
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t RtcGetTimerContext( void )
{
//    return RtcTimerContext.Time;
    return 0;
}

/*!
 * \brief returns the wake up time in ticks
 *
 * \retval wake up time in ticks
 */
uint32_t RtcGetMinimumTimeout( void )
{
    return 0;
}

/*!
 * \brief converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval returns time in timer ticks
 */
uint32_t RtcMs2Tick( uint32_t milliseconds )
{
    return 0;
}

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t RtcTick2Ms( uint32_t tick )
{
    return 0;
}

/*!
 * \brief a delay of delay ms by polling RTC
 *
 * \param[IN] delay in ms
 */
void RtcDelayMs( uint32_t delay )
{
/*    uint64_t delayTicks = 0;
    uint64_t refTicks = RtcGetTimerValue( );

    delayTicks = RtcMs2Tick( delay );

    // Wait delay ms
    while( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks )
    {
        __NOP( );
    }*/
}

/*!
 * \brief Sets the alarm
 *
 * \note The alarm is set at now (read in this function) + timeout
 *
 * \param timeout Duration of the Timer ticks
 */
void RtcSetAlarm( uint32_t timeout )
{
/*    // We don't go in Low Power mode for timeout below MIN_ALARM_DELAY
    if( ( int64_t )MIN_ALARM_DELAY < ( int64_t )( timeout - RtcGetTimerElapsedTime( ) ) )
    {
        LpmSetStopMode( LPM_RTC_ID, LPM_ENABLE );
    }
    else
    {
        LpmSetStopMode( LPM_RTC_ID, LPM_DISABLE );
    }

    RtcStartAlarm( timeout );*/
}

void RtcStopAlarm( void )
{
/*    // Disable the Alarm A interrupt
    HAL_RTC_DeactivateAlarm( &RtcHandle, RTC_ALARM_A );

    // Clear RTC Alarm Flag
    __HAL_RTC_ALARM_CLEAR_FLAG( &RtcHandle, RTC_FLAG_ALRAF );

    // Clear the EXTI's line Flag for RTC Alarm
    __HAL_RTC_ALARM_EXTI_CLEAR_FLAG( );*/
}

void RtcStartAlarm( uint32_t timeout )
{
/*    uint16_t rtcAlarmSubSeconds = 0;
    uint16_t rtcAlarmSeconds = 0;
    uint16_t rtcAlarmMinutes = 0;
    uint16_t rtcAlarmHours = 0;
    uint16_t rtcAlarmDays = 0;
    RTC_TimeTypeDef time = RtcTimerContext.CalendarTime;
    RTC_DateTypeDef date = RtcTimerContext.CalendarDate;

    RtcStopAlarm( );

    //reverse counter 
    rtcAlarmSubSeconds =  PREDIV_S - time.SubSeconds;
    rtcAlarmSubSeconds += ( timeout & PREDIV_S );
    // convert timeout  to seconds
    timeout >>= N_PREDIV_S;

    // Convert microsecs to RTC format and add to 'Now'
    rtcAlarmDays =  date.Date;
    while( timeout >= TM_SECONDS_IN_1DAY )
    {
        timeout -= TM_SECONDS_IN_1DAY;
        rtcAlarmDays++;
    }

    // Calc hours
    rtcAlarmHours = time.Hours;
    while( timeout >= TM_SECONDS_IN_1HOUR )
    {
        timeout -= TM_SECONDS_IN_1HOUR;
        rtcAlarmHours++;
    }

    // Calc minutes
    rtcAlarmMinutes = time.Minutes;
    while( timeout >= TM_SECONDS_IN_1MINUTE )
    {
        timeout -= TM_SECONDS_IN_1MINUTE;
        rtcAlarmMinutes++;
    }

    // Calc seconds
    rtcAlarmSeconds =  time.Seconds + timeout;

    // Correct for modulo
    while( rtcAlarmSubSeconds >= ( PREDIV_S + 1 ) )
    {
        rtcAlarmSubSeconds -= ( PREDIV_S + 1 );
        rtcAlarmSeconds++;
    }

    while( rtcAlarmSeconds >= TM_SECONDS_IN_1MINUTE )
    { 
        rtcAlarmSeconds -= TM_SECONDS_IN_1MINUTE;
        rtcAlarmMinutes++;
    }

    while( rtcAlarmMinutes >= TM_MINUTES_IN_1HOUR )
    {
        rtcAlarmMinutes -= TM_MINUTES_IN_1HOUR;
        rtcAlarmHours++;
    }

    while( rtcAlarmHours >= TM_HOURS_IN_1DAY )
    {
        rtcAlarmHours -= TM_HOURS_IN_1DAY;
        rtcAlarmDays++;
    }

    if( date.Year % 4 == 0 ) 
    {
        if( rtcAlarmDays > DaysInMonthLeapYear[date.Month - 1] )
        {
            rtcAlarmDays = rtcAlarmDays % DaysInMonthLeapYear[date.Month - 1];
        }
    }
    else
    {
        if( rtcAlarmDays > DaysInMonth[date.Month - 1] )
        {   
            rtcAlarmDays = rtcAlarmDays % DaysInMonth[date.Month - 1];
        }
    }

    // Set RTC_AlarmStructure with calculated values
    RtcAlarm.AlarmTime.SubSeconds     = PREDIV_S - rtcAlarmSubSeconds;
    RtcAlarm.AlarmSubSecondMask       = ALARM_SUBSECOND_MASK; 
    RtcAlarm.AlarmTime.Seconds        = rtcAlarmSeconds;
    RtcAlarm.AlarmTime.Minutes        = rtcAlarmMinutes;
    RtcAlarm.AlarmTime.Hours          = rtcAlarmHours;
    RtcAlarm.AlarmDateWeekDay         = ( uint8_t )rtcAlarmDays;
    RtcAlarm.AlarmTime.TimeFormat     = time.TimeFormat;
    RtcAlarm.AlarmDateWeekDaySel      = RTC_ALARMDATEWEEKDAYSEL_DATE; 
    RtcAlarm.AlarmMask                = RTC_ALARMMASK_NONE;
    RtcAlarm.Alarm                    = RTC_ALARM_A;
    RtcAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    RtcAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

    // Set RTC_Alarm
    HAL_RTC_SetAlarm_IT( &RtcHandle, &RtcAlarm, RTC_FORMAT_BIN );*/
}

uint32_t RtcGetTimerValue( void )
{
/*    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    uint32_t calendarValue = ( uint32_t )RtcGetCalendarValue( &date, &time );

    return( calendarValue );*/
    return 0;
}

uint32_t RtcGetTimerElapsedTime( void )
{
/*  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;
  
  uint32_t calendarValue = ( uint32_t )RtcGetCalendarValue( &date, &time );

  return( ( uint32_t )( calendarValue - RtcTimerContext.Time ) );*/
  return 0;
}

uint32_t RtcGetCalendarTime( uint16_t *milliseconds )
{
/*    RTC_TimeTypeDef time ;
    RTC_DateTypeDef date;
    uint32_t ticks;

    uint64_t calendarValue = RtcGetCalendarValue( &date, &time );

    uint32_t seconds = ( uint32_t )( calendarValue >> N_PREDIV_S );

    ticks =  ( uint32_t )calendarValue & PREDIV_S;

    *milliseconds = RtcTick2Ms( ticks );

    return seconds;*/
    return 0;
}

/*!
 * \brief RTC IRQ Handler of the RTC Alarm
 */
void RTC_Alarm_IRQHandler( void )
{
/*    RTC_HandleTypeDef* hrtc = &RtcHandle;

    // Enable low power at irq
    LpmSetStopMode( LPM_RTC_ID, LPM_ENABLE );

    // Clear the EXTI's line Flag for RTC Alarm
    __HAL_RTC_ALARM_EXTI_CLEAR_FLAG( );

    // Gets the AlarmA interrupt source enable status
    if( __HAL_RTC_ALARM_GET_IT_SOURCE( hrtc, RTC_IT_ALRA ) != RESET )
    {
        // Gets the pending status of the AlarmA interrupt
        if( __HAL_RTC_ALARM_GET_FLAG( hrtc, RTC_FLAG_ALRAF ) != RESET )
        {
            // Clear the AlarmA interrupt pending bit
            __HAL_RTC_ALARM_CLEAR_FLAG( hrtc, RTC_FLAG_ALRAF ); 
            // AlarmA callback
            HAL_RTC_AlarmAEventCallback( hrtc );
        }
    }*/
}

void RtcBkupWrite( uint32_t data0, uint32_t data1 )
{
//    HAL_RTCEx_BKUPWrite( &RtcHandle, RTC_BKP_DR0, data0 );
//    HAL_RTCEx_BKUPWrite( &RtcHandle, RTC_BKP_DR1, data1 );
}

void RtcBkupRead( uint32_t *data0, uint32_t *data1 )
{
//  *data0 = HAL_RTCEx_BKUPRead( &RtcHandle, RTC_BKP_DR0 );
//  *data1 = HAL_RTCEx_BKUPRead( &RtcHandle, RTC_BKP_DR1 );
}

void RtcProcess( void )
{
    // Not used on this platform.
}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
/*    float k = RTC_TEMP_COEFFICIENT;
    float kDev = RTC_TEMP_DEV_COEFFICIENT;
    float t = RTC_TEMP_TURNOVER;
    float tDev = RTC_TEMP_DEV_TURNOVER;
    float interim = 0.0f;
    float ppm = 0.0f;

    if( k < 0.0f )
    {
        ppm = ( k - kDev );
    }
    else
    {
        ppm = ( k + kDev );
    }
    interim = ( temperature - ( t - tDev ) );
    ppm *=  interim * interim;

    // Calculate the drift in time
    interim = ( ( float ) period * ppm ) / 1000000.0f;
    // Calculate the resulting time period
    interim += period;
    interim = floor( interim );

    if( interim < 0.0f )
    {
        interim = ( float )period;
    }

    // Calculate the resulting period
    return ( TimerTime_t ) interim;*/
    return 0;
}

LmnStatus_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    LmnStatus_t status = LMN_STATUS_ERROR;

/*    assert_param( ( FLASH_EEPROM_BASE + addr ) >= FLASH_EEPROM_BASE );
    assert_param( buffer != NULL );
    assert_param( size < ( FLASH_EEPROM_END - FLASH_EEPROM_BASE ) );

    if( HAL_FLASHEx_DATAEEPROM_Unlock( ) == HAL_OK )
    {
        CRITICAL_SECTION_BEGIN( );
        for( uint16_t i = 0; i < size; i++ )
        {
            if( HAL_FLASHEx_DATAEEPROM_Program( FLASH_TYPEPROGRAMDATA_BYTE,
                                                ( FLASH_EEPROM_BASE + addr + i ),
                                                  buffer[i] ) != HAL_OK )
            {
                // Failed to write EEPROM
                break;
            }
        }
        CRITICAL_SECTION_END( );
        status = LMN_STATUS_OK;
    }

    HAL_FLASHEx_DATAEEPROM_Lock( );*/
    return status;
}

LmnStatus_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
/*    assert_param( ( FLASH_EEPROM_BASE + addr ) >= FLASH_EEPROM_BASE );
    assert_param( buffer != NULL );
    assert_param( size < ( FLASH_EEPROM_END - FLASH_EEPROM_BASE ) );

    memcpy1( buffer, ( uint8_t* )( FLASH_EEPROM_BASE + addr ), size );*/
    return LMN_STATUS_OK;
}

void EepromMcuSetDeviceAddr( uint8_t addr )
{
//    assert_param( LMN_STATUS_ERROR );
}

LmnStatus_t EepromMcuGetDeviceAddr( void )
{
    LmnStatus_t status = LMN_STATUS_ERROR;
 //   assert_param( LMN_STATUS_ERROR );
    return status;
}

}
