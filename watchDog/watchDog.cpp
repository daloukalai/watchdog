////////////////////////////////////////////////////////////////////////
//
// File:       watchdog.cpp

////////////////////////////////////////////////////////////////////////


#include "watchDog.h"

////////////////////////////////
// WatchdogBase - Constructor
////////////////////////////////
//

WatchdogBase::WatchdogBase( const char* szDescriptor )
 : hSemWatchdog( 0 )
 , fValid      ( FALSE )
 , fShuttingDown( 0 )
{
   int rc ;

   SetTickerDelay( 5 * 60 * 1000 ) ; // Default is 5 minutes
   hSemWatchdog = CreateWaitableTimer( NULL, FALSE, NULL ) ;

   // FALSE => synchronization timer (A timer whose state remains
   // signaled until a thread completes a wait operation on the timer object.)

   if ( hSemWatchdog == NULL )
   {
      rc = ::GetLastError() ;
      OutputDebugString( L"CreateWaitableTimer for Watchdog failed") ;
   }
   else
   {
      // Start our own timer thread
	   DWORD threadld;
      thread = CreateThread(NULL,0, &fnWatchdogThread, this,0, &threadld ) ;
      if ( thread == NULL )
        OutputDebugString( L"Create Watchdog thread failed." ) ;
      else
      {
         for ( int iRepeat = 20 ; iRepeat > 0 ; --iRepeat )
         {
            Sleep(500) ; // Give up timeslice, allow thread to be created
            if ( fValid )
               break ;
         }

         if ( !fValid )
            OutputDebugString( L"Watchdog thread failed to start after 10 seconds." ) ;
      }
   }
}

////////////////////////////////
// WatchdogBase - Destructor
////////////////////////////////
//

WatchdogBase::~WatchdogBase( )
{
   int rc ;
   if ( thread != NULL )
   {
      fShuttingDown = 1 ; // Ready to kill thread

      if ( hSemWatchdog )
      {
        if ( ! CancelWaitableTimer( hSemWatchdog ) )
        {
          rc = ::GetLastError() ;
          OutputDebugString( L"~WatchdogBase: CancelWaitableTimer failed, rc") ;
        }
      }
   }

   StopTicker( );

   if ( hSemWatchdog )
   {
      if ( ! CloseHandle( hSemWatchdog ) )
      {
        rc = ::GetLastError() ;
        OutputDebugString( L"~WatchdogBase: CloseHandle failed, rc" ) ;
      }
   }
}

DWORD WINAPI WatchdogBase::stupidFn( LPVOID pV )
{
	return 0;
}
////////////////////////////////
// fnWatchdogThread
////////////////////////////////
//
DWORD WINAPI WatchdogBase::fnWatchdogThread( LPVOID pV )
{
   WatchdogBase * pWatchdog = (Watchdog*)pV;
   int         rc ;
#if defined(_WIN32)
   pWatchdog->fValid = TRUE ; // Ready to rock and roll

   for ( ;; )
   {
      // Wait 'forever' for the timer to expire
      rc = WaitForSingleObject( pWatchdog->hSemWatchdog, INFINITE ) ;

      if ( pWatchdog->fShuttingDown )
         return 0;

      // Only get here if user didn't run StopTicker() in time.
      //   "Arf! Arf!"

      if ( rc != WAIT_OBJECT_0 )
      {
        OutputDebugString( L"fnWatchdogThread: WaitForSingleObject got unexpected rc") ;
      }

      // Do what the user wants
      pWatchdog->Expiry() ;
   }
#endif
   return 0;
   // Can NEVER get here !
}


////////////////////////////////
//    StopTicker
////////////////////////////////
//

void WatchdogBase::StopTicker( void )
{
   int   rc;
#if defined(_WIN32)
   if ( hSemWatchdog )
   {
     if ( ! CancelWaitableTimer( hSemWatchdog ) )
     {
       rc = ::GetLastError() ;
       OutputDebugString( L"StopTicker: CancelWaitableTimer failed, rc.") ;
     }
   }
#endif
}

////////////////////////////////
//    StartTicker
////////////////////////////////
//

void WatchdogBase::StartTicker( unsigned long ulDelay )
{
   if ( ulDelay )
      SetTickerDelay( ulDelay ) ;

   // Allow user to call StartTicker() several times in a row
   // without an intervening StopTicker()
   StopTicker( );
#if defined(_WIN32)
   if ( hSemWatchdog )
   {
      if ( ! SetWaitableTimer( hSemWatchdog,
                               &ulSaveDelay,
                               0,           // Period == 0 means timer is only signalled once
                               NULL,        // Optional completion routine
                               NULL,        // Parm passed to optional completion routine as (void*)
                               FALSE
                             )
         )
      {
        int rc = ::GetLastError() ;
        OutputDebugString( L"SetWaitableTimer error." ) ;
      }
   }
#endif
}

////////////////////////////////////////////////////////////////////////
//  Implementation of concrete Watchdog (subclass of WatchdogBase)
////////////////////////////////////////////////////////////////////////
//
////////////////////////////////
//    Expiry
////////////////////////////////
//

void  Watchdog::Expiry( void )
{
   // Call users function
   if ( pfnExpiry != NULL )
      pfnExpiry( pVarg );
}

void sleepFn(void *)
{
	// This function will hit because we purposefully did a sleep() in main.
	cout << "Function to hit after the time expiry!" << endl;
}
void main()
{
	const char * fnDesc = "SleepFnDescription";
	 Watchdog wDog(fnDesc, sleepFn, 0);
	 
	 // Start a ticker with 2 seconds.
	 // This expects the code flow after it,
	 //should execute within 2 seconds. Otherwise, its watchDog thread
	 // will call the expiry function.
	 wDog.StartTicker(2000);

	 // Purposefully do sleep activity for 4 secs.
	 // This is more than the watchdog expected time.
	 Sleep(4000);

	 // THis resets the timer
	 wDog.StopTicker();
}
// end of Watchdog.CPP







