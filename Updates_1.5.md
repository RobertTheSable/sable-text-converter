# New In 1.5.0

## New parser setting: @end_on_label

Setting this option to `on` will cause Sable to generate a new block for each
`@label` setting, without requiring a manual end.

This could allowe a file like this:
```
@label name_alice
Alice[End]
@label name_bob
Bob[End]
@label name_eve
Eve[End]
```
Could be re-written as:
```
@end_on_label on
@label name_alice
Alice
@label name_bob
Bob
@label name_eve
Eve
```

## New parser setting: @export_width

Setting this option to `on` will cause Sable to export the widths of all blocks
in the current file.

A file such as:
```
@export_width on
@label name_alice
Alice
@label name_bob
Bob
@label name_eve
Eve
```
Would result in the following variables being defined:
```
!def_name_alice_length = ...
!def_name_bob_length = ...
!def_name_eve_length = ...
```
These variables may then be used in custom assembly.

## New config.yml Option: exportAllAddresses

Currently, Sable generates a definition for the address of every 
parsed text block. the new option "exportAllAddresses" can be set 
to "off" or "false" to only generate a definition for non-contiguous addresses.
This may potentially speed up the assembly step.

This option can be overwritten on a per-file basis using the `export_address` 
parser option. 
- Note that the `export_address` setting accepts only `on` or `off` values.

## Other changes

The order of includes in the main assembly file has been changed. The text and
testDefines files are not included at the beginning of the list so that the
length variables are available for custom assembly. This may break some
assembly if you were relying on the text address variables being undefined.
