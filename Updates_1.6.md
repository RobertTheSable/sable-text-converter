# New In 1.6.0

## New Font setting: Disable prefixes for certain commands

Prior to 1.6.0, all commands in fonts with a CommandValue defined would be 
prefixed by the defined CommandValue. 

Starting in 1.6.0, individual commands can disable this behavior.

Example usage:
```yaml
Commands:
    End:
        code: 0x08
        prefix: no
    NewLine:
        code: 0x09
        prefix: no
    SomeOthercommand:
        code: 0x10
        # prefix: yes is assumed by default
```

Defining non-prefixed commands applies some additional validation to glyphs.
Glyphs cannot have a code which is less than or equal 
the last non-prefixed command.

If there is a gap between the last known non-prefixed command and the first 
grapheme in the Encoding of any Page, then the widths of the font 
will be written at an offset of FontWidthAddress equal to the gap.

For example, assuming the last prefixed command is:
```yaml
NewLine:
    code: 0x09
    prefix: no
```
 and the first grapheme is:
```yaml
A:
    code: '0x10'
    length: 8
```
 then the data written for the font assembly file would be:
```asm
ORG !FontWidthsLocation
skip k
db 8, ; and so on....
```

## New Font settings: LastUnprefixedCommand, FirstPrefixedCommand

These settings may be used to define the last prefixed command if it is not
specified in the Commands section.

### LastUnprefixedCommand

If defined, sable will assume the last prefixed command is equal to this value.

### FirstPrefixedCommand

If defined, sable will assume the last prefixed command is one less than this value.

