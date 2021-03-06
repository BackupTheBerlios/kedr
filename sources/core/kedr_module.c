/*
 * Combine different KEDR components into one module.
 */

/* ========================================================================
 * Copyright (C) 2012, KEDR development team
 * Copyright (C) 2010-2012, Institute for System Programming 
 *                          of the Russian Academy of Sciences (ISPRAS)
 * Authors: 
 *      Eugene A. Shatokhin <spectre@ispras.ru>
 *      Andrey V. Tsyvarev  <tsyvarev@ispras.ru>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 ======================================================================== */

#include <kedr/core/kedr.h>
#include <kedr/core/kedr_functions_support.h>

#include "kedr_base_internal.h"
#include "kedr_instrumentor_internal.h"
#include "kedr_functions_support_internal.h"
#include "kedr_target_detector_internal.h"

#include <linux/version.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/slab.h> /* kmalloc */
#include <linux/string.h> /* kstrdup, strlen, strcpy */

#include <linux/mutex.h>

#include "config.h"

MODULE_AUTHOR("Tsyvarev Andrey");
MODULE_LICENSE("GPL");

/* 
 * Name of the module to analyze. It can be passed to 'insmod' as 
 * an argument, for example,
 *  insmod kedr.ko target_name="module_to_be_analyzed".
 * 
 * This module parameter is implemented by hand
 * (with 'set' and 'get' callbacks).
 */

#define target_name_not_set "none"

/*
 * Initial value of target_name parameter is set before (!)
 * initialization of KEDR.
 * 
 * So, we cannot call kedr_target_detector_set_target_name()
 * at that stage.
 * 
 * Similarly, reading of 'target_name' parameter may occur before
 * initialization of the module, and we cannot call
 * kedr_target_detector_get_target_name() at this stage.
 * 
 * Instead, we use a temporary buffer for reading and writting
 * in that case.
 */

static int is_target_detector_initialized = 0;

static char target_name_buffer[MODULE_NAME_LEN] = target_name_not_set;
/* 
 * Protect target_name_buffer and is_target_detector_initialized
 * from concurrent access.
 */
static DEFINE_MUTEX(target_name_mutex);

static int
target_name_param_get(char* buffer,
#if defined(MODULE_PARAM_CREATE_USE_OPS_STRUCT)
	const struct kernel_param *kp
#elif defined(MODULE_PARAM_CREATE_USE_OPS)
	struct kernel_param *kp
#else 
#error Unknown way to create module parameter with callbacks
#endif
)
{
	int result = 0;
	
	mutex_lock(&target_name_mutex);
	
	if(is_target_detector_initialized)
	{
		char* target_name = kedr_target_detector_get_target_name();

		if(IS_ERR(target_name))
		{
			result = PTR_ERR(target_name);
		}
		else if(target_name != NULL)
		{
			strcpy(buffer, target_name);
			kfree(target_name);
		}
		else
		{
			strcpy(buffer, target_name_not_set);
		}
	}
	else
	{
		strcpy(buffer, target_name_buffer);
	}
	
	mutex_unlock(&target_name_mutex);
	
	if(!result)
	{
		int size = strlen(buffer);
		/* append '\n' symbol to the buffer for pretty output */
		buffer[size] = '\n';
		return size;
	}
	else
		return result;
}

/* Replace '-' with '_' in the name of the target to allow the user to 
 * specify the target names like "kvm-intel" or the like. */
static void
replace_dashes(char *str)
{
	int i = 0;
	while (str[i] != 0) {
		if (str[i] == '-')
			str[i] = '_';
		++i;
	}
}

static int
target_name_param_set(const char* val,
#if defined(MODULE_PARAM_CREATE_USE_OPS_STRUCT)
	const struct kernel_param *kp
#elif defined(MODULE_PARAM_CREATE_USE_OPS)
	struct kernel_param *kp
#else 
#error Unknown way to create module parameter with callbacks
#endif
)
{
	int result;
	char target_name[MODULE_NAME_LEN];
	
	size_t size = strlen(val) + 1;
	if (size == 1)
		return -EINVAL;
	
	if(val[size - 2] == '\n')
	{
		/* The last symbol in the value to be set is '\n'.
		 * 
		 * Usually, it is result of using 'echo' shell command,
		 * so ignore this symbol. */
		size--;
	}
	if(size > sizeof(target_name))
		size = sizeof(target_name);

	strlcpy(target_name, val, size);
	replace_dashes(target_name);
	
	mutex_lock(&target_name_mutex);
	
	if (!is_target_detector_initialized) {
		strncpy(target_name_buffer, target_name, 
			sizeof(target_name_buffer));
		
		result = 0;
		goto out;
	}
	
	if (strcmp(target_name, target_name_not_set) != 0)
	{
		result = kedr_target_detector_set_target_name(target_name);
	}
	else
	{
		result = kedr_target_detector_clear_target_name();
	}

out:	
	mutex_unlock(&target_name_mutex);
	return result;
}

