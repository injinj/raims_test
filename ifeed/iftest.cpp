/* Copyright (c) 2009 Rai Technology.  All rights reserved.
 *  http://www.raitechnology.com */

#if ! defined(NO_WHATLIST) && defined(__GNUC__)
static char CVS_ID_api__iftest_cpp[] __attribute__ ((__unused__)) = "$Header$";
#endif /* ! defined(NO_WHATLIST) */
#pragma GCC diagnostic ignored "-Wdeprecated"
/* for keyboard interrupt (ctrl-c) */
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
#include <signal.h>
#else
#include <windows.h>
#endif
#include "raiapi2.h"
#include "base/thread.h"
#include "stream/io_stream.h"
#include "stream/file_stream.h"
#include "stream/byte_array_stream.h"
#include "msg/sass_const.h"

using namespace rai;

namespace {

struct IFTestArgs {
  const StringArg subject_arg;

  IFTestArgs() :
    subject_arg("subject", "-", "<subject> ...",
                "Subject name(s) to subscribe, use '-' to read "
                "subscriptions from stdin" ) {}

  void getArgs( Args &args ) const throw( RaiException ) {
    args.add( &subject_arg, COMMAND_ARG | RESOURCE_ARG |
                            LIST_ARG );
  }
};

struct IFTest : public RaiMsgCallback, public RaiTimerCallback, public Thread {
  RaiApi       * api;
  RaiSession   * session;
  RaiQueue     * queue;
  RaiSubscribe * isub;
  RaiPublish   * pub;
  bool           quit;

  IFTest() : Thread( "iftest-dispatch" ),
             api( 0 ), session( 0 ), queue( 0 ), isub( 0 ), pub( 0 ),
             quit( false ), head( 0 ), tail( 0 ) {}
  ~IFTest() {
    if ( this->pub != NULL )
      delete this->pub;
    if ( this->isub != NULL )
      delete this->isub;
    if ( this->queue != NULL )
      delete this->queue;
    if ( this->session != NULL )
      delete this->session;
    if ( this->api != NULL )
      delete this->api;
  }

  struct SubjList {
    SubjList * next;
    TimeHires  startTime;
    ullong     msgCount;
    RaiTimer * timer;
    char     * subj;
    char    ** id;
    size_t     idCount;

    SYS_OPS( SubjList );
    SubjList() : next( 0 ), startTime( Time::getHiresTime() ),
      msgCount( 1 ), timer( NULL ), subj( NULL ), id( 0 ), idCount( 0 ) {}
    ~SubjList() {
      if ( this->timer != NULL )
        delete this->timer;
      if ( this->subj != NULL )
        FREE( this->subj );
      if ( this->id != NULL )
        FREE( this->id );
    }

    void add( const char *str ) {
      for ( size_t i = 0; i < this->idCount; i++ )
        if ( ::strcmp( str, this->id[ i ] ) == 0 )
          return;
      REALLOC( sizeof( this->id[ 0 ] ) * ( this->idCount + 1 ), &this->id );
      this->id[ this->idCount ] = NULL;
      STRDUP( this->id[ this->idCount ], str );
      this->idCount++;
    }
    void remove( const char *str ) {
      for ( size_t i = 0; i < this->idCount; i++ ) {
        if ( ::strcmp( str, this->id[ i ] ) == 0 ) {
          FREE( this->id[ i ]  );
          for ( size_t j = i + 1; j < this->idCount; )
            this->id[ i++ ] = this->id[ j++ ];
          this->idCount--;
          return;
        }
      }
    }
  };

  SubjList * head, * tail;

  int startSubj( const char *subj,  const char *id,  int ival ) {
    SubjList * sl = NEW SubjList();
    STRDUP( sl->subj, subj );
    sl->add( id );
    sl->timer = this->queue->CreateTimer( this, sl );
    sl->timer->SetInterval( ival );
    sl->timer->Start();
    if ( this->tail == NULL )
      this->head = sl;
    else
      this->tail->next = sl;
    this->tail = sl;
    return sl->idCount;
  }

