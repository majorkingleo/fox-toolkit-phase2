# Library fixes that also apply to FOX 1.7.95

Defects that the console self-tests (`--test`, see `test-port-plan.md`) found in
the library code that fox2 inherited from `fox-1.7.95`, and that were fixed in
`fox2/lib` or `fox2/include`. Each entry gives the fox2 commit with the fix, the
fox2 file, and the place in `fox-1.7.95` that has the same code. The list is
meant as the base for patches to the vendor.

How the list was made: every library change of the test port commits was
compared with the `fox-1.7.95` source by reading it. All of them are present in
FOX 1.7.95 unchanged. The FOX 1 test programs were not run against the old
library, so a "present" below means the same code, not a reproduced failure.
The fox2 test that shows each defect is in `fox2/tests`, in the commit named.

Making a patch: `git show <commit> -- <fox2 file>` gives the change. The fox2
file names differ from FOX 1 where the platform seam split a file (for example
`FXProcess_posix.cpp` and `FXProcess_win32.cpp` are `FXProcess.cpp` in
FOX 1), and the include path is `include/` instead of `fox2/include/`; the hunks
are small and apply by hand.

Not in the list: changes made to fox2 only for the platform seam or for Wayland,
the fixes in the test programs themselves (the `match` test used `/` where
`PATHSEP` is `\`; `dictest` divided by zero on empty input). The initial copy of the library sources
(1cebd45) was not compared line by line with `fox-1.7.95`.

## Summary

| Commit | Area | fox2 files | FOX 1 files |
| --- | --- | --- | --- |
| 0ccf4f8 | process, Windows handles | `FXProcess_posix.cpp`, `FXProcess_win32.cpp`, `FXFile_win32.cpp`, `FXPipe_win32.cpp`, `FXMappedFile_win32.cpp` | `FXProcess.cpp`, `FXFile.cpp`, `FXPipe.cpp`, `FXMappedFile.cpp` |
| 81c3324 | units table | `FXUnits.cpp` | `FXUnits.cpp` |
| c8ad63c | math fallbacks | `fxmath.h`, `fxmath.cpp` | `fxmath.h`, `fxmath.cpp` |
| 5345c29 | printf | `fxprintf.cpp` | `fxprintf.cpp` |
| 9b90187 | half float | `FXhalf.h` | `FXhalf.h` |
| 097dc42 | time format and parse | `FXSystemTime.cpp`, `FXSystemTimeFormat.cpp`, `FXSystemTimeParse.cpp` | same names |
| 4ae416b | variant, JSON, INI, string to integer | `FXVariant.cpp`, `FXJSON.cpp`, `FXINI.cpp`, `fxstrtoll.cpp`, `fxstrtoull.cpp` | same names |
| c6e8176 | XML reader and writer | `FXXML.cpp` | `FXXML.cpp` |

Line numbers refer to the files of `fox-1.7.95` as they are in this repository.

## 0ccf4f8: process and Windows handle inheritance

Found by `process --test`.

1. `FXProcess::wait(FXint& code)` on POSIX returns the raw `waitpid` status:
   a child that exits with 42 gives 10752. Use `WIFEXITED`/`WEXITSTATUS`, and
   minus `WTERMSIG` for a child ended by a signal.
   FOX 1: `lib/FXProcess.cpp` line 774. (`wait()` without code at line 752 is
   not affected.)
2. On Windows the handle of a file, pipe or mapped file is inheritable exactly
   when `FXIO::Inheritable` was not given: `bInheritHandle=((m&FXIO::Inheritable)==0)`
   must be `!=0`. A child process therefore inherits every handle that was not
   meant for it, and cannot use the ones that were.
   FOX 1: `lib/FXFile.cpp` line 106, `lib/FXPipe.cpp` line 93,
   `lib/FXMappedFile.cpp` line 88.
3. Windows non-Unicode `FXProcess::start` (the variant with the environment)
   takes the standard error handle from `output` instead of `errors`.
   FOX 1: `lib/FXProcess.cpp` line 553 (`si.hStdError=output->handle();`; the
   Unicode variant at line 505 is correct).

## 81c3324: units table

Found by `units --test`.

1. `HorsePower` has the dimensions of energy, `kg*m^2/s^2`; power is
   `kg*m^2/s^3`. FOX 1: `lib/FXUnits.cpp` line 513.
2. `PoundForce` is 4.448222 N, rounded to 7 digits, so `psi` is off at the seventh
   digit; the exact value is 4.4482216152605. FOX 1: `lib/FXUnits.cpp`
   line 382.

## c8ad63c: math fallbacks and inverse error function

Found by `math --test`. Items 1 to 6 only matter where the C library lacks the
function (the `NO_...` macros, for example older MinGW); items 7 and 8 matter
everywhere.

1. `fmin`/`fmax` fallbacks return NaN when only the second argument is NaN;
   the C function returns the number. `fxmath.h`, FOX 1 lines 443, 453, 463,
   472.
2. `cbrt` fallback uses `pow(x,1/3)`: NaN for negative arguments, and `-Inf` gives
   `+Inf`. `fxmath.h` line 971, 980.
3. `hypot` fallback overflows above 1e154 and underflows below 1e-154.
   `fxmath.h` lines 1036, 1045.
4. `expm1` fallback is `exp(x)-1`, which has no precision near zero; this also
   spoils `sinh`, `tanh`, `asinh` and `atanh`. `fxmath.h` lines 1089, 1098.
5. `log1p` fallback is `log(1+x)`, the same problem. `fxmath.h` lines 1193,
   1203.
6. `log2` fallback is `log(x)*(1/ln 2)`, inexact for powers of two.
   `fxmath.h` line 1213 and the double version below it.
   (The fox2 replacements are in `fxmath.cpp`: Kahan's scaling for `expm1`
   and `log1p`, exponent splitting for `log2`, one Newton step for `cbrt`,
   scaling for `hypot`.)
7. `acosh(Inf)` is NaN: the test is `z.u<0x7f800000` and must be
   `<=` (float) and `z.u<FXULONG(0x7ff0000000000000)` must be `<=` (double).
   FOX 1: `lib/fxmath.cpp` lines 529, 553.
8. `inverf` and `inverfc` start from `sqrt(-log(0.5*x*x))`; the formula is
   `sqrt(-2*log(0.5*x))`. The error is up to 2e-6 (double) and 2e-2 (float).
   FOX 1: `lib/fxmath.cpp` lines 687, 716.

## 5345c29: printf of Inf and NaN

Found by `format --test`. `%010f`, `%010e`, `%010g` and `%010a` of Inf or NaN
pad with zeros (`0000000inf`); the C library pads with spaces. Clear
`FLG_ZERO` when the number is not finite. FOX 1: `lib/fxprintf.cpp`,
the cases `'f'`, `'e'`, `'g'`, `'a'` around lines 1710 to 1750.

## 9b90187: NaN to half

Found by `half --test`. `FXhalf(FXfloat)` turns a NaN whose mantissa has only
low bits set into infinity, because the table conversion drops those bits.
Fix: `if(((r.u&0x7fffffff)>0x7f800000) && (v&0x3ff)==0) v|=0x200;`.
FOX 1: `include/FXhalf.h` line 52.

## 097dc42: system time

Found by `timefmt --test`. FOX 1: `lib/FXSystemTime.cpp`,
`lib/FXSystemTimeFormat.cpp`, `lib/FXSystemTimeParse.cpp`.

1. `timeFromSystemTime` carries a day beyond the end of a month with a loop
   that carries wrongly (`Jan 100` gives the wrong date); `daysFromCivil` already
   handles any day number, so the loop can go. `FXSystemTime.cpp` line 167.
2. `universalTime(string,format)` and `localTime(string,format)` add the zone
   offset of the parsed text; the offset is seconds east of UTC, so it has to be
   subtracted. `FXSystemTime.cpp` lines 468, 497.
3. `localTime(utc,format)` stores the zone offset before `systemTimeFromTime`,
   which overwrites it, so `%z` always printed `+0000`; the offset must also
   be seconds east of UTC, the negative of the zone offset. `FXSystemTime.cpp` line 484.
4. `%z` prints `+0030` for -00:30 (the sign is taken from the hours, which are 0).
   `FXSystemTimeFormat.cpp` line 317.
5. `%fm` and `%fu` round, and the result can overflow its digits (`1000`);
   they should truncate. `FXSystemTimeFormat.cpp` lines 354, 358.
6. `%G` is wrong for 3 January that belongs to the previous ISO year:
   `st.yday<3` must be `st.yday<4`. `FXSystemTimeFormat.cpp` line 218.
7. `%y` and `%g` guess the century the wrong way round: `num<69` gives 2000+num,
   not 1900+num. `FXSystemTimeParse.cpp` lines 420, 436.
8. `%s` cannot parse negative times, and the overflow check is against
   `LLONG_MAX` in seconds instead of in nanoseconds. `FXSystemTimeParse.cpp`
   lines 597 to 610.
9. `%U`, `%W`, `%V` are ignored when followed by `%T`, `%D`, `%F`, `%c`, `%r`,
   `%R`, `%x`, `%X`: the derived fields (day of year and the rest) are
   calculated inside the nested parse, before everything else is known. Fix:
   a parameter `outer` so only the outermost call calculates.
   `FXSystemTimeParse.cpp` lines 336, 631 to 659, 684 and 748.

## 4ae416b: variant, JSON, INI, string to integer

Found by `variant --test`. FOX 1: the same file names in `lib/`.

1. `FXVariant::assign` frees its contents before it copies, so `v=v["a"]`
   (assigning a part of itself) reads freed memory. `FXVariant.cpp` line 179.
   The string assignments `operator=(const FXchar*)` and
   `operator=(const FXString&)` have the same problem
   (`v=v.asChars()+1`); copy first, then `clear()`.
2. `toUInt` reports 0 and `UINT_MAX` as failures (`<=0` and `>=UINT_MAX`
   must be `<0` and `>UINT_MAX`). `FXVariant.cpp` lines 429, 430.
3. `toInt`, `toUInt`, `toLong`, `toULong` read a `ULong` as signed and a
   negative value as unsigned, instead of clamping and clearing `ok`.
   `FXVariant.cpp` from line 380 to 520.
4. `toFloat` and `toDouble` read a negative `char` as unsigned (`-1` gives
   1.8e19). `FXVariant.cpp` lines 538 to 600.
5. `__strtoll` and `__strtoull` detect overflow with
   `((FXlong)(value-prior))>>63`, which flags valid values
   (18446744073709551615 gives `ok=false` and the maximum) and misses others.
   Use `value>cutoff || (value==cutoff && digit>cutlim)` with
   `cutoff=ULLONG_MAX/base`. `fxstrtoll.cpp` lines 103 to 105,
   `fxstrtoull.cpp` lines 100 to 102.
6. The same two functions skip the leading `0` before looking for digits, so
   `"0"` has no digits and is reported as a failure (`ok=false`). Only skip it
   as part of `0x` or `0b`; with base 0 a lone `0` is an octal digit.
   `fxstrtoll.cpp` line 81, `fxstrtoull.cpp` line 78.
7. JSON integers above `LLONG_MAX` or equal to `LLONG_MIN` (`-9223372036854775808`)
   are lost; with the fix in item 5 they load as `ULong` or exactly.
   `FXJSON.cpp` lines 1250 to 1321 (six `__strtoll` sites).
8. A bare `null`, `true`, `false` or identifier at the very end of JSON text
   (`"null"` with no trailing character) is reported as end of input,
   because `FXJSON::ident()` returns `TK_EOF` when it runs out of data before
   it has looked at the word. `FXJSON.cpp` line 906.
9. An array or object that is not closed (`"[1,2"`, `"{\"a\":1"`) gives
   `ErrBracket` or `ErrBrace`; it should be `ErrEnd`. `FXJSON.cpp` lines 1340,
   1347.
10. INI: a value in a CRLF file keeps the `\r`, because `next()` steps over the
    `\r\n` before it returns the value token. `FXINI.cpp` line 240.
11. INI: the last value of a file without a final newline is a parse error;
    `next()` returns `TK_EOF` with the value still open. `FXINI.cpp` line 507.
12. INI: blanks around a key and around an unquoted value are part of the key
    and value (`  key  =  v  `). Trim them. `FXINI.cpp` lines 534, 548.

Noted and not changed, in both versions: the comment of `FXJSON::load` says
that loading into a populated variant overlays keys, but `loadMap` calls
`setType(MapType)`, which clears, so the content is replaced. INI values always
load as strings. The default INI constructor and the buffer constructor use a
different numeric precision (15 and 16). The FOX string to double conversion
returns Inf for `DBL_MAX` written with 17 digits.

## c6e8176: XML reader and writer

Found by `xml --test`. FOX 1: `lib/FXXML.cpp`.

1. `FXXML::encode` escapes every byte whose table entry has the `ESCAPE` bit,
   and the table marks the UTF-8 continuation bytes 0x80 to 0x9F that way. The
   writer therefore turns `E2 82 AC` (the euro sign) into `E2 &#x82; AC`, which
   reads back as U+0082 and corrupts the text; the same happens to the
   typographic quotes (`E2 80 9C`) and most other characters outside Latin 1.
   Only bytes below 0x80 should be escaped. FOX 1: lines 493 and 534
   (`if(restrictedChar(ch) && (flags&REFS))`).
2. The same table has the `ESCAPE` bit on `;` (entry 59, value 0x26; it must
   be 0x22), so every `;` is written as `&#x3B;`. FOX 1: line 238.
3. `encode` writes a CR raw. The reader turns a raw CR into LF (flag `CRLF`),
   so a CR does not survive a round trip; write it as `&#x0D;`. FOX 1: lines
   493 and 534.
4. `parseprocessing` keeps the blank between the target and the data, so
   `<?pi data?>` is reported as target `pi` and data ` data`, and the writer,
   which writes a blank of its own, gives `<?pi  data?>` after a round trip.
   Skip the white space (`spaces()`) before the data. FOX 1: line 1195.
5. `parseexternalid` tests `match("SYSTEM",6)` and then `match("PUBLIC",6)`;
   `match` reads the next name and consumes it even when it does not match, so
   `PUBLIC` is never recognized and `<!DOCTYPE a PUBLIC "id" "a.dtd">` fails
   with `ErrToken`. Read the keyword once and compare it twice; an unknown
   keyword is then an error instead of being accepted. FOX 1: lines 1059 to
   1075.
6. A run of white space (blank, tab, line feed) longer than the buffer minus
   `MAXTOKEN` gives `ErrEof` when reading from a file: the pending text is only
   handed over (`characters`, `str.append`, `data.append`) in the `default`
   branch of the scan loop, never in the white space branches, so the buffer
   cannot be refilled. It affects text, attribute values, comments, CDATA and
   processing instructions. Do the check at the top of the loop. FOX 1:
   `parsestring` (line 865), `parseprocessing` (1190), `parsecomment` (1244),
   `parsecdata` (1402) and `parsecontents` (1455).

Noted and not changed, in both versions: `parsecontents` calls `characters`
with an empty string before every child element; nothing after the root
element is checked; `ErrReference` is never returned; the encoder does not
escape the control characters U+0080 to U+009F that the XML grammar lists as
restricted; a truncated end tag (`<a>x</a`) gives `ErrNoMatch` and not
`ErrEof`; the writer writes `\r\n` for a line feed on Windows.