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
#ifndef __FD32_NLS_H
#define __FD32_NLS_H

/**
 * \file
 * \brief NLS Manager facilities.
 *
 * Include this header file to use the facilities of the NLS Manager.
 */

#if 0 /* Standard headers */
 //#define NDEBUG /* Define NDEBUG to disable assert */
 #include <assert.h>
 #include <stdarg.h>
 #include <stddef.h> /* Assuming that wchar_t is 32-bit */
 #include <stdint.h>
 #include <string.h>
 #include <errno.h>
#else /* FreeDOS-32 headers */
// #include <dr-env.h>
#include <lfn.h>
#endif

#ifdef __GNUC__
 #define restrict __restrict__
#endif



/// The maximum number of bytes per multibyte character in any code page.
#define NLS_MB_MAX_LEN 6 /* UTF-8 (the longest one) can be up to 6 bytes */


/** \fn int request(REQ_GET_OPERATIONS, int type, void **operations)
 * \brief Gets a structure of operations.
 * \param type the requested type of operations.
 * \param[out] the address of a pointer to the returned operations;
 *             it can be NULL to just check if the requested type is available.
 * \return On success, the updated reference count for the code page (incremented
 *         if 'methods' is not NULL). -EINVAL on failure (request or methods unavailable).
 */

#define REQ_GET_OPERATIONS 0
/** \fn int request(REQ_GET_REFERENCES)
 * \brief Gets the reference cound of the object.
 * \return if supported, the current non-negative reference count.
 */
#define REQ_GET_REFERENCES 1


/** \brief Code page structure.
 *
 * Code page modules register a structure of this type to the NLS manager.
 * Each code page may have several of them registered in the linked list
 * of code pages, for name aliasing, but there must be only one instance
 * for each code page. Thus, a "this"-like parameter is not needed.
 */
struct nls_code_page
{
	/** \brief A name to identify the code page.
	 *
	 * This string is encoded in Unicode UTF-8. Name comparison is performed
	 * disregarding case for characters in ASCII range (that is, 'a'..'z'), so that the
	 * simple implementation of \c strcasecmp can be used, without knowledge of the UTF-8 encoding.
	 */
	const char *name;
	/** \brief The address of the request function for the code page.
	 *
	 * Code page modules are required to support the \c REQ_GET_OPERATIONS and
	 * \c REQ_GET_REFERENCES requests.
	 */
	int (*request)(int function, ...);
	/** \brief Reserved for the NLS Manager.
	 *
	 * This is a pointer to the next code page structure in the linked list owned
	 * by the NLS manager. It must be initialized to a null pointer when registering
	 * a new code page, and ignored as reserved afterwards.
	 */
	struct nls_code_page *next;
};


/*
 * struct nls_operations
 * NLS users get a pointer to a struct nls_operations in order to use the
 * features of a code page. A version numbering is used in order to support
 * expandibility and compatibility (when this stuff will be stable).
 * To get methods for a code page, NLS users call for nls_get_methods, which
 * in turn performs an NLS_REQ_GET_METHODS request to the code page module.
 */
#define OT_NLS_OPERATIONS 3
/** \brief Structure of operations for NLS support.
 *
 * NLS users get a pointer to a structure of this type in order
 * to use the facilities of a code page.
 */
struct nls_operations
{
	/** \brief Multibyte to wide character.
	 *  \param result where to store the converted wide character;
	 *  \param string buffer containing the character to convert (that may be multibyte);
	 *  \param size max number of bytes of \c string to examine;
	 *  \retval >0 the length in bytes of the processed source character, the wide character is stored in \c result;
	 *  \retval -EILSEQ invalid byte sequence in the source character;
	 *  \retval -ENAMETOOLONG \c size too small to parse the source character.
	 */
	int (*mbtowc) (wchar_t *restrict result, const char *restrict string, size_t size);
	/** \brief Wide character to multibyte.
	 *  \param s where to store the converted character (that may be multibyte);
	 *  \param wc the wide character to convert;
	 *  \param size max number of bytes to store in \c s;
	 *  \retval >0 the length in bytes of the converted character, stored in \c s;
	 *  \retval -EINVAL invalid wide character (don't know how to convert it);
	 *  \retval -ENAMETOOLONG \c size too small to store the converted character.
	 */
	int (*wctomb) (char *s, wchar_t wc, size_t size);
	/** \brief Gets the length of a multibyte character.
	 *  \param string buffer containing the character (that may be multibyte);
	 *  \param size max number of bytes of \c string to examine;
	 *  \retval >0 the length in bytes of the multibyte character;
	 *  \retval -EILSEQ invalid byte sequence in the character;
	 *  \retval -ENAMETOOLONG \c size too small to parse the character.
	 *  \remarks You can use this function to know whether a character is single byte or not.
	 *           For example, you must be sure a character is single byte before calling the
	 *           #toupper and #tolower operations.
	 */
	int (*mblen)  (const char *string, size_t size);
	/** \brief Converts a single byte character to upper case.
	 *
	 * If the single byte character \c ch, interpreted as an \c unsigned \c char, is a lower
	 * case letter, this function converts to its corresponding upper case letter. If \c ch
	 * is a byte that is part of a multibyte character, the result is undefined. You can use
	 * the #mblen operation to know whether a character is single byte or not.
	 * \return A byte corresponding to the upper case version of \c ch, if available,
	 * otherwise \c ch is returned unchanged.
	 */
	int (*toupper)(int ch);
	/** \brief Converts a single byte character to lower case.
	 *
	 * This is identical to the #toupper operation, except it converts \c ch to lower case.
	 */
	int (*tolower)(int ch);
	/** \brief Releases the code page structure releasing its reference count.
	 *  \retval >=0 the updated reference count;
	 *  \retval -EINVAL the reference count was already zero prior this call.
	 *
	 *  NLS users shall call this operation when they no longer need to use the facilities
	 *  of the code page. This decreases the internal reference count of the code page.
	 *  When the reference count is zero, nobody is using the code page, thus the code
	 *  page may be unregistered and the code page module may be unloaded.
	 */
	int (*release)(void);
};


/// Gets NLS operations for a code page.
int nls_get(const char *name, int type, void **operations);

/// Registers a code page to the NLS manager liked list.
int nls_register(struct nls_code_page *cp);

/// Unregisters a code page from the NLS manager linked list.
int nls_unregister(struct nls_code_page *cp);

int default_mbtowc(wchar_t *restrict result, const char *restrict string, size_t size);

#endif /* #ifndef __FD32_NLS_H */