  int stopSubj( const char *subj,  const char *id ) {
    SubjList * sl = NULL;
    if ( this->head == NULL ) {
    notfound:;
      Sys::out->printf( "%s not found\n", subj );
      Sys::out->flush();
      return 0;
    }
    sl = this->head;
    if ( ::strcmp( sl->subj, subj ) == 0 ) {
      sl->remove( id );
      if ( sl->idCount != 0 )
        return sl->idCount;
      this->head = sl->next;
      if ( this->head == NULL )
        this->tail = NULL;
    }
    else {
      for ( ; sl->next != NULL; sl = sl->next ) {
        if ( ::strcmp( sl->next->subj, subj ) == 0 ) {
          sl->next->remove( id );
          if ( sl->next->idCount != 0 )
            return sl->next->idCount;
          SubjList * tmp = sl->next;
          sl->next = sl->next->next;
          if ( sl->next == NULL )
            this->tail = sl;
          sl = tmp;
          goto foundit;
        }
      }
      goto notfound;
    foundit:;
    }
    sl->timer->Stop();
    delete sl;
    return 0;
  }

  SubjList *findSubj( const char *subj,  const char *id ) {
    for ( SubjList *sl = this->head; sl != NULL; sl = sl->next ) {
      if ( ::strcmp( sl->subj, subj ) == 0 ) {
        sl->add( id );
        return sl;
      }
    }
    return NULL;
  }

  virtual void onMsg( RaiMsgEvent &event,  RaiMsg &raiMsg,  void *closure ) {
    const char * subj, * id = NULL;
    bool is_start = false, reassert = false;
    if ( ::strstr( event.subject, "LISTEN.START" ) != NULL ) {
      subj = &event.subject[ 29 ];
      is_start = true;
    }
    else {
      subj = &event.subject[ 28 ];
      is_start = false;
    }
    RaiField field;
    if ( raiMsg.Get( "id", field ) )
      id = (const char *) field.Data();
    if ( id == NULL ) {
      Sys::err->printf( "no id found\n" );
      return;
    }
    int cnt = 0;
    if ( is_start ) {
      raiMsg.Print( Sys::out );
      SubjList * sl;
      if ( (sl = this->findSubj( subj, id )) != NULL ) {
        Sys::out->printf( "Reassert subject %s (%s) cnt=%d\n", subj, id,
                          (int) sl->idCount );
        Sys::out->flush();
        cnt = sl->msgCount;
        reassert = true;
      }
    }
    if ( is_start ) {
      RaiMsg msg;
      msg.Append( "MSG_TYPE", (Rai_u16) 8 );
      msg.Append( "SEQ_NO", (Rai_u16) cnt );
      msg.Append( "REC_STATUS", (Rai_u16) 0 );
      msg.Append( "ASK", (Rai_f64) ( 1350 + cnt ) );
      msg.Append( "BID", (Rai_f64) ( 1360 + cnt ) );
      this->pub->Publish( subj, msg );
      if ( ! reassert )
        cnt = this->startSubj( subj, id, 1000 );
    }
    else {
      cnt = this->stopSubj( subj, id );
    }
    Sys::out->printf( "%s subject %s (%s) cnt=%d\n",
                      is_start ? "Start" : "Stop", subj, id, cnt );
    Sys::out->flush();
  }

  virtual void onTimer( RaiTimer &timer,  void *closure ) {
    SubjList *sl = (SubjList *) closure;
    RaiMsg msg;
    if ( ::memcmp( sl->subj, "HEAR", 4 ) == 0 )
      msg.Append( "HELLO", "WORLD" );
    else {
      msg.Append( "MSG_TYPE", (Rai_u16) 1 );
      msg.Append( "SEQ_NO", (Rai_u16) sl->msgCount++ );
      msg.Append( "REC_STATUS", (Rai_u16) 0 );
      msg.Append( "ASK", (Rai_f64) ( 1351 + sl->msgCount + 2 ) );
      msg.Append( "BID", (Rai_f64) ( 1361 + sl->msgCount ) );
    }
    this->pub->Publish( sl->subj, msg );
  }

  bool init( RaiApi *apip,  Args &args ) throw( RaiException ) {
    this->api = apip;
    try {
      IFTestArgs iargs;
      const char * subjname = args.getString( iargs.subject_arg.name );

      RaiApi::OpenLog( args );
      this->api->ParseArgs( args );

      this->session = this->api->CreateSession();
      this->session->Start();
      this->queue   = this->session->CreateQueue( false );

      this->isub  = this->queue->CreateSubscribe( this );
      this->isub->Start( subjname, RaiSubscribe::UPDATE, 0 );
      this->pub   = this->session->CreatePublish();

      this->api->PrintLog( LMINOR, "Starting subject %s", subjname );
      this->startSubj( "HEARTBEAT.primary", "me", 1000 );
      return true;
    } catch ( RaiException e ) {
      this->api->PrintLog( LERROR, e, "Not initialized, stopped" );
    }
    return false;
  }

