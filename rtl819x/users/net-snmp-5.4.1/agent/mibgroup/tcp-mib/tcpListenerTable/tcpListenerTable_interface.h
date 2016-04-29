/*
 * Note: this file originally auto-generated by mib2c using
 *       version : 1.67 $ of : mfd-interface.m2c,v $
 *
 * $Id: tcpListenerTable_interface.h,v 1.1 2008/06/23 10:52:17 michael Exp $
 */
/** @ingroup interface Routines to interface to Net-SNMP
 *
 * \warning This code should not be modified, called directly,
 *          or used to interpret functionality. It is subject to
 *          change at any time.
 * 
 * @{
 */
/*
 * *********************************************************************
 * *********************************************************************
 * *********************************************************************
 * ***                                                               ***
 * ***  NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE NOTE  ***
 * ***                                                               ***
 * ***                                                               ***
 * ***       THIS FILE DOES NOT CONTAIN ANY USER EDITABLE CODE.      ***
 * ***                                                               ***
 * ***                                                               ***
 * ***       THE GENERATED CODE IS INTERNAL IMPLEMENTATION, AND      ***
 * ***                                                               ***
 * ***                                                               ***
 * ***    IS SUBJECT TO CHANGE WITHOUT WARNING IN FUTURE RELEASES.   ***
 * ***                                                               ***
 * ***                                                               ***
 * *********************************************************************
 * *********************************************************************
 * *********************************************************************
 */
#ifndef TCPLISTENERTABLE_INTERFACE_H
#define TCPLISTENERTABLE_INTERFACE_H

#ifdef __cplusplus
extern          "C" {
#endif


#include "tcpListenerTable.h"


    /*
     ********************************************************************
     * Table declarations
     */

    /*
     * PUBLIC interface initialization routine 
     */
    void
        _tcpListenerTable_initialize_interface
        (tcpListenerTable_registration * user_ctx, u_long flags);
    void
        _tcpListenerTable_shutdown_interface(tcpListenerTable_registration
                                             * user_ctx);

    tcpListenerTable_registration *tcpListenerTable_registration_get(void);

        tcpListenerTable_registration
        * tcpListenerTable_registration_set(tcpListenerTable_registration *
                                            newreg);

    netsnmp_container *tcpListenerTable_container_get(void);
    int             tcpListenerTable_container_size(void);

        tcpListenerTable_rowreq_ctx
        * tcpListenerTable_allocate_rowreq_ctx(tcpListenerTable_data *,
                                               void *);
    void
        tcpListenerTable_release_rowreq_ctx(tcpListenerTable_rowreq_ctx *
                                            rowreq_ctx);

    int             tcpListenerTable_index_to_oid(netsnmp_index * oid_idx,
                                                  tcpListenerTable_mib_index
                                                  * mib_idx);
    int             tcpListenerTable_index_from_oid(netsnmp_index *
                                                    oid_idx,
                                                    tcpListenerTable_mib_index
                                                    * mib_idx);

    /*
     * access to certain internals. use with caution!
     */
    void            tcpListenerTable_valid_columns_set(netsnmp_column_info
                                                       *vc);


#ifdef __cplusplus
}
#endif
#endif                          /* TCPLISTENERTABLE_INTERFACE_H */
/**  @} */

