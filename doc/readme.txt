==============================================================================================
                                     Sable Text Converter
                                           By Robert
==============================================================================================

A text parser/inserter targeting Fire Emblem: Monshou no Nazo, featuring:
* UTF-8 support.
* Code page switching.
* Support for multiple user-defined encoding types.
* Automatic error/format checking.
* A built-in assembler so you can compile your script and assembly changes at the same time.

==============================================================================================
                                       How do I use it?
==============================================================================================

Set up a text project. You can use These projects as examples:
- https://github.com/RobertTheSable/sable-text-converter/tree/master/example
- https://github.com/RobertTheSable/fe3-translation
See project_structure.md for more details on configuration.

Once you've done that, you can run sable. Sable needs to be run from the command line.

You can either place Sable + all its DLLs/libraries in the root of your text project,
or you can pass the location via the --project argument.

Sable first parses your script and generates binaries, alongside several 
Asar-comparible ASM files. Then it uses Asar to assemble the generated files to
each fo the ROM files specified in your config.yml. You can disable automatic assembly
using the --no-assembly argument. You can also skip script parsing using the --no-script flag.
