#ifndef __FS_UNICODE_H__
#define __FS_UNICODE_H__

extern void _FAT_utf8_to_unicode16( const char* src, u16* dest );

extern void _FAT_unicode16_to_utf8( const u16* src, char* dest);

extern u32 _unistrnlen( const u16* unistr, u32 maxlen );

extern int _unistrncmp( const u16 * src, const u16 * dest, u32 maxlen );

extern const u16 * _unistrchr( const u16 * str, u16 unichar );

int _uniisalnum( u8 ch );

#endif	//__FS_UNICODE_H__
