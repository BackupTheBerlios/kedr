/*********************************************************************
 * Module: kedr_payload
 *********************************************************************/

/* ========================================================================
 * Copyright (C) 2010, Institute for System Programming 
 *                     of the Russian Academy of Sciences (ISPRAS)
 * Authors: 
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 ======================================================================== */
 
#include <linux/module.h>

MODULE_AUTHOR("Tsyvarev");
MODULE_LICENSE("GPL");

#include <kedr/base/common.h>

#include <linux/cdev.h>
#include <linux/slab.h>	    /* kmalloc(), kfree() */

/*********************************************************************/

/* To minimize the unexpected consequences of trace event-related 
 * headers and symbols, place #include directives for system headers 
 * before '#define CREATE_TRACE_POINTS' directive
 */
#define CREATE_TRACE_POINTS
#include "trace_payload.h" /* trace event facilities */

void repl_kfree(void* p)
{
    trace_kedr_payload(2);
    kfree(p);
}
static void* orig_addrs[] = {(void*)kfree};
static void* repl_addrs[] = {(void*)repl_kfree};

static struct kedr_payload payload = {
	.mod 			        = THIS_MODULE,
	.repl_table.orig_addrs 	= &orig_addrs[0],
	.repl_table.repl_addrs 	= &repl_addrs[0],
	.repl_table.num_addrs	= sizeof(orig_addrs) / sizeof(orig_addrs[0]),
    .target_load_callback   = NULL,
    .target_unload_callback = NULL
};

/* ================================================================ */
static void
kedr_payload_cleanup_module(void)
{
    kedr_payload_unregister(&payload);
	return;
}

static int __init
kedr_payload_init_module(void)
{
    return kedr_payload_register(&payload);
}

module_init(kedr_payload_init_module);
module_exit(kedr_payload_cleanup_module);
/* ================================================================ */


