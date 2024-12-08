/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <ctime>
#include <iostream>
#include <unistd.h> 

#include "timestamp.hh"
#include "exception.hh"

uint64_t raw_timestamp( void )
{
    timespec ts;
    SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ts ) );

    uint64_t millis = ts.tv_nsec / 1000000;
    millis += uint64_t( ts.tv_sec ) * 1000;

    return millis;
}

uint64_t initial_timestamp( void )
{
    static uint64_t initial_value = raw_timestamp();
    return initial_value;
}

uint64_t timestamp( void )
{
    return raw_timestamp() - initial_timestamp();
}

timespec raw_timestamp_nano( void )
{
    timespec ts;
    SystemCall( "clock_gettime", clock_gettime( CLOCK_REALTIME, &ts ) );

    return ts;
}

uint64_t timestamp_nano( void )
{   
    static timespec init_ts = raw_timestamp_nano();
    timespec raw_ts = raw_timestamp_nano();
    uint64_t nano = raw_ts.tv_nsec-init_ts.tv_nsec + (raw_ts.tv_sec-init_ts.tv_sec) * 1000000000;
    return nano;
}