#if defined(MODULE_PARAM_CREATE_USE_OPS_STRUCT)
static struct kernel_param_ops target_name_param_ops =
{
	.set = target_name_param_set,
	.get = target_name_param_get,
};
module_param_cb(target_name,
	target_name_param_ops,
	NULL,
	S_IRUGO | S_IWUSR);
#elif defined(MODULE_PARAM_CREATE_USE_OPS)
module_param_call(target_name,
	target_name_param_set, target_name_param_get,
	NULL,
	S_IRUGO | S_IWUSR);
#else 
#error Unknown way to create module parameter with callbacks
#endif

static int
on_target_load(struct kedr_target_module_notifier* notifier,
	struct module* m)
{
	int result;
	
	const struct kedr_instrumentor_replace_pair* replace_pairs;
	const struct kedr_base_interception_info* interception_info =
		kedr_base_target_load_callback(m);
	
	if(IS_ERR(interception_info))
	{
		return PTR_ERR(interception_info);
	}
	
	replace_pairs = kedr_functions_support_prepare(interception_info);
	
	if(IS_ERR(replace_pairs))
	{
		kedr_base_target_unload_callback(m);
		return PTR_ERR(replace_pairs);
	}
	
	result = kedr_instrumentor_replace_functions(m, replace_pairs);
	if(result)
	{
		kedr_functions_support_release();
		kedr_base_target_unload_callback(m);
		return result;
	}
	
	return 0;
}

static void
on_target_unload(struct kedr_target_module_notifier* notifier,
		struct module* m)
{
	kedr_instrumentor_replace_clean(m);
	kedr_functions_support_release();
	kedr_base_target_unload_callback(m);
}

static struct kedr_target_module_notifier notifier =
{
	.mod = THIS_MODULE,

	.on_target_load = on_target_load,
	.on_target_unload = on_target_unload
};

static int function_use(struct kedr_base_operations* ops, void* function)
{
	return kedr_functions_support_function_use(function);
}

static int function_unuse(struct kedr_base_operations* ops, void* function)
{
	return kedr_functions_support_function_unuse(function);
}

static struct kedr_base_operations base_operations =
{
	.function_use = function_use,
	.function_unuse = function_unuse,
};

static int __init
kedr_module_init(void)
{
	int result;

	result = kedr_functions_support_init();
	if (result) goto functions_support_err;
	
	result = kedr_instrumentor_init();
	if (result) goto instrumentor_err;
	
	result = kedr_base_init(&base_operations);
	if (result) goto base_err;
	
	result = kedr_target_detector_init(&notifier);
	if (result) goto target_detector_err;
	
	mutex_lock(&target_name_mutex);
	replace_dashes(target_name_buffer);
	result = strcmp(target_name_buffer, target_name_not_set)
		? kedr_target_detector_set_target_name(target_name_buffer)
		: kedr_target_detector_clear_target_name();
	if(!result) is_target_detector_initialized = 1;
	
	mutex_unlock(&target_name_mutex);
	
	if(result) goto set_target_name_err;

	return 0;

set_target_name_err:
	kedr_target_detector_destroy();
target_detector_err:
	kedr_base_destroy();
base_err:
	kedr_instrumentor_destroy();
instrumentor_err:
	kedr_functions_support_destroy();
functions_support_err:
	return result;
}

static void __exit
kedr_module_exit(void)
{
	mutex_lock(&target_name_mutex);
	is_target_detector_initialized = 0;
	/*
	 * Do not synchronize 'target_name_buffer'.
	 * 
	 * It cause only old value to be returned
	 * in case of read 'target_name' after this moment.
	 */
	mutex_unlock(&target_name_mutex);
	
	kedr_target_detector_destroy();
	kedr_base_destroy();
	kedr_instrumentor_destroy();
	kedr_functions_support_destroy();
}

module_init(kedr_module_init);
module_exit(kedr_module_exit);

// The only functions exported from the KEDR module.
EXPORT_SYMBOL(kedr_payload_register);
EXPORT_SYMBOL(kedr_payload_unregister);

EXPORT_SYMBOL(kedr_functions_support_register);
EXPORT_SYMBOL(kedr_functions_support_unregister);

EXPORT_SYMBOL(kedr_target_module_in_init);
