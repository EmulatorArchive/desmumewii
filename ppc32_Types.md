#internal use only.

## Types ##

```
  byte - 8-bit wide.
  halfword - 16-bit wide.
  word - 32-bit wide.
```

```
  r - some 32-bit register.
  offset - some offset from address stored in r.
  mr - move register. 'a = b'. 1 cycle.
  extsb/extsh - sign-extend byte/halfword. 1 cycle.
  rlwinm - Rotate Left Word Immediate then and with Mask. 1 cycle.
```

## type casting ##

### Words ###

As ppc32 hasn't sign-extend word the rules are the same for all types
```
  mr r, r
```
and it would be signed or unsigned as a source.
If you need to get unsigned word from signed value then
```
  u32 _u32 = (u8) _s8; // same with halfwords.
  u32 _u32 = (u32) _s32; // would contain signed 32-bit value.
  s32 _s32 = _u32; // would contain unsigned 32-bit value.
```

### Halfwords ###

from word:
```
to s16:
  extsh r, r
to u16:
  rlwinm r, r, 0, 16, 31
```

from halfword:
```
s16 to s16 and u16 to u16:
  mr r, r
u16 to s16:
  extsh r, r
s16 to u16:
  rlwinm r, r, 0, 16, 31
```

from byte:
```
s8 to u16:
  rlwinm r, r, 0, 16, 31
s8 to s16, u8 to u16 and u8 to s16:
  mr r, r
```

### Bytes ###

from words and halfwords:
```
to s8:
  extsb r, r
to u8:
  rlwinm r, r, 0, 24, 31
```

from byte:
```
s8 to s8 and u8 to u8:
  mr r, r
u8 to s8:
  extsb r, r
s8 to u8:
  rlwinm r, r, 0, 24, 31
```

## Summary ##
> No need to convert all variables to 32-bit. It takes only one cycle to convert from one type into another, same as give a value to the variable.