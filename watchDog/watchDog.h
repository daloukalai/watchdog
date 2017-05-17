/**********************************************************************
*
* File:         watchdog.hpp
***********************************************************************/

#include <iostream>
#include <windows.h>
using namespace std;

#ifndef WATCHDOG_HPP_INCLUDED
#define WATCHDOG_HPP_INCLUDED



class WatchdogBase
{
  // Implementation of a monitor which will perform a user-specified action
  // if a timer is allowed to expire.
  //
  // If the time between calls to StartTicker() and StopTicker() exceeds
  // the time specified by SetTickerDelay(value), then Expiry() will
  // be called.
  //
  // Constructing an instance of Watchdog gets everything ready.
  // After construction, IsValid() will be TRUE if all semaphores, threads,
  // etc. have been set up OK.
  //
  // The timeout value is specified in milliseconds, and
  // may be provided either of 2 ways:
  // -- Any execution of StartTicker(value) may specify the new timeout
  //    value.
  // -- After construction but before the first execution of
  //    StartTicker(), use SetTickerDelay(value). If you do this,
  //    your StartTicker() calls can be written without specifying a
  //    value.
  //
  // THIS IS AN ABSTRACT BASE CLASS.
  // You must SUBCLASS this class, and override the virtual Expiry()
  // function.

  public :

    // Constructor & destructor

    WatchdogBase( const char * szDescriptor ) ;

    virtual ~WatchdogBase() ;

    // Operational functions

#define LONG64 long long
#define _NANOS_ 10000LL

    void SetTickerDelay ( unsigned long ulDelay )
    { 
       // Positive value indicates absolute time,
       // negative value indicates relative time (SetWaitableTimer)
       // Convert millisecond value to 100's of nanoseconds.
       ulSaveDelay.QuadPart = -((LONG64)ulDelay) * _NANOS_ ;
    }

    void StartTicker    ( unsigned long ulDelay = 0 ) ;
    void StopTicker     ( void ) ;

    // Status query function

    int  IsValid        ( void ) const            { return fValid ; }

    // Virtual function, implementing the action to be taken if
    // the timer expires

    virtual void Expiry ( void ) = 0 ; // PURE

    enum { NOT_CREATED = -1 } ;

  private :
    HANDLE              hSemWatchdog ;

    LARGE_INTEGER        ulSaveDelay ;   // Timeout value in 100-nanosecond intervals 1e-7

    HANDLE                  thread ;   // Thread id of timer thread
    int                       fValid ;   // Status of object
    int                fShuttingDown ;   // TRUE when DTOR is running

  private :

    // Our own service thread
    static DWORD WINAPI fnWatchdogThread( LPVOID pV ) ;
	static DWORD WINAPI stupidFn( LPVOID pV );
} ;

class Watchdog : public WatchdogBase
{

  // CONCRETE implementation of a WatchdogBase, which takes a pointer
  // to a callback function which will be called by Expiry().

  public :

    // For a concrete instance of Watchdog, the instantiator
    // must provide a function which will be called if the
    // timer expires, as well as a value which will be passed
    // to the function.

    typedef void (*WATCHDOG_EXPIRY_FN)( void *) ;

    // Constructor for use by CONCRETE Watchdog

    Watchdog( const char * szDescriptor, WATCHDOG_EXPIRY_FN pfn, void * pV ) ;
    virtual ~Watchdog() { }

    virtual void Expiry ( void ) ;

  private :

    WATCHDOG_EXPIRY_FN pfnExpiry ;
    void             * pVarg ;

    // User is not allowed to copy this object
    //   ( These functions are not implemented anywhere )
    Watchdog( const Watchdog& ) ;
    Watchdog operator= ( const Watchdog& ) ;
} ;

//-------------------------------------------------------------------------------
//  Inline functions
//-------------------------------------------------------------------------------

inline Watchdog::Watchdog( const char * szDescriptor, WATCHDOG_EXPIRY_FN pfn, void * pV )
  : WatchdogBase( szDescriptor )
  , pfnExpiry   ( pfn )
  , pVarg       ( pV )
{
  // Empty
}

#endif
