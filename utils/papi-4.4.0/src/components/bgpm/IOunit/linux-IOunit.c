/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/** 
 * @file    linux-IOunit.c
 * CVS:     $Id$
 * @author  Heike Jagode
 *          jagode@eecs.utk.edu
 * Mods:	<your name here>
 *			<your email address>
 * BGPM / IOunit component 
 * 
 * Tested version of bgpm (early access)
 *
 * @brief
 *  This file has the source code for a component that enables PAPI-C to 
 *  access hardware monitoring counters for BG/Q through the bgpm library.
 */


#include "linux-IOunit.h"

/* Declare our vector in advance */
papi_vector_t _IOunit_vector;

/*****************************************************************************
 *******************  BEGIN PAPI's COMPONENT REQUIRED FUNCTIONS  *************
 *****************************************************************************/

/*
 * This is called whenever a thread is initialized
 */
int
IOUNIT_init( hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_init\n" );
#endif
	
	( void ) ctx;
	return PAPI_OK;
}


/* Initialize hardware counters, setup the function vector table
 * and get hardware information, this routine is called when the 
 * PAPI process is initialized (IE PAPI_library_init)
 */
int
IOUNIT_init_substrate( int cidx )
{  
#ifdef DEBUG_BGQ
	printf( "IOUNIT_init_substrate\n" );
#endif

	_IOunit_vector.cmp_info.CmpIdx = cidx;
#ifdef DEBUG_BGQ
	printf( "IOUNIT_init_substrate cidx = %d\n", cidx );
#endif
	
	return ( PAPI_OK );
}


/*
 * Control of counters (Reading/Writing/Starting/Stopping/Setup)
 * functions
 */
int
IOUNIT_init_control_state( hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_init_control_state\n" );
#endif

	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;
	
	this_state->EventGroup = Bgpm_CreateEventSet();
	CHECK_BGPM_ERROR( this_state->EventGroup, "Bgpm_CreateEventSet" );
	
	return PAPI_OK;
}


/*
 *
 */
