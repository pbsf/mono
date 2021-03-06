/**
 * \file
 */

#ifndef __MONO_ERROR_INTERNALS_H__
#define __MONO_ERROR_INTERNALS_H__

#include "mono/utils/mono-compiler.h"
#include "mono/metadata/class-internals.h"

/*Keep in sync with MonoError*/
typedef struct {
	unsigned short error_code;
    unsigned short flags;

	/*These name are suggestions of their content. MonoError internals might use them for something else.*/
	const char *type_name;
	const char *assembly_name;
	const char *member_name;
	const char *exception_name_space;
	const char *exception_name;
	union {
		/* Valid if error_code != MONO_ERROR_EXCEPTION_INSTANCE.
		 * Used by type or field load errors and generic error specified by class.
		 */
		MonoClass *klass;
		/* Valid if error_code == MONO_ERROR_EXCEPTION_INSTANCE.
		 * Generic error specified by a managed instance.
		 */
		uint32_t instance_handle;
	} exn;
	const char *full_message;
	const char *full_message_with_fields;
	const char *first_argument;
	const char *member_signature;

	void *padding [2];
} MonoErrorInternal;

/* Invariant: the error strings are allocated in the mempool of the given image */
struct _MonoErrorBoxed {
	MonoError error;
	MonoImage *image;
};

/*
Historically MonoError initialization was deferred, but always had to occur,
	even in success paths, as cleanup could be done unconditionally.
	This was confusing.

ERROR_DECL (error)
	This is the overwhelmingly common case.
	Declare and initialize a local variable, named "error",
	pointing to an initialized MonoError (named "error_value",
	using token pasting).

ERROR_DECL_VALUE (foo)
	Declare and initialize a local variable, named "foo";
	no pointer is produced for it.

MONO_API_ERROR_INIT
	This is used for MonoError in/out parameter on a public interface,
	which must be presumed uninitialized. These are often
	marked with MONO_API, MONO_RT_EXTERNAL_ONLY, MONO_PROFILER_API, etc.
	Tnis includes functions called from dis, profiler, pedump, and driver.
	dis, profiler, and pedump make sense, these are actually external and
	uninitialized. Driver less so.

error_init
	Initialize a MonoError. These are historical and usually
	but not always redundant, and should be reduced/eliminated.
	All the non-redundant ones should be renamed and all the redundant
	ones removed.

error_init_reuse
	This indicates an error has been cleaned up and will be reused.
	Consider also changing mono_error_cleanup to call error_init_internal,
	and then remove these.

error_init_internal
	Rare cases without a better name.
	For example, setting up an icall frame, or initializing member data.

new0, calloc, static
	A zeroed MonoError is valid and initialized.
	Zeroing an entire MonoError is overkill, unless it is near other
	bulk zeroing.

All initialization is actually bottlenecked to error_init_internal.
Different names indicate different scenarios, but the same code.
*/
#define ERROR_DECL_VALUE(x) 		MonoError x; error_init_internal (&x)
#define ERROR_DECL(x) 			ERROR_DECL_VALUE (x##_value); MonoError * const x = &x##_value
#define error_init_internal(error) 	((void)((error)->init = 0))
#define MONO_API_ERROR_INIT(error) 	error_init_internal (error)
#define error_init_reuse(error) 	error_init_internal (error)

// Historical deferred initialization was called error_init.

// possible bug detection that did not work
//#define error_init(error) (is_ok (error))

// FIXME Eventually all error_init should be removed, however it is prudent
// to leave them in for now, at least most of them, while we sort out
// the few that are needed and to experiment with adding them back in bulk,
// i.e. in an entire source file. Some are obviously not needed.
//#define error_init(error) // nothing
#define error_init(error) error_init_internal (error)
// Function for experimentation, should go away.
//void error_init(MonoError*);

#define is_ok(error) ((error)->error_code == MONO_ERROR_NONE)

#define return_if_nok(error) do { if (!is_ok ((error))) return; } while (0)
#define return_val_if_nok(error,val) do { if (!is_ok ((error))) return (val); } while (0)

#define goto_if_nok(error,label) do { if (!is_ok ((error))) goto label; } while (0)

/* Only use this in icalls */
#define return_val_and_set_pending_if_nok(error, value) \
do { 							\
	if (mono_error_set_pending_exception ((error)))	\
		return (value); 			\
} while (0)						\

void
mono_error_assert_ok_pos (MonoError *error, const char* filename, int lineno) MONO_LLVM_INTERNAL;

#define mono_error_assert_ok(e) mono_error_assert_ok_pos (e, __FILE__, __LINE__)

void
mono_error_dup_strings (MonoError *error, gboolean dup_strings);

/* This function is not very useful as you can't provide any details beyond the message.*/
void
mono_error_set_error (MonoError *error, int error_code, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_assembly_load (MonoError *error, const char *assembly_name, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_assembly_load_simple (MonoError *error, const char *assembly_name, gboolean refection_only);

void
mono_error_set_type_load_class (MonoError *error, MonoClass *klass, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_vset_type_load_class (MonoError *error, MonoClass *klass, const char *msg_format, va_list args);

void
mono_error_set_type_load_name (MonoError *error, const char *type_name, const char *assembly_name, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(4,5);

void
mono_error_set_method_load (MonoError *oerror, MonoClass *klass, const char *method_name, const char *signature, const char *msg_format, ...);

void
mono_error_set_field_load (MonoError *error, MonoClass *klass, const char *field_name, const char *msg_format, ...)  MONO_ATTR_FORMAT_PRINTF(4,5);

void
mono_error_set_bad_image (MonoError *error, MonoImage *image, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_bad_image_name (MonoError *error, const char *file_name, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_out_of_memory (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_argument (MonoError *error, const char *argument, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_argument_null (MonoError *oerror, const char *argument, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_not_verifiable (MonoError *oerror, MonoMethod *method, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(3,4);

void
mono_error_set_generic_error (MonoError *error, const char * name_space, const char *name, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(4,5);

void
mono_error_set_execution_engine (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_not_implemented (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_not_supported (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_invalid_operation (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_file_not_found (MonoError *error, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_exception_instance (MonoError *error, MonoException *exc);

void
mono_error_set_invalid_program (MonoError *oerror, const char *msg_format, ...) MONO_ATTR_FORMAT_PRINTF(2,3);

void
mono_error_set_invalid_cast (MonoError *oerror);

MonoException*
mono_error_prepare_exception (MonoError *error, MonoError *error_out);

MonoException*
mono_error_convert_to_exception (MonoError *error);

void
mono_error_move (MonoError *dest, MonoError *src);

MonoErrorBoxed*
mono_error_box (const MonoError *error, MonoImage *image);

gboolean
mono_error_set_from_boxed (MonoError *error, const MonoErrorBoxed *from);

const char*
mono_error_get_exception_name (MonoError *oerror);

#endif