  /* thread from main() for dispatch loop */
  virtual void run( void ) {
    this->dispatchLoop();
  }

  /* dispatch queue events */
  void dispatchLoop( void ) {
    while ( ! this->quit ) {
      try {
        this->queue->TimedDispatch( 100 );
      } catch ( RaiException e ) {
        this->api->PrintLog( LERROR, e, "In dispatchLoop" );
      }
    }
  }

  /* close everything */
  void close( void ) {
    this->quit = true;
    for ( SubjList *sl = this->head; sl != NULL; sl = sl->next )
      sl->timer->Stop();
    if ( this->isub != NULL )
      this->isub->Cancel();
    if ( this->queue != NULL )
      this->queue->Destroy();
    if ( this->session != NULL )
      this->session->Destroy();
    if ( this->api != NULL )
      this->api->Close();
  }
};
} // namespace

static IFTest * iftest = NULL; /* for signal handlers to set quit = true */

#if ! defined( _WIN32 ) && ! defined( _WIN64 )
extern "C" void myInterruptHandler( int );
#else
extern "C" BOOL myCtrlHandler( DWORD );
#endif

int
main( int argc, char *argv[] )
{
#if ! defined( _WIN32 ) && ! defined( _WIN64 )
  struct sigaction nsa;

  /* do this before threads are created so that they inherit the sigs */
  ::memset( &nsa, 0, sizeof( nsa ) );
  ::sigemptyset( &nsa.sa_mask );
  nsa.sa_handler = ::myInterruptHandler;
  ::sigaction( SIGHUP, &nsa, NULL );
  ::sigaction( SIGINT, &nsa, NULL );
  ::sigaction( SIGTERM, &nsa, NULL );
#else
  ::SetConsoleCtrlHandler( (PHANDLER_ROUTINE) ::myCtrlHandler, TRUE );
#endif
  RaiApi    * api = NULL;
  Args   args;
  IFTestArgs iargs;

  /* initialize rai::Time, rai::Sys::in, rai::Sys::out, rai::Sys::err,
   * and calls rai::Hash32::selftest() */
  Sys::initialize();
  /* open log to stderr in case command line fails to parse, it may open
   * again if -log is specified on command line */
  Log::openLog( "-", Log::LVL_MINOR, 4 );

  try {
    /* Open the api type from the command line, looks for -api <name> in
     * argc/argv[] and loads that middleware.  Program could also pass "tibrv"
     * or some other api name in the first argument.  If neither are specfied
     * then the default api is loaded (capr) */
    api = RaiApi::RaiOpen( NULL, argc, argv );
    /* get the api's configuration arguments */
    api->GetArgs( args );
    /* get the subject and settings for the program */
    iargs.getArgs( args );
    /* get the logging, version, help, rc arguments and sets error output */
    args.addDefaults( api->RaiVersion(), "rai_", Sys::err, argv[ 0 ] );

    try {
      if ( args.processArgs( argc, argv ) ) {
        iftest = NEW IFTest();
        /* create api elements and start the dispatch thread */
        if ( iftest->init( api, args ) ) {
          iftest->start(); /* start thread dispatching queue */
          /* either done or waiting for signal */
          iftest->join(); /* join mainloop dispatch thread */
        }
        /* stop all the subscribes, if any, close the api */
        iftest->close();
        delete iftest;
      }
    } catch ( RaiException e ) {
      logError( LERROR, e, "Main" );
    }
  } catch ( RaiException e ) {
    logError( LERROR, e, "Unable to load Rai API" );
  }

  Log::closeLog();
  Sys::terminate();

  return 0;
}

void
myInterruptHandler( int sig ) {
  if ( sig == SIGHUP )
    return;
  Sys::err->printf( "Caught signal %d event, shutting down\n", sig );
  if ( iftest == NULL )
    exit( 1 );
  iftest->quit = true;
}