int
IOUNIT_start( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_start\n" );
#endif
	( void ) ctx;
	int retval;
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;
	
	retval = Bgpm_ResetStart( this_state->EventGroup );
	CHECK_BGPM_ERROR( retval, "Bgpm_ResetStart" );
	
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_stop( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_stop\n" );
#endif
	( void ) ctx;
	int retval;
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;
	
	retval = Bgpm_Stop( this_state->EventGroup );
	CHECK_BGPM_ERROR( retval, "Bgpm_Stop" );
	
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_read( hwd_context_t * ctx, hwd_control_state_t * ptr,
		   long_long ** events, int flags )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_read\n" );
#endif
	( void ) ctx;
	( void ) flags;
	int i, numEvts;
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;

	numEvts = Bgpm_NumEvents( this_state->EventGroup );
	if ( numEvts == 0 ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is %d for BGPM API function Bgpm_NumEvents.\n", numEvts );
#endif
		//return ( EXIT_FAILURE );
	}
		
	for ( i = 0; i < numEvts; i++ )
		this_state->counts[i] = _common_getEventValue( i, this_state->EventGroup );

	*events = this_state->counts;
	
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_shutdown( hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_shutdown\n" );
#endif
	
	( void ) ctx;
	return ( PAPI_OK );
}


/*
 * user_signal_handler
 *
 * This function is used when hardware overflows are working or when
 * software overflows are forced
 */
void
user_signal_handler_IOUNIT( int hEvtSet, uint64_t address, uint64_t ovfVector, const ucontext_t *pContext )
{
#ifdef DEBUG_BGQ
	printf( "user_signal_handler_IOUNIT\n" );
#endif
	
	int retval, i;
	int isHardware = 1;
	int cidx = _IOunit_vector.cmp_info.CmpIdx;
	long_long overflow_bit = 0;
	caddr_t address1;
	_papi_hwi_context_t ctx;
	ctx.ucontext = ( hwd_ucontext_t * ) pContext;
	ThreadInfo_t *thread = _papi_hwi_lookup_thread( 0 );
	EventSetInfo_t *ESI;
	ESI = thread->running_eventset[cidx];
    // Get the indices of all events which have overflowed.
    unsigned ovfIdxs[BGPM_MAX_OVERFLOW_EVENTS];
    unsigned len = BGPM_MAX_OVERFLOW_EVENTS;
	
    retval = Bgpm_GetOverflowEventIndices( hEvtSet, ovfVector, ovfIdxs, &len );
	if ( retval < 0 ) {
#ifdef DEBUG_BGPM
		printf ( "Error: ret value is %d for BGPM API function Bgpm_GetOverflowEventIndices.\n", 
				retval ); 
#endif
		return;
	}
	
	if ( thread == NULL ) {
		PAPIERROR( "thread == NULL in user_signal_handler!" );
		return;
	}
	
	if ( ESI == NULL ) {
		PAPIERROR( "ESI == NULL in user_signal_handler!");
		return;
	}
	
	if ( ESI->overflow.flags == 0 ) {
		PAPIERROR( "ESI->overflow.flags == 0 in user_signal_handler!");
		return;
	}
	
	for ( i = 0; i < len; i++ ) {
		uint64_t hProf;
        Bgpm_GetEventUser1( hEvtSet, ovfIdxs[i], &hProf );
        if ( hProf ) {
			overflow_bit ^= 1 << ovfIdxs[i];
			break;
        }
		
	}
	
	if ( ESI->overflow.flags & PAPI_OVERFLOW_FORCE_SW ) {
#ifdef DEBUG_BGQ
		printf("OVERFLOW_SOFTWARE\n");
#endif
		address1 = GET_OVERFLOW_ADDRESS( ctx );
		_papi_hwi_dispatch_overflow_signal( ( void * ) &ctx, address1, NULL, 0, 0, &thread, cidx );
		return;
	}
	else if ( ESI->overflow.flags & PAPI_OVERFLOW_HARDWARE ) {
#ifdef DEBUG_BGQ
		printf("OVERFLOW_HARDWARE\n");
#endif
		address1 = GET_OVERFLOW_ADDRESS( ctx );
		_papi_hwi_dispatch_overflow_signal( ( void * ) &ctx, address1, &isHardware, overflow_bit, 0, &thread, cidx );
	}
	else {
#ifdef DEBUG_BGQ
		printf("OVERFLOW_NONE\n");
#endif
		PAPIERROR( "ESI->overflow.flags is set to something other than PAPI_OVERFLOW_HARDWARE or PAPI_OVERFLOW_FORCE_SW (%x)", thread->running_eventset[cidx]->overflow.flags);
	}
}


/*
 * Set Overflow
 *
 * This is commented out in BG/L/P - need to explore and complete...
 * However, with true 64-bit counters in BG/Q and all counters for PAPI
 * always starting from a true zero (we don't allow write...), the possibility
 * for overflow is remote at best...
 */
int
IOUNIT_set_overflow( EventSetInfo_t * ESI, int EventIndex, int threshold )
{
#ifdef DEBUG_BGQ
	printf("BEGIN IOUNIT_set_overflow\n");
#endif
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ESI->ctl_state;
	int retval;
	int evt_idx;
	uint64_t threshold_for_bgpm;
		
	/* convert threadhold value assigned by PAPI user to value that is
	 * programmed into the counter. This value is required by Bgpm_SetOverflow() */ 
	threshold_for_bgpm = BGPM_PERIOD2THRES( threshold );

	evt_idx = ESI->EventInfoArray[EventIndex].pos[0];
	SUBDBG( "Hardware counter %d (vs %d) used in overflow, threshold %d\n",
		   evt_idx, EventIndex, threshold );
#ifdef DEBUG_BGQ
	printf( "Hardware counter %d (vs %d) used in overflow, threshold %d\n",
		   evt_idx, EventIndex, threshold );
#endif
	/* If this counter isn't set to overflow, it's an error */
	if ( threshold == 0 ) {
		/* Remove the signal handler */
		retval = _papi_hwi_stop_signal( _IOunit_vector.cmp_info.hardware_intr_sig );
		if ( retval != PAPI_OK )
			return ( retval );
	}
	else {
#ifdef DEBUG_BGQ
		printf( "IOUNIT_set_overflow: Enable the signal handler\n" );
#endif
		/* Enable the signal handler */
		retval = _papi_hwi_start_signal( _IOunit_vector.cmp_info.hardware_intr_sig, 
										NEED_CONTEXT, 
										_IOunit_vector.cmp_info.CmpIdx );
		if ( retval != PAPI_OK )
			return ( retval );
		
		retval = Bgpm_SetOverflow( this_state->EventGroup,
								   evt_idx,
								   threshold_for_bgpm );
		CHECK_BGPM_ERROR( retval, "Bgpm_SetOverflow" );
		
        retval = Bgpm_SetEventUser1( this_state->EventGroup,
									 evt_idx,
									 1024 );
		CHECK_BGPM_ERROR( retval, "Bgpm_SetEventUser1" );
		
		/* user signal handler for overflow case */
		retval = Bgpm_SetOverflowHandler( this_state->EventGroup, user_signal_handler_IOUNIT );
		CHECK_BGPM_ERROR( retval, "Bgpm_SetOverflowHandler" );
	}
	
	return ( PAPI_OK );
}


/* This function sets various options in the substrate
 * The valid codes being passed in are PAPI_SET_DEFDOM,
 * PAPI_SET_DOMAIN, PAPI_SETDEFGRN, PAPI_SET_GRANUL * and PAPI_SET_INHERIT
 */
int
IOUNIT_ctl( hwd_context_t * ctx, int code, _papi_int_option_t * option )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_ctl\n" );
#endif
	
	( void ) ctx;
	( void ) code;
	( void ) option;
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_update_control_state( hwd_control_state_t * ptr,
						   NativeInfo_t * native, int count,
						   hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_update_control_state: count = %d\n", count );
#endif
	( void ) ctx;
	int retval, index, i;
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;
	
	// Delete and re-create BGPM eventset
	_common_deleteRecreate( &this_state->EventGroup );
		
	// otherwise, add the events to the eventset
	for ( i = 0; i < count; i++ ) {
		index = ( native[i].ni_event & PAPI_NATIVE_AND_MASK & PAPI_COMPONENT_AND_MASK ) + OFFSET;
		
		native[i].ni_position = i;

#ifdef DEBUG_BGQ
		printf("IOUNIT_update_control_state: ADD event: i = %d, index = %d\n", i, index );
#endif
				
		/* Add events to the BGPM eventGroup */
		retval = Bgpm_AddEvent( this_state->EventGroup, index );
		CHECK_BGPM_ERROR( retval, "Bgpm_AddEvent" );
	}
	
	return ( PAPI_OK );
}


/*
 * This function has to set the bits needed to count different domains
 * In particular: PAPI_DOM_USER, PAPI_DOM_KERNEL PAPI_DOM_OTHER
 * By default return PAPI_EINVAL if none of those are specified
 * and PAPI_OK with success
 * PAPI_DOM_USER is only user context is counted
 * PAPI_DOM_KERNEL is only the Kernel/OS context is counted
 * PAPI_DOM_OTHER  is Exception/transient mode (like user TLB misses)
 * PAPI_DOM_ALL   is all of the domains
 */
int
IOUNIT_set_domain( hwd_control_state_t * cntrl, int domain )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_set_domain\n" );
#endif
	int found = 0;
	( void ) cntrl;

	if ( PAPI_DOM_USER & domain )
		found = 1;

	if ( PAPI_DOM_KERNEL & domain )
		found = 1;

	if ( PAPI_DOM_OTHER & domain )
		found = 1;

	if ( !found )
		return ( PAPI_EINVAL );

	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_reset( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_reset\n" );
#endif
	( void ) ctx;
	int retval;
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ptr;

	/* we can't simply call Bgpm_Reset() since PAPI doesn't have the 
	 restriction that an EventSet has to be stopped before resetting is
	 possible. However, BGPM does have this restriction. 
	 Hence we need to stop, reset and start */
	retval = Bgpm_Stop( this_state->EventGroup );
	CHECK_BGPM_ERROR( retval, "Bgpm_Stop" );
	
	retval = Bgpm_ResetStart( this_state->EventGroup );
	CHECK_BGPM_ERROR( retval, "Bgpm_ResetStart" );
	
	return ( PAPI_OK );
}


/*
 * PAPI Cleanup Eventset
 *
 * Destroy and re-create the BGPM / IOunit EventSet
 */
int
IOUNIT_cleanup_eventset( hwd_control_state_t * ctrl )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_cleanup_eventset\n" );
#endif
	
	IOUNIT_control_state_t * this_state = ( IOUNIT_control_state_t * ) ctrl;
		
	// create a new empty bgpm eventset
	// reason: bgpm doesn't permit to remove events from an eventset; 
	// hence we delete the old eventset and create a new one
	_common_deleteRecreate( &this_state->EventGroup ); // HJ try to use delete() only
	
	return ( PAPI_OK );
}


/*
 * Native Event functions
 */
int
IOUNIT_ntv_enum_events( unsigned int *EventCode, int modifier )
{
#ifdef DEBUG_BGQ
	//printf( "IOUNIT_ntv_enum_events\n" );
#endif
	int cidx = PAPI_COMPONENT_INDEX( *EventCode );

	switch ( modifier ) {
	case PAPI_ENUM_FIRST:
		*EventCode = PAPI_NATIVE_MASK | PAPI_COMPONENT_MASK( cidx );

		return ( PAPI_OK );
		break;

	case PAPI_ENUM_EVENTS:
	{
		int index = ( *EventCode & PAPI_NATIVE_AND_MASK & PAPI_COMPONENT_AND_MASK ) + OFFSET;

		if ( index < IOUNIT_MAX_COUNTERS ) {
			*EventCode = *EventCode + 1;
			return ( PAPI_OK );
		} else
			return ( PAPI_ENOEVNT );

		break;
	}
	default:
		return ( PAPI_EINVAL );
	}
	return ( PAPI_EINVAL );
}


/*
 *
 */
int
IOUNIT_ntv_name_to_code( char *name, unsigned int *event_code )
{
#ifdef DEBUG_BGQ
	printf( "IOUNIT_ntv_name_to_code\n" );
#endif
	int ret;
	
	/* Return event id matching a given event label string */
	ret = Bgpm_GetEventIdFromLabel ( name );
	
	if ( ret <= 0 ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is %d for BGPM API function '%s'.\n",
				ret, "Bgpm_GetEventIdFromLabel" );
#endif
		return PAPI_ENOEVNT;
	}
	else if ( ret < OFFSET || ret > IOUNIT_MAX_COUNTERS ) // not an IOUnit event
		return PAPI_ENOEVNT;
	else
		*event_code = ( ret - OFFSET ) | PAPI_NATIVE_MASK | PAPI_COMPONENT_MASK( _IOunit_vector.cmp_info.CmpIdx ) ;
	
	return PAPI_OK;
}


