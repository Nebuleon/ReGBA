/* The FreeDOS-32 Native Language Support (NLS) version 2.0
 * Copyright (C) 2004-2005  Salvatore ISAJA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ACKNLOWLEDGEMENTS
 * This file has been developed from scratch, but using the Linux 2.6.8
 * fs/nls/nls_base.c (which has no copyright nor lincensing notices) as
 * inspiration, as well as the GNU libc documentation.
 */
#include <errno.h>
#include "nls.h"
 
/* Head of the NLS manager linked list of code pages */
static struct nls_code_page *nls_head = NULL;

#define assert(x) {                                                   \
  if (! (x))                                                          \
    printf("%s:%d: assertion failed: %s", __FILE__, __LINE__, #x);   \
} 

/** \param name name of the requested code page, case insensitive in ASCII range ('a'..'z');
 *  \param type the requested type of operations;
 *  \param[out] operations the address of a pointer to the returned operations;
 *              it can be NULL to just check if the requested type is available.
 *  \retval 0 success, \c operations points to the structure of operations;
 *  \retval -ENOTSUP request or type of operations not available.
 */
int nls_get(const char *name, int type, void **operations)
{
	struct nls_code_page *cp;
	if (name)
		for (cp = nls_head; cp; cp = cp->next)
		{
			assert(cp->name);
			if (strcasecmp(name, cp->name) == 0)
			{
				assert(cp->request);
				return cp->request(REQ_GET_OPERATIONS, type, operations);
			}
		}
	return -EINVAL;
}


/** \param cp the code page structure to register.
 *  \retval 0 success;
 *  \retval -EINVAL the code page structure is not valid;
 *  \retval -EBUSY  the code page structure is already linked (the \c next field is not null).
 *  \remarks The NLS Manager updates the \c next field of the code page structure.
 */
int nls_register(struct nls_code_page *cp)
{
	struct nls_code_page *i;
	if (!cp) return -EINVAL;
	if (!cp->name || !cp->request) return -EINVAL;
	if (cp->next) return -EBUSY;
	for (i = nls_head; i; i = i->next)
		if (i == cp) return -EBUSY;
	cp->next = nls_head;
	nls_head = cp;
	return 0;
}


/** \param cp the code page to unregister.
 *  \retval 0 success;
 *  \retval -EINVAL code page structure not found in the linked list;
 *  \retval -EBUSY  the reference count of the code page structure is
 *                  not zero (somebody is using the code page).
 *  \remarks The NLS Manager sets the \c next field of the code page
 *           structure back to a null pointer.
 */
int nls_unregister(struct nls_code_page *cp)
{
	struct nls_code_page *curr, *prev = NULL;
	for (curr = nls_head; curr; prev = curr, curr = curr->next)
		if (curr == cp)
		{
			assert(cp->request);
			if (cp->request(REQ_GET_REFERENCES) > 0)
				return -EBUSY;
			if (!prev) nls_head   = curr->next;
			      else prev->next = curr->next;
			cp->next = NULL;
			return 0;
		}
	return -EINVAL;
}


/****************************************************************************** 
 * A default code page, which maps a single char to a single wchar_t without
 * conversion. It is actually ISO-8859-1 (Latin1), the first page of Unicode.
 ******************************************************************************/


/* The reference count of this code page. The code page can be unregistered
 * only if the reference count is zero (nobody is using the code page).
 */
static unsigned default_refcount = 0;


//static int default_mbtowc(wchar_t *restrict result, const char *restrict string, size_t size)
int default_mbtowc(wchar_t *restrict result, const char *restrict string, size_t size)
{
	if (!result || !string) return -EFAULT;
	if (size < 1) return -ENAMETOOLONG;
	*result = (wchar_t) (unsigned char) *string;
	return 1;
}


int default_wctomb(char *s, wchar_t wc, size_t size)
{
	if (!s) return -EFAULT;
	if ((wc < 0) || (wc > 255)) return -EINVAL;
	if (size < 1) return -ENAMETOOLONG;
	*s = (char) wc;
	return 1;
}


int default_mblen(const char *string, size_t size)
{
	if (!string) return -EFAULT;
	if (size < 1) return -ENAMETOOLONG;
	return 1;
}


int default_toupper(int ch)
{
	unsigned char c = (unsigned char) ch;
	if (((c >= 'a') && (c <= 'z'))
	 || ((c >= 0xE8) && (c <= 0xFE) && (c != 0xF7))) c -= 0x20;
	return (int) c;
}


static int default_tolower(int ch)
{
	unsigned char c = (unsigned char) ch;
	if (((c >=  'A') && (c <=  'Z'))
	 || ((c >= 0xC0) && (c <= 0xDE) && (c != 0xD7))) c += 0x20;
	return (int) c;
}


static int default_release(void)
{
	int res = -EINVAL;
	if (default_refcount > 0) res = --default_refcount;
	return res;
}


static struct nls_operations default_nls_operations =
{
//	.mbtowc  = default_mbtowc,
//	.wctomb  = default_wctomb,
//	.mblen   = default_mblen,
///	.toupper = default_toupper,
//	.tolower = default_tolower,
//	.release = default_release
};

#if 0
int default_request(int function, ...)
{
	va_list ap;
	va_start(ap, function);
	switch (function)
	{
		case REQ_GET_OPERATIONS:
		{
			int type = va_arg(ap, int);
			void **operations = va_arg(ap, void **);
			if (type != OT_NLS_OPERATIONS) return -ENOTSUP;
			if (operations)
			{
				*operations = (void *) &default_nls_operations;
				default_refcount++;
			}
			return 0;
		}
		case REQ_GET_REFERENCES:
			return default_refcount;
	}
	va_end(ap);
	return -ENOTSUP;
}


static struct nls_code_page default_cp =
{
	.name    = "default",
	.request = default_request,
	.next    = NULL
};


static struct nls_code_page iso8859_1_cp =
{
	.name    = "iso8859-1",
	.request = default_request,
	.next    = NULL
};


/****************************************************************************** 
 * NLS Manager initialization.
 ******************************************************************************/


#include <kernel.h>
#include <ll/i386/error.h>
static const struct { /*const*/ char *name; DWORD address; } symbols[] =
{
	{ "nls_get",        (DWORD) nls_get        },
	{ "nls_register",   (DWORD) nls_register   },
	{ "nls_unregister", (DWORD) nls_unregister },
	{ 0, 0 }
};


void nls_init(void)
{
	int k;
	message("Going to install the NLS Manager... ");
	for (k = 0; symbols[k].name; k++)
		if (add_call(symbols[k].name, symbols[k].address, ADD) == -1)
			message("Cannot add %s to the symbol table\n", symbols[k].name);
	k = nls_register(&default_cp);
	if (k < 0) message("Error %i registering the default code page\n", k);
	k = nls_register(&iso8859_1_cp);
	if (k < 0) message("Error %i registering the ISO-8859-1 code page\n", k);
	message("Done\n");
}
#endif