/*
 *
 */
int
IOUNIT_ntv_code_to_name( unsigned int EventCode, char *name, int len )
{
#ifdef DEBUG_BGQ
	//printf( "IOUNIT_ntv_code_to_name\n" );
#endif
	int index;
	
	index = ( EventCode & PAPI_NATIVE_AND_MASK & PAPI_COMPONENT_AND_MASK ) + OFFSET;

	if ( index >= MAX_COUNTERS )
		return PAPI_ENOEVNT;

	strncpy( name, Bgpm_GetEventIdLabel( index ), len );
	
	if ( name == NULL ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is NULL for BGPM API function Bgpm_GetEventIdLabel.\n" );
#endif
		return PAPI_ENOEVNT;
	}
	
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_ntv_code_to_descr( unsigned int EventCode, char *name, int len )
{
#ifdef DEBUG_BGQ
	//printf( "IOUNIT_ntv_code_to_descr\n" );
#endif	
	int retval, index;
	
	index = ( EventCode & PAPI_NATIVE_AND_MASK & PAPI_COMPONENT_AND_MASK ) + OFFSET;
	
	retval = Bgpm_GetLongDesc( index, name, &len );
	CHECK_BGPM_ERROR( retval, "Bgpm_GetLongDesc" );						 
	
	return ( PAPI_OK );
}


/*
 *
 */
int
IOUNIT_ntv_code_to_bits( unsigned int EventCode, hwd_register_t * bits )
{
#ifdef DEBUG_BGQ
	//printf( "IOUNIT_ntv_code_to_bits\n" );
#endif

	return ( PAPI_OK );
}


/*
 *
 */
papi_vector_t _IOunit_vector = {
	.cmp_info = {
				 /* default component information (unspecified values are initialized to 0) */
				 .name = "$Id: linux-IOunit.c,v 1.2 2011/03/18 21:40:22 jagode Exp $",
				 .version = "$Revision: 1.2 $",
				 .CmpIdx = 0, 
				 .num_cntrs = IOUNIT_MAX_COUNTERS,
				 .num_mpx_cntrs = PAPI_MPX_DEF_DEG,
				 .default_domain = PAPI_DOM_USER,
				 .available_domains = PAPI_DOM_USER | PAPI_DOM_KERNEL,
				 .default_granularity = PAPI_GRN_THR,
				 .available_granularities = PAPI_GRN_THR,
		
				 .itimer_sig = PAPI_INT_MPX_SIGNAL,
				 .itimer_num = PAPI_INT_ITIMER,
				 .itimer_res_ns = 1,
				 .hardware_intr_sig = PAPI_INT_SIGNAL,
				 
		
				 .kernel_multiplex = 0,

				 /* component specific cmp_info initializations */
				 .fast_real_timer = 0,
				 .fast_virtual_timer = 0,
				 .itimer_ns = PAPI_INT_MPX_DEF_US * 1000,
				 .attach = 0,
				 .attach_must_ptrace = 0,
				 }
	,

	/* sizes of framework-opaque component-private structures */
	.size = {
			 .context = sizeof ( IOUNIT_context_t ),
			 .control_state = sizeof ( IOUNIT_control_state_t ),
			 .reg_value = sizeof ( IOUNIT_register_t ),
			 .reg_alloc = sizeof ( IOUNIT_reg_alloc_t ),
			 }
	,
	/* function pointers in this component */
	.init = IOUNIT_init,
	.init_substrate = IOUNIT_init_substrate,
	.init_control_state = IOUNIT_init_control_state,
	.start = IOUNIT_start,
	.stop = IOUNIT_stop,
	.read = IOUNIT_read,
	.shutdown = IOUNIT_shutdown,
	.set_overflow = IOUNIT_set_overflow,
	.cleanup_eventset = IOUNIT_cleanup_eventset,
	.ctl = IOUNIT_ctl,
	
	.update_control_state = IOUNIT_update_control_state,
	.set_domain = IOUNIT_set_domain,
	.reset = IOUNIT_reset,

	.ntv_name_to_code = IOUNIT_ntv_name_to_code,
	.ntv_enum_events = IOUNIT_ntv_enum_events,
	.ntv_code_to_name = IOUNIT_ntv_code_to_name,
	.ntv_code_to_descr = IOUNIT_ntv_code_to_descr,
	.ntv_code_to_bits = IOUNIT_ntv_code_to_bits,
	.ntv_bits_to_info = NULL,
};
